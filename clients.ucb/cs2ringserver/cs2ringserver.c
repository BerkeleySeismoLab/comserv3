/************************************************************************
 * cs2ringserver (aka cs2ring)
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>

#define VERSION	"1.02 (2020.273)"

#ifdef COMSERV2
#define CLIENT_NAME	"2RNG"
#else
#define CLIENT_NAME	"CS2RINGSERVER"
#endif

char *syntax[] = {
"%s   version " VERSION,
"%s - write MSEED data records from comserv server or datsock socket to ringserver.",
"Syntax:",
"%s  [-H host] [-S service] [-p passwd] [-P passwdfile] [-R]",
"    [-o station.net] [-n client_name] [-v n] [-h] station_list",
"    where:",
"	-O ringserver	Name of ringserver (host:port).",
"	-H host		Specify remote host that provides data via a socket.",
"	-S service	Specify socket name/number on remote host for data.",
"	-p passwd	Passwd to specify via remote socket.",
"	-P passwdfile	File containing passwd to specify via remote socket.",
"	-o station.net	Override the station and network in the MSEED data",
"			data records with the specified station.net.",
"	-n client_name	Override default comserv client name with new name.",
"			Default client name is " CLIENT_NAME ".",
"	-R		Request stations and channels via remote socket.",
"	-v n		Set verbosity level to n.",
"	-f		Flush - start with new data.",
"	-h		Help - prints syntax message.",
"	station_name	Comserv station name.",
"Notes:",
"1.  Program will not register as a comserv client if it is receiving",
"    MSEED data from a datasock socket.",
NULL };

#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <qlib2.h>

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#ifdef	end
#undef	end
#endif

#include "channel_info.h"
#include "datasock_codes.h"
#include "basics.h"

#include "scnl_convert.h"

#include "libdali.h"

#define	SEED_BLKSIZE	512
#define	SEED_MAX_BLKSIZE 8192

/************************************************************************
 *  Externals variables and functions.
 ************************************************************************/
FILE *info;			/* Default FILE for messages.		*/
char *cmdname;			/* Program name.			*/
int verbosity;			/* Verbosity flag.			*/
int terminate_proc;		/* Terminate program flag;		*/
int skip_ack = 0;		/* skip comserv ack of packets.		*/
int flush = 0;			/* Default is no flush (blocking client).*/
int nchannel;			/* number of entries in channelv.	*/
CHANNEL_INFO **channelv = NULL;	/* list of channels.			*/
DLCP *dlconn;			/* descriptor for ring connection	*/
char *new_sn;			/* Optional station.channel rename.	*/
int ack = 0;			/* Default is no ack from ringserver.	*/
tclientname client_name;	/* Comserv client name			*/

/*  Signal handler variables and functions.				*/
void finish_handler(int sig);
void terminate_program (int error);

extern int fill_from_socket (char *station, char *host, char *service,
			     char *passwd, int request_flag);
extern int fill_from_comserv (char *station);
extern int write_to_ring (seed_record_header *pseed);
extern int decompress_data(DATA_HDR*, unsigned char *, int *);

/************************************************************************
 *  finish_handler:
 *	Signal handler to handle program termination cleanly.
 ************************************************************************/
void finish_handler(int sig)
{
    terminate_proc = 1;
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)	*/
}

/************************************************************************
 *  ringput_mseed
 ************************************************************************/
int ringput_mseed( char *pseed, int size )
{
    char	streamid[MAXSTREAMID];
    int	rc;
    double ds, de;
    dltime_t datastart, dataend;

    DATA_HDR *hdr;

    hdr = decode_hdr_sdr ((void *)pseed, size);
    if (hdr) {
	trim (hdr->station_id);
	trim (hdr->network_id);
	trim (hdr->channel_id);
	trim (hdr->location_id);
    }
    else {
	return (FAILURE);
    }
    /* Ringserver streamid is NET_STA_LOC_CHA/MSEED */
    sprintf( streamid, "%s_%s_%s_%s/MSEED", hdr->network_id,
	    hdr->station_id, hdr->location_id, hdr->channel_id);
    
    ds = int_to_nepoch (hdr->begtime);
    de = int_to_nepoch (hdr->endtime);
    datastart = DL_EPOCH2DLTIME(ds);
    dataend = DL_EPOCH2DLTIME(de);
    free_data_hdr (hdr);

    rc = dl_write( dlconn, pseed, size, streamid, datastart, dataend, ack);

    if( rc < 0 ) {
	fprintf( info, "ringput failed for packet source %s\n", streamid );
    }
    if (verbosity & 8) {
	fprintf (info, "dlconn=0x%p pseed=0x%p size=%d streamid=%s datastart=%" PRId64 ", dataend=%" PRId64 ", ack=%d\n",
		 (void *)dlconn, (void *)pseed, (unsigned int)size, streamid, datastart, dataend, ack);
    }
    return (rc);
}

void remap_scnl (DATA_HDR *hdr) 
{
    S2S s;
    s.scnl_in.s = hdr->station_id;
    s.scnl_in.c = hdr->channel_id;
    s.scnl_in.n = hdr->network_id;
    s.scnl_in.l = hdr->location_id;
    if (scnl2scnl(&s)) {
	strncpy (hdr->station_id, s.scnl_out.s, DH_STATION_LEN);
	strncpy (hdr->channel_id, s.scnl_out.c, DH_CHANNEL_LEN);
	strncpy (hdr->network_id, s.scnl_out.n, DH_NETWORK_LEN);
	strncpy (hdr->location_id, s.scnl_out.l, DH_LOCATION_LEN);
    }
    return;
}

/************************************************************************
 *  write_to_ring:
 *	Write SEED packet to ring.
 ************************************************************************/
int write_to_ring(seed_record_header *tseed)
{
    DATA_HDR *hdr;
    BS *bs = NULL;
    BLOCKETTE_HDR *bh = NULL;
    int status;
    char out_buf[SEED_MAX_BLKSIZE];
    int data_offset, datalen;

    if ((hdr = decode_hdr_sdr((SDR_HDR *)tseed, SEED_BLKSIZE)) == NULL)
    {
	return(FAILURE);
    }

    if(hdr->blksize != SEED_BLKSIZE)
    {
	printf("Invalid SEED blocksize %d\n", hdr->blksize);
	free_data_hdr(hdr);
	return(FAILURE);
    }

    /* Remap the SNCL if required. */
    if (new_sn) remap_scnl (hdr);

    /* Fill in the number of data frames.   */
    if ((bs = find_blockette(hdr,1001)) &&
	(bh = (BLOCKETTE_HDR *)(bs->pb))) {
	hdr->num_data_frames = ((BLOCKETTE_1001 *)bh)->frame_count;
	/* Explicitly set num_data_frames for SHEAR stations.	*/
	if (hdr->num_data_frames == 0 && hdr->sample_rate != 0)
	    hdr->num_data_frames = (hdr->blksize - hdr->first_data) / 
		sizeof(FRAME);
	}
	else hdr->num_data_frames =
	    (hdr->sample_rate == 0) ? 0 :
	    (hdr->blksize - hdr->first_data) / sizeof(FRAME);

    if (verbosity & 8) {
	int seconds, usecs;
	INT_TIME etime;
	printf ("packet %s.%s.%s.%s %s to ",
		hdr->station_id, hdr->network_id, 
		hdr->channel_id, hdr->location_id, time_to_str(hdr->begtime,MONTHS_FMT_1));
	time_interval2 (1, hdr->sample_rate, hdr->sample_rate_mult, 
			&seconds, &usecs);
	etime = add_time(hdr->endtime, seconds, usecs);
	printf ("%s nsamples=%d\n", 
		time_to_str(etime,MONTHS_FMT_1), hdr->num_samples);
    }

    /* Rewrite MiniSEED header for this record.	*/
    data_offset = hdr->first_data;
    datalen = hdr->blksize - data_offset;
    memset ((void *)out_buf, 0, hdr->blksize);
    init_sdr_hdr ((SDR_HDR *)out_buf, hdr, NULL);
    if (datalen > 0) {
	memcpy (out_buf+data_offset, ((char *)tseed)+data_offset, datalen);
    }
    update_sdr_hdr ((SDR_HDR *)out_buf, hdr);

    /*:: Write data to ring */
    status = ringput_mseed (out_buf, hdr->blksize);
    if (verbosity & 8) {
	if (status >= 0) {
	    fprintf (info, "wrote %s.%s.%s.%s data to ring.\n",
		     hdr->station_id, hdr->network_id,
		     hdr->channel_id, hdr->location_id);

	}
	else {
	    fprintf (info, "Error %d writing to ring: "
		     "%s.%s.%s.%s %s nsamples=%d\n", status,
		     hdr->station_id, hdr->network_id,
		     hdr->channel_id, hdr->location_id,
		     time_to_str(hdr->begtime,MONTHS_FMT_1),
		     hdr->num_samples);
	}
    }

    free_data_hdr(hdr);
    fflush (info);
    return (status >= 0) ? SUCCESS : FAILURE;
}

/************************************************************************/
/*  main procedure							*/
/************************************************************************/
int main (int argc, char **argv)
{
    tservername station;
    char *host = NULL;
    char *service = NULL;
    int request_flag = 0;
    int status, n;
    char *passwd = NULL;
    char *passwdfile = NULL;
    char passwd_str[80];
    FILE *fp;
    char ringserver[128];		/* IP:Port of ringserver */

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind, opterr;
    int		c;

    info = stdout;
    strncpy(client_name, CLIENT_NAME, CLIENT_NAME_SIZE);
    client_name[CLIENT_NAME_SIZE-1] ='\0';    
    
    cmdname = argv[0];
    while ( (c = getopt(argc,argv,"hRv:O:H:S:p:P:o:n:")) != -1)
	switch (c) {
	case '?':
	case 'h':   print_syntax (cmdname,syntax,stdout); exit(0);
	case 'v':   verbosity=atoi(optarg); break;
	case 'O':   strcpy(ringserver,optarg); break;
	case 'H':   host = optarg; break;
	case 'S':   service = optarg; break;
	case 'p':   passwd = optarg; break;
	case 'P':   passwdfile = optarg; break;
	case 'R':   request_flag |= SOCKET_REQUEST_CHANNELS; break;
	case 'o':   new_sn = optarg; break;
	case 'n':   strncpy(client_name,optarg,CLIENT_NAME_SIZE);
		    client_name[CLIENT_NAME_SIZE-1] ='\0'; break;
	}

    /*	Skip over all options and their arguments. */

    argv = &(argv[optind]);
    argc -= optind;
    info = stdout;

    /* Allow override of station name on command line */
    if (argc > 0) {
	strncpy(station,argv[0],SERVER_NAME_SIZE);
	station[SERVER_NAME_SIZE-1]= '\0';
    }
    else {
	fprintf (stderr, "Missing station name\n");
	exit(1);
    }
    upshift(station) ;

    if ((host || service) && ! (host && service)) {
	fprintf (stderr, "Must specify both -H host and -S service\n");
	exit(1);
    }

    dl_loginit (verbosity, NULL, NULL, NULL, NULL);

    if (passwdfile) {
	if ((fp = fopen (passwdfile, "r")) == NULL) {
	    fprintf (stderr, "Unable to open password file %s\n", passwdfile);
	    exit(1);
	}
	n = fscanf (fp, "%s", passwd_str);
	if (n <= 0) {
	    fprintf (stderr, "Error reading password from file %s\n", passwdfile);
	    exit(1);
	}
	passwd = passwd_str;
    }


    if (new_sn) {
	S2S s;
	char *p1 = NULL, *p2 = NULL;
	char *str = strdup(new_sn);
	upshift(str) ;
	s.scnl_in.s = strdup("*");
	s.scnl_in.n = strdup("*");
	s.scnl_in.c = strdup("*");
	s.scnl_in.l = strdup("*");
	s.scnl_out.c = strdup("*");
	s.scnl_out.l = strdup("*");
	p1 = strtok (str, ".");
	p2 = strtok (NULL, ".");
	if (p1 == NULL || p2 == NULL) {
	    fprintf (stderr, "Invalid station.net specified for override: %s\n", new_sn);
	    exit( -1 );
	}
	s.scnl_out.s = strdup(p1);
	s.scnl_out.n = strdup(p2);
	free (str);
	if (s2s_set(s) == 0) {
	    fprintf (stderr, "Error initializing station.net renaming.\n");
	    exit(1);
	}
	sort_scnl();
    }

    dlconn = dl_newdlcp (ringserver, cmdname);
    if( dlconn == NULL ) {
	fprintf (stderr, "Failed to allocate dlconn for connection to ringserver\n" );
	exit( -1 );
    }
    status = dl_connect(dlconn);
    if(  status < 0 ) {
	fprintf (stderr, "Failed to connect to ringserver\n" );
	exit( -1 );
    }

    if (passwd) request_flag |= SOCKET_REQUEST_PASSWD;
    if (host && service) {
	status = fill_from_socket (station, host, service, passwd, request_flag);
    }
    else {
	status = fill_from_comserv (station);
    }
    terminate_program (status);
    return (0);
}

