/************************************************************************
 * Program:	datasock
 * Author:
 *	Douglas Neuhauser, UC Berkeley Seismographic Station, 
 *
 *	Adopted from a demo program by written by 
 *	Woodrow H. Owens for Quanterra, Inc.
 *	Copyright 1994 Quanterra, Inc.
 *	Copyright (c) 1998-2000 The Regents of the University of California.
 * 
 * Purpose:
 *	Acquire Mini-SEED data from comserv process(es), and output data
 *	over an Internet socket.

Modification History:
Date     who	Version	Modifications.
------------------------------------------------------------------------
95/xx/xx DSN	1.0	Initial coding.
95/05/24 DSN	1.1	Allowed -i option to allow station and channels 
			to be requested via the socket.
96/06/27 DSN	1.2	Added optional multi-line station and channel
			requests via the socket.
			Added station_list channel_list requests.
			Added optional passwords on requests.
			Allow station_list channel_list request via cmdline.
			Split source into multiple files.
97/03/14 DSN	1.4	Converted to qlib2.
97/10/10 DSN	1.4.1	Added option to specify comserv client name.
97/12/19 DSN	1.4.2	Added -M and -m options to specify multiple comservs
			handling unique stations for a single site.  
			This is NOT for redundant comservs.
98/01/31 DSN	1.4.3	Cosmetic fix to syntax display.
98/02/20 DSN	1.4.4	Added periodic read on socket to detect closed socket.
98/02/23 DSN	1.4.5	Read on socket to detect closed socket uses interval
			timer instead of one-shot timer.
98/05/08 DSN	1.4.6	Removed all multi_comserv code.
			Require "site=sitename" (eg "site=SAO") in stations.ini
			file for comlink with a different name than the sitename.
2003/12/16 DSN	1.4.7	Increase max packets on cs_setup call from 10 to 50.
2007/01/24 PNL  1.4.8	Added support for location code within channel string
			in format "CCC.LL".
2007/08/27 DSN	1.4.9	Changed signal handling from signal() to sigaction().
			Added timeout in interactive mode.
2009/07/16 DSN	1.5.1	Updated for Solaris 10, added function declarations
			to remove compiler warnings.
2020.273   DSN  1.6.0   Updated for comserv3.
			Modified for 15 character station and client names.
			Command line argument for Station list consists of 
			wildcarded station or station.net entries.
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define	VERSION	"1.6.0 (2020.273)"

#ifdef COMSERV2
#define	DEFAULT_CLIENT	"DSOC"
#else
#define	DEFAULT_CLIENT	"DATASOCK"
#endif

char *syntax[] = {
"%s - Version " VERSION,
"%s [-S port] [-p passwd | -P passwdfile] [-c client_name]",
"	[-v n] [-h] station_list channel_list",
"    where:",
"	-S port		Specify port number to listen to for connections.",
"			If no port is specified, assume port is already open",
"			on unit 0 (from inetd).",
"	-p passwd	Password required from remote connection.",
"	-P passwdfile	File containing password required from remote system.",
"	-c client_name	Comserv client name.  Default is '" DEFAULT_CLIENT "'.",
"	-v n		Set verbosity level to n.",
"	-h		Help - prints syntax message.",
"	-i		Interactive - get station and channel list from port.",
"	sta_net_list	List of station.net names (comma-delimited).",
"			Station name with no net matches all (wildcard) nets.",
"	channel_list	List of channel.location names (comma-delimited).",
"			channel.location names are in CCC.LL format.",
"			CCC specifies channel with all (wildcard) location codes.",
"			CCC. specified channel with blank location code.",
NULL };

#include <errno.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>
#include <unistd.h>
#include <fnmatch.h>
#include <math.h>
double log2(double);

#include "qlib2.h"

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"
pchar seednamestring (seed_name_type *sd, location_type *loc);

#include "statchan.h"
#include "bld_statchans.h"
#include "cs_station_list.h"
#include "set_selectors.h"
#include "read_socket_request.h"
#include "datasock_codes.h"

#define	SEED_BLKSIZE	512
#define	TIMESTRLEN	40
#define	POLLTIME	250000	/* microseconds to sleep.		*/
#define TIMEOUT	EINTR

/************************************************************************
 *  Externals symbols.
 ************************************************************************/
char *cmdname;			/* Program name.			*/
static int port = -1;		/* No explicit port means the program	*/
				/* should inherit the socket on stdout.	*/
FILE *info;			/* Default FILE for messages.		*/
static tclientname client = DEFAULT_CLIENT;	/* Default client name	*/
static int nreq;		/* # of stations in request list.	*/
static int ngen;		/* # of stations in comserv list.	*/
static STATCHAN *req;		/* station info from requests.		*/
static STATCHAN *gen;		/* station info for comserv.		*/
static int terminate_proc;	/* flag to terminate program.		*/
static int verbosity;

static pclient_struc me = NULL;	/* ptr to comserv shared memory.	*/
typedef char char23[24];	/* char string type for status msgs.	*/
static char23 stats[11] = {	/* status msgs for comserv states.	*/
    "Good", "Enqueue Timeout", "Service Timeout", "Init Error",
    "Attach Refused", "No Data", "Server Busy", "Invalid Command",
    "Server Dead", "Server Changed", "Segment Error" };

void finish_handler(int sig);
void stopread(int sig);
char *gmt_clock_string(time_t clock);
int terminate_program (int error);
int defer_terminate;
int setup_socket (int port);
int write_seed (int sd, seed_record_header *pseed, int blksize);
int xcwrite (int fd, char buf[], int n);
int check_socket (int sd);

/************************************************************************/
/*  main procedure							*/
/************************************************************************/
int main (int argc, char *argv[], char **envp)
{
    pclient_station this;
    boolean alert;
    pdata_user pdat;
    seed_record_header *pseed;
    tstations_struc stations;
    short data_mask = (CSIM_DATA);	/* data mask for cs_setup.	*/

    char station_name[8];
    char time_str[TIMESTRLEN];
    int sd;
    int maxstation;
    int maxselectors;
    int i, j, k, n;
    int status;
    FILE *fp;
    char *p;
    int interactive = 0;		/* flag for remote station list	*/
    int request_flag;
    char *passwdfile = NULL;
    char *passwd = NULL;		/* Optional password required.		*/
    char passwdstr[80];
#ifdef SUNOS4
    int usleeptime;
#else
    struct timespec rqtp, rmtp;
#endif
    struct sigaction action;

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind, opterr;
    int		c;

    info = stdout;
    cmdname = tail(argv[0]);
    while ( (c = getopt(argc,argv,"hip:P:S:M:m:c:v:")) != -1)
      switch (c) {
	case '?':
	case 'h':   print_syntax (cmdname,syntax,info); exit(0);
	case 'i':   interactive = 1; break;
	case 'S':   port=atoi(optarg); break;
	case 'p':   passwd = optarg; break;
	case 'P':   passwdfile = optarg;
	case 'c':   strncpy (optarg,client,CLIENT_NAME_SIZE); client[CLIENT_NAME_SIZE-1]='\0'; break;
	case 'v':   verbosity=atoi(optarg); break;
      }

    /*	Skip over all options and their arguments.			*/
    argv = &(argv[optind]);
    argc -= optind;
    if (port < 0) info = stderr;
    if (passwdfile) {
	if ((fp = fopen (passwdfile, "r")) == NULL) {
	    fprintf (stderr, "Unable to open password file %s\n", passwdfile);
	    exit(1);
	}
	n = fscanf (fp, "%s", passwdstr);
	if (n <= 0) {
	    fprintf (stderr, "Error reading password from file %s\n", passwdfile);
	    exit(1);
	}
	passwd = passwdstr;
    }

#ifdef	SUNOS4
    usleeptime = POLLTIME;
#else
    rqtp.tv_sec = (1000 * POLLTIME) / 1000000000 ;
    rqtp.tv_nsec = (1000 * POLLTIME) % 1000000000 ;
#endif

    terminate_proc = 0;
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

    /* Set up a condition handler for SIGPIPE, since a write to a	*/
    /* close pipe/socket raises a alarm, not an error return code.	*/
    /*:: sigignore (SIGPIPE); */
    signal (SIGPIPE,finish_handler);

    /* Either setup the socket from the specified port number,		*/
    /* or assume that the socket was inherited from parent (inetd).	*/
    if (port >= 0)
      sd = setup_socket(port);
    else
      sd = 0;

    if (interactive) {
	request_flag = SOCKET_REQUEST_CHANNELS;
	if (passwd) request_flag |= SOCKET_REQUEST_PASSWD;
	status = read_socket_request (sd, &req, &nreq, request_flag, passwd);
	if (status != 0) {
	    exit(1);
	}
    }
    else {
	if (argc < 2) {
	    fprintf (stderr, "Error - missing station and/or channel list\n");
	    exit(1);
	}

/*::
	printf ("argc = %d\n", argc);
	for (i=0; i<argc; i++) printf ("argv[%d] = %s\n", i, argv[i]);
::*/

	status = bld_statchans (argv[0], argv[1], &req, &nreq);
	if (status != 0) {
	    fprintf (stderr, "Error parsing station and/or channel list\n");
	    exit(1);
	}
	if (passwd) {
	    request_flag = SOCKET_REQUEST_PASSWD;
	    status = read_socket_request (sd, &req, &nreq, request_flag, passwd);
	    if (status != 0) {
		fprintf (stderr, "Error reading socket request\n");
		exit(1);
	    }
	}
    }

    /* Set up a permanently installed signal handler for SIGALRM. */
    action.sa_handler = cs_sig_alrm;
    action.sa_flags = 0;
    sigemptyset (&(action.sa_mask));
    sigaction (SIGALRM, &action, NULL);

    /* Generate desired station channel list from request list based on */
    /* actual station list on the system.  				*/
    /* Handle station wildcard matching in request list.		*/
    status = cs_station_list(&gen, &ngen ,&req, &nreq);
    if (status != 0) {
	fprintf (stderr, "Error generating station channel list\n");
	exit(1);
    }

    /* Determine maximum number of explicit selectors required.		*/
    maxselectors = 0;
    for (i=0; i<ngen; i++) {
	if (gen[i].nchannels > maxselectors) maxselectors = gen[i].nchannels;
    }

    /* Generate an entry for all available stations.			*/      
    cs_setup (&stations, client, "*", TRUE, TRUE, 50, maxselectors+1, data_mask, 6000);

    
    /* Remove any station that was not requested.			*/
    for (i=0; i<stations.station_count; ) {
	p = sname_str_cs(stations.station_list[i].stationname);
        strcpy((char *)station_name,p);	
	for (j=0; j<ngen; j++) {
	    if (fnmatch(station_name,gen[j].station,0)==0) break;
	}
	/* Determine whether station should be kept or removed.		*/
	if (j >= ngen) cs_remove(&stations,i);
	else {	
	    i++;
	    if (verbosity) {
		fprintf (info, "Matching station %s\n", station_name);
		fflush(info);
	    }
	}
    }
    
    /* Create my segment and attach to all stations */      
    defer_terminate = 1;
    me = cs_gen (&stations);
    if (me == NULL) {
	fprintf (stderr, "Error created shared memory for %s\n", cmdname);
	exit(1);
    }
    maxstation = me->maxstation;
    if (maxstation <= 0) {
	fprintf (stderr, "Error - maxstation = %d\n", maxstation);
	exit(1);
    }

    /* Setup selectors for the requested stations and chanenls.		*/
    for (i=0; i<maxstation; i++) {
	this = (pclient_station) ((long) me + me->offsets[i]);
	p = sname_str_cs(this->name);
        strcpy((char *)station_name,p);	
	for (j=0; j<ngen; j++) {
	    if (strcmp(station_name, gen[j].station) != 0) continue;
	    status = set_selectors(me, i, DATAQ, 1, gen[j].channel, gen[j].nchannels);
	    if (status != 0) terminate_program(status);
	    break;
	}
	if (j >= ngen) {
	    fprintf (stderr, "Error - channel list not found for station %s\n",
		     station_name);
	    terminate_program(1);
	}
    }

    /* Show beginning status of all stations */
    strcpy(time_str, localtime_string(dtime()));
    for (j = 0; j < me->maxstation; j++) {
	this = (pclient_station) ((long) me + me->offsets[j]);
	this->seqdbuf = CSQ_LAST;
	if (port >= 0) {
	    fprintf (info, "%s - [%s] Status=%s\n", time_str, sname_str_cs(this->name), 
		     (char *)stats[this->status]);
	}
    }
    fflush (info);

    /* Continually scan all comservs for MiniSEED records.		*/
    while (! terminate_proc) {
	j = cs_scan (me, &alert);
	if (j != NOCLIENT) {
	    this = (pclient_station) ((long) me + me->offsets[j]);
	    if (alert) {
		strcpy(time_str, localtime_string(dtime()));
		if (port >= 0) {
		    fprintf (info, "%s - New status on station %s is %s\n", time_str,
			     sname_str_cs(this->name), (char *)stats[this->status]);
		    fflush (info);
		}
	    }
	    if (this->valdbuf) {
		pdat = (pdata_user) ((long) me + this->dbufoffset);
		for (k = 0; k < this->valdbuf; k++) {
		    pseed = (pvoid) &pdat->data_bytes;
		    if (verbosity & 1) {
			if (port >= 0) {
			    fprintf (info, "[%s] <%2d> %s recvtime=%s ",
				     sname_str_cs(this->name), k, 
				     (char *)seednamestring(&pseed->channel_id, &pseed->location_id), 
				     (char *)localtime_string(pdat->reception_time));
			    printf("hdrtime=%s\n", time_string(pdat->header_time));
			    fflush (info);
			}
		    }
		    write_seed(sd, pseed, SEED_BLKSIZE);
		    pdat = (pdata_user) ((long) pdat + this->dbufsize);
		}
	    }
	}
	else {
	    if (check_socket(sd) < 0) {
		terminate_proc = 1;
		break;
	    }
	    if ((verbosity & 2) && port >= 0) {
		fprintf (info, "sleeping...");
		fflush (info);
	    }
#ifdef SUNOS4
	    usleep (usleeptime) ;
#else
	    nanosleep (&rqtp, &rmtp) ;
#endif
	    if ((verbosity & 2) && port >= 0) {
		fprintf (info, "awake\n");
		fflush (info);
	    }
	}
    }

    /* Clean up comserv shared memory and exit.				*/
    terminate_program (0);
    return(0);
}

/************************************************************************
 *  finish_handler:
 *	Signal handler -  sets terminate flag so we can exit cleanly.
 ************************************************************************/
void finish_handler(int sig)
{
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)	*/
    terminate_proc = 1;
    if (! defer_terminate) terminate_program (0);
    return;
}

/************************************************************************
 *  gmt_clock_str:
 *	Generate time string with GMT time.
 ************************************************************************/
char *gmt_clock_string(time_t clock)
{
    static char time_str[TIMESTRLEN];
    strftime(time_str, TIMESTRLEN, "%Y/%m/%d,%T GMT", gmtime(&clock));
    return(time_str);
}

/************************************************************************
 *  terminate_program
 *	Terminate prog and return error code.  Clean up on the way out.
 ************************************************************************/
int terminate_program (int error) 
{
    pclient_station this;
    char time_str[TIMESTRLEN];
    int j;
    boolean alert;

    strcpy(time_str, localtime_string(dtime()));
    if ((verbosity & 2) && port >= 0) {
	fprintf (info, "%s - Terminating program.\n", time_str);
	fflush (info);
    }

    /* Perform final cs_scan for 0 records to ack previous records.	*/
    /* Detach from all stations and delete my segment.			*/
    if (me != NULL) {
	for (j=0; j< me->maxstation; j++) {
	    this = (pclient_station) ((long) me + me->offsets[0]);
	    this->reqdbuf = 0;
	}
	strcpy(time_str, localtime_string(dtime()));
	if (port >= 0) {
	    fprintf (info, "%s - Final scan to ack all received packets\n", time_str);
	    fflush (info);
	}
	cs_scan (me, &alert);
	cs_off (me);
    }

    strcpy(time_str, localtime_string(dtime()));
    if (port >= 0) {
	fprintf (info, "%s - Terminated\n", time_str);
    }
    exit(error);
}

/************************************************************************
 *  setup_socket:
 *	Setup the socket appropriately.
 ************************************************************************/
int setup_socket (int port)
{
    struct sockaddr_in sin;
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    int ruflag;
    int sd;
    int ld;

    ld = 0;
    if ((ld = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
	fprintf (stderr, "Error creating the socket\n");
	terminate_program (1);
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = port;

    ruflag = 1;
    if (setsockopt (ld, SOL_SOCKET, SO_REUSEADDR, (char *)&ruflag, sizeof(ruflag))) {
	fprintf (stderr, "Error setting REUSEADDR socket option\n");
	terminate_program(1);
    }
    ruflag = 1;
    if (setsockopt (ld, SOL_SOCKET, SO_KEEPALIVE, (char *)&ruflag, sizeof(ruflag))) {
	fprintf (stderr, "Error setting KEEPALIVE socket option\n");
	terminate_program(1);
    }

    if (bind (ld, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	fprintf (stderr, "Error binding to socket\n");
	terminate_program (1);
    }

    listen (ld,1);

    if ((sd = accept (ld, (struct sockaddr *)&from, &from_len)) < 0) {
	fprintf (stderr, "Error accepting socket connection\n");
	terminate_program (1);
    }
    if (verbosity) {
	fprintf (stderr, "Accepting connection from: %s\n",
		 inet_ntoa(from.sin_addr));
	fflush (stderr);
    }
    return (sd);
}

/************************************************************************
 *  write_seed:
 *	Write Mini-SEED packet to the specified file descriptor.
 p************************************************************************/
int write_seed (int sd, seed_record_header *pseed, int blksize)
{
    int n;

    if ((n = xcwrite (sd, (char *)pseed, blksize)) != blksize) {
	fprintf (stderr, "Error writing seed record - returned %d\n", n);
	terminate_program (1);
    }
    if (verbosity && port >= 0) fprintf (info, "seed packet sent\n");
    fflush (info);
    return (blksize);
}

#define MAX_RETRIES 20
/************************************************************************
 *  xcwrite:
 *	Write output buffer.  Continue writing until all N bytes are
 *	written or until error.
 ************************************************************************/
int xcwrite (int fd, char buf[], int n)
{
    int nw;
    int left = n;
    int retries = 0;
    while (left > 0) {
	if ( (nw = write (fd, buf+(n-left), left)) <= 0 && errno != EINTR) {
	    fprintf (stderr, "error writing output, errno = %d\n", errno);
	    return (nw);
	}
	if (nw == -1) {
	    fprintf (stderr, "Interrupted write, retry %d.\n", retries);
	    ++retries;
	    if (retries > MAX_RETRIES) {
		fprintf (stderr, "Giving up...\n");
		return(n-left);
	    }
	    continue;
	}
	left -= nw;
    }
    return (n);
}

#define	TIMEOUT_SEC	0
#define	TIMEOUT_USEC	100

/************************************************************************
 *  check_socket:
 *	Check to see if socket is still open.
 *	Return -1 on error, 0 on OK.
 *
 *  Note:  Since we want a relatively short timeout on the read, we use
 *  setitimer() instead of alarm().  In addition, we use the interval
 *  timer to issue repeated SIGALRM signals until the read has either
 *  completed or been interrupted.  Experience has shown that a single
 *  alarm may go off before the read() is executed, and the read would
 *  hang indefinitely.
 ************************************************************************/
int check_socket (int sd)
{
    int n;
    int err;
    char junk[2];

    struct itimerval  value;

    /*  Set up timeout interval for setitimer and getitimer.		*/
    value.it_interval.tv_sec = TIMEOUT_SEC;
    value.it_interval.tv_usec = TIMEOUT_USEC;
    value.it_value.tv_sec = TIMEOUT_SEC;
    value.it_value.tv_usec = TIMEOUT_USEC;
	
    /*  Set up the timer just before the read.				*/
    errno =0;
    setitimer (ITIMER_REAL, &value, NULL);
    n = read (sd, junk, 1);
    err = errno;

    /*  Reset timeout interval for setitimer and getitimer.		*/
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = 0;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 0;
    setitimer (ITIMER_REAL, &value, NULL);

    /*	We either read something, timed out, or got an error.		*/
    errno = err;
    if ((errno != TIMEOUT && errno) || (n==0 && errno==0)) {
	fprintf (stderr, "Socket apparently closed, errno=%d read()=%d\n",
		 errno,n);
	return (-1);
    }
    return (0);
}
