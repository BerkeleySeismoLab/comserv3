/*   cpick_card_client
     Copyright 1994 Quanterra, Inc.
     Copyright (c) 1995 The Regents of the University of California.
     Based on a client written by Woodrow H. Owens
     Written by Douglas Neuhauser, UC Berkeley Seismographic Station

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 11 Mar 94 WHO First created.
    1 30 Mar 94 WHO Uses new service calls.
    2 15 Feb 96 DSN Update for comserv 1.0.
    3 2020-09-29 DSN Updated for comserv3.
		Ver 1.0.1 Modified for 15 character station and client names.
*/

#define	VERSION		"1.0.1 (2020.273)"
#define	CLIENT_NAME	"EVENT_DETECTOR"
/* #define	CLIENT_NAME	"EVTD" */

#include <stdio.h>

char *syntax[] = {
"%s - version " VERSION,
"%s - read comserv detection msgs and output them in printable format.",
"%s   [-h] [-d n] [-v n] [-F] [-f datetime] [-t types] [-o format]",
"	[-c channels] [-i stations] stations",
"    where:",
"	-h	    Help - prints syntax message.",
"	-d n	    Debug level n.  Output to stdout.",
"	-v n	    Set verbosity level to n.",
"	-F	    Do not flush old detections.",
"	-t types    Types of detections to process.  Default is m.",
"		    m = Murdock Hutt (201)",
"		    t = Threshold (200)",
"		    Default detection type is m.",
"	-o format   Output format specifier.  Default is t.",
"		    t = normal text.",
"		    c = Card associator format",
"	-f datetime Set start datetime for first detections.",
"	-c channels A comma-delimited list of channels to use.",
"		    If no channels are listed, all channels will be used.",
"		    Wildcard characters ('*' or '?') may be used if quoted.",
"	-i stations A comma-delimited list of stations to ignore.",
"	stations    A comma-delimited list of stations to use.",
"		    'ALL' or '*' indicates all stations.",
"		    If no stations were specified, use all stations.",
NULL };

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>

#include "qlib2.h"

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"
pchar seednamestring (seed_name_type *sd, location_type *loc);

#define	CS_BLKSIZE  512
#define	TIMESTRLEN  40
#define	STATIONS_FILE	"/etc/stations.ini"
#define	info		stdout
#define	diag		stderr
#define TEXT_FORMAT    0
#define	CARD_FORMAT   1
#define DEFAULT_FORMAT		TEXT_FORMAT
#define	DEFAULT_DETECTION_TYPE	"m"

/************************************************************************/
/*  Externals required in multiple routines.				*/
/************************************************************************/

#define	ALIVE_MSG   "alive"
char alive_msg[80];			/* alive message.		*/
char *cmdname;				/* command name.		*/
int alive_interval = 0;			/* alive interval in seconds.	*/
int verbosity;				/* verbosity flag.		*/
short data_mask = (CSIM_EVENT);		/* data mask for cs_setup.	*/
char *detection_types = "m";		/* default detection type.	*/
string15 sname = "*" ;			/* default station list.	*/
int output_format = DEFAULT_FORMAT;	/* default output format.	*/
int flush = 1;				/* flag to flush old detections*/
int start_flag = 0;			/* flag to set start time.	*/
double dstart = 0.0;			/* detection start time.	*/
INT_TIME start_time;			/* detection start time.	*/
static int terminate_proc;		/* terminate program flag.	*/
int debug;				/* debug value.			*/

typedef char char23[24] ;
char23 stats[11] = { "Good", "Enqueue Timeout", "Service Timeout", "Init Error",
                       "Attach Refused", "No Data", "Server Busy", "Invalid Command",
                       "Server Dead", "Server Changed", "Segment Error" } ;

typedef struct _LIST {
    int argc;
    char **argv;
} LIST;
LIST station_list;			/* list of stations.		*/
LIST station_exclude;			/* list of stations to exclude.	*/
LIST channel_list;			/* list of channels.		*/


void finish_handler(int sig);
pclient_struc setup_comserv_connections(LIST station_list, LIST station_exclude, LIST channel_list);
char *print_evt (seed_record_header *pseed, int output_format, char *types);
int output (char *msgbuf);
int parse_cmdline (int *pargc, char ***pargv);

pclient_struc me;

int syserr(char *msg);
void fatalsyserr(char *msg);

/************************************************************************
 *  main program.
 ************************************************************************/
int main (int argc, char *argv[])
{
    pclient_station this_station;
    int j, k;
    boolean alert;
    pdata_user pdat;
    seed_record_header *pseed;
    double last_alive, now;
    char *msgbuf;

    char time_str[TIMESTRLEN];

    cmdname = argv[0];
    get_my_wordorder();

    parse_cmdline (&argc, &argv);

    me = setup_comserv_connections(station_list, station_exclude, channel_list);

    if (alive_interval) {
	sprintf (alive_msg,"%s\n",ALIVE_MSG);
    }

    /* Send a message every second to all stations. Sending an attach command is harmless */
    last_alive = dtime();
    while (! terminate_proc) {
	j = cs_scan (me, &alert) ;
	if (j != NOCLIENT) {
	    this_station = (pclient_station) ((intptr_t) me + me->offsets[j]) ;
	    if (alert) {
		strcpy(time_str, localtime_string(dtime()));
		fprintf(diag, "%s - New status on station %s is %s\n", time_str,
			sname_str_cs(this_station->name), (char *)stats[this_station->status]) ;
		fflush (diag);
	    }
	    if (this_station->valdbuf) {
		pdat = (pdata_user) ((intptr_t) me + this_station->dbufoffset) ;
		for (k = 0 ; k < this_station->valdbuf ; k++) {
		    pseed = (pvoid) &pdat->data_bytes ;
		    if (verbosity & 1) {
			fprintf(info, "[%s] <%2d> %s recvtime=%s ",
				(char *)cname_str_cs(this_station->name), k, 
				(char *)seednamestring(&pseed->channel_id, &pseed->location_id), 
				(char *)localtime_string(pdat->reception_time)) ;
			fprintf(info, "hdrtime=%s\n", time_string(pdat->header_time)) ;
			fflush (info);
		    }
		    msgbuf = print_evt(pseed, output_format, detection_types);
		    if (output (msgbuf) < 0) break;
		    pdat = (pdata_user) ((intptr_t) pdat + this_station->dbufsize) ;
		}
	    }
	}
	else {
	    if (verbosity & 2) {
		printf ("sleeping...");
		fflush (stdout);
	    }
	    sleep (1) ; /* Bother the server once every second */
	    if (verbosity & 2) {
		printf ("awake\n");
		fflush (stdout);
	    }
	}
	now = dtime();
	if (alive_interval && (now - last_alive >= alive_interval)) {
	    if (output (alive_msg) < 0) break;
	    last_alive = now;
	}
    }
    strcpy(time_str, localtime_string(dtime()));
    if (verbosity & 2) {
	fprintf (diag, "%s - Terminating due to signal\n", time_str);
	fflush (diag);
    }

/* Perform a final cs_scan with 0 records requested to ack last records.    */
    for (j=0; j< me->maxstation; j++) {
	this_station = (pclient_station) ((intptr_t) me + me->offsets[0]) ;
	this_station->reqdbuf = 0;
    }
    if (1 /*::verbosity & 2::*/) {
	strcpy(time_str, localtime_string(dtime()));
	fprintf (diag, "%s - Final scan to ack all received packets\n", time_str);
	fflush (diag);
    }
    cs_scan (me, &alert);

/* Detach from all stations and delete my segment */
      cs_off (me) ;
      strcpy(time_str, localtime_string(dtime()));
      printf ("%s - Terminated\n", time_str);
      return(0);
}

void upcase(str)
    char *str;
{
    unsigned char c;
    while ((c = *str))
	*(str++) = islower(c) ? toupper(c) : c;
}

void make_list (LIST *p_hdr, char *string, char *prefix)
{
    char *s, *t;
    int n;
    s = string;
    p_hdr->argc = 0;
    p_hdr->argv = NULL;
    while ((t = strtok(s, ","))) {
	s = NULL;
	n = (p_hdr->argc)++;
	if (n == 0) 
	    p_hdr->argv = (char **)malloc((n+1)*sizeof(char *));
	else
	    p_hdr->argv = (char **)realloc(p_hdr->argv,(n+1)*sizeof(char *));
	p_hdr->argv[n] = (char *)malloc((strlen(t)+strlen(prefix)+1)*sizeof(char));
	strcpy(p_hdr->argv[n],prefix);
	strcat(p_hdr->argv[n],t);
	upcase(p_hdr->argv[n]);
    };
}
    
/************************************************************************/
/*  set_selectors:							*/
/*	Set selector values for the single station.			*/
/*	Assume sufficient selectors have been reserved.			*/
/************************************************************************/
void set_selectors (pclient_struc me)
{
    pclient_station this_station ;
    seltype *psel ;
    int nsel = 1;
    int n, i, k;
    int maxstation;

    /* Install a set of selectors to be used for event detections for	*/
    /* all stations.							*/

    n = channel_list.argc;
    maxstation = me->maxstation;
    if (maxstation < 0) {
	fprintf (stderr, "maxstation = %d\n", maxstation);
	return;
    }

    /* For each station, set selector range to point to new selectors.	*/
    for (k=0; k<maxstation; k++) {
	this_station = (pclient_station) ((intptr_t) me + me->offsets[k]) ;
	psel = (pvoid) ((intptr_t) me + this_station->seloffset) ;
	/* Install common selector list for all stations.			*/
	if (n+nsel > this_station->maxsel) {
	    fprintf (diag, "Error: Require %d selectors, allocated %d\n",
		     n+nsel, this_station->maxsel);
	    return;
	}
	for (i=0; i<n; i++) {
	    strcpy(psel[i+nsel],channel_list.argv[i]);
	}

	this_station->sels[DETQ].first = nsel;
	this_station->sels[DETQ].last = nsel + n - 1;
    }
}

int station_match (char *station, LIST *list)
{
    int i;
    int result = 0;
    if (list == NULL)
	return (result);
    for (i=0; i<list->argc; i++) {
	if (fnmatch(list->argv[i],station,0)) {
	    printf ("match: %s %s\n", list->argv[i],station);
	    result = 1;
	    break;
	}
    }
    return(result);
}

/************************************************************************/
/*  print_evt:								*/
/*	Print event event detection packet.				    */
/************************************************************************/
char *print_evt (seed_record_header *pseed, int output_format, char *types)
{
    DATA_HDR *hdr;
    char detector_name[25];
    char date_time[40];
    INT_TIME it;
    static char msgbuf[512];

    hdr = decode_hdr_sdr((SDR_HDR *)pseed, CS_BLKSIZE);
    if (hdr == NULL) {
	fprintf (stderr, "Unable to decode hdr\n");
	return ("Error in SEED header\n");
    }
    /* For some reason, decode_hdr_sdr does not trim all of the strings	*/
    /* in the hdr.  We will do it here, so that comparisons are always	*/
    /* done on trimmed strings.						*/
    trim (hdr->station_id);
    trim (hdr->channel_id);
    trim (hdr->location_id);
    trim (hdr->network_id);
    msgbuf[0] = '\0';

    if (hdr->pblockettes != NULL) {
	int type;
	BS *bs = hdr->pblockettes;
	do {
	    BLOCKETTE_HDR *bh = (BLOCKETTE_HDR *)(bs->pb);
	    switch (type=bh->type) {
	      case 200:	
		/* Generic (Threshold) event detector blockette.	*/
		if (strchr(types,'t') == NULL && strchr(types,'g') == NULL) break;
		strncpy(detector_name,((BLOCKETTE_200 *)bh)->detector_name,24);
		detector_name[24] = '\0';
		trim(detector_name);
		it = decode_time_sdr(((BLOCKETTE_200 *)bh)->time,my_wordorder);
		switch (output_format) {
		  case CARD_FORMAT:
		    strcpy(date_time, time_to_str(it, 3));
		    date_time[4] = date_time[7] = '/';
		    sprintf (msgbuf, "%s %c %s %s %4d %10.0f %10.0f %s",
			    hdr->station_id, 'Q', hdr->channel_id, date_time, type, 
			    ((BLOCKETTE_200 *)bh)->signal_amplitude,
			    ((BLOCKETTE_200 *)bh)->background_estimate,
			    detector_name);
		    msgbuf[80] = '\0';	    /* limit to 80 char line. */
		    break;
		    case TEXT_FORMAT:
			sprintf (msgbuf, "%s %s ", hdr->station_id, hdr->channel_id);
			sprintf (msgbuf+strlen(msgbuf), "%s %s detection_type= %d ",
				time_to_str(it, 0), "(evt)", 200);
			sprintf (msgbuf+strlen(msgbuf), "amp= %.0f limit= %.0f name= %s\n",
				((BLOCKETTE_200 *)bh)->signal_amplitude,
				((BLOCKETTE_200 *)bh)->background_estimate,
				detector_name);
/*
			if (print_delay) {
			    tdif = tdiff (cur_time, it);
			    sprintf (msgbuf+strlen(msgbuf), " (delay=%.1f sec)", tdif/USECS_PER_SEC);
			}
*/
			sprintf (msgbuf+strlen(msgbuf),"\n");
			break;
		}
		break;
	      case 201:	
		/* Murdock-Hutt evente detector blockette.	*/
		if (strchr(types,'m') == NULL) break;
		strncpy(detector_name,((BLOCKETTE_201 *)bh)->detector_name,24);
		detector_name[24] = '\0';
		trim(detector_name);
		it = decode_time_sdr(((BLOCKETTE_201 *)bh)->time,my_wordorder);
		switch (output_format) {
		  case CARD_FORMAT:
		    strcpy(date_time, time_to_str(it, 3));
		    date_time[4] = date_time[7] = '/';
		    sprintf (msgbuf, "%s %c %s %s %3d %c%c%c%c%c%c%c%c %8.0f %6.3f %8.0f %s",
			    hdr->station_id, 'Q', hdr->channel_id, date_time, type, 
			    (((BLOCKETTE_201 *)bh)->detection_flags & 0x01) + 'c',
			    (((BLOCKETTE_201 *)bh)->pick_algorithm) + 'a',
			    (((BLOCKETTE_201 *)bh)->loopback_value) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[0]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[1]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[2]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[3]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[4]) + '0',
			    ((BLOCKETTE_201 *)bh)->signal_amplitude,
			    ((BLOCKETTE_201 *)bh)->signal_period,
			    ((BLOCKETTE_201 *)bh)->background_estimate,
			    detector_name);
		    msgbuf[80] = '\0';	    /* limit to 80 char line. */
		    break;
		  case TEXT_FORMAT:
		    sprintf (msgbuf, "%s %s ", hdr->station_id, hdr->channel_id);
		    sprintf (msgbuf+strlen(msgbuf), "%s %s detection_type= %d ",
			    time_to_str(it, 0), "(evt)", 201);
		    sprintf (msgbuf+strlen(msgbuf), "motion_quality= %c%c%c%c%c%c%c%c ",
			    (((BLOCKETTE_201 *)bh)->detection_flags & 0x01) ? 'd' : 'c',
			    (((BLOCKETTE_201 *)bh)->pick_algorithm) + 'a',
			    (((BLOCKETTE_201 *)bh)->loopback_value) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[0]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[1]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[2]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[3]) + '0',
			    (((BLOCKETTE_201 *)bh)->signal_to_noise[4]) + '0');
		    sprintf (msgbuf+strlen(msgbuf), "peak_amp= %.0f period*100= %.0f background_amp= %.0f name= %s",
			    ((BLOCKETTE_201 *)bh)->signal_amplitude,
			    ((BLOCKETTE_201 *)bh)->signal_period,
			    ((BLOCKETTE_201 *)bh)->background_estimate,
			    detector_name);
		    break;
		}
		/*::
		if (print_delay) {
		    tdif = tdiff (cur_time, it);
		    sprintf (msgbuf+strlen(msgbuf), " (delay=%.1f sec)", tdif/USECS_PER_SEC);
		}
		::*/
		sprintf (msgbuf+strlen(msgbuf),"\n");
		break;
	      case 300:	
	      case 1001: 
		/*::
		sprintf (msgbuf, "blockette %d, clock_quality = %d, usec99 = %d, frame_count=%d\n",
			 type, 
			 ((BLOCKETTE_1001 *)bh)->clock_quality, 
			 ((BLOCKETTE_1001 *)bh)->usec99, 
			 ((BLOCKETTE_1001 *)bh)->frame_count );
		::*/
		break;
	      default:
		/* We are not interested in any other blockette.	*/
		break;
	    }
	    bs = bs->next;
	} while (bs != (BS *)NULL);
    }
    free_data_hdr (hdr);
    return (msgbuf);
}

/************************************************************************/
/*  parse_cmdline							*/
/*	Parse command line.						*/
/************************************************************************/
int parse_cmdline (int *pargc, char ***pargv)
{
    INT_TIME	*pt;
    char *p;

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind;
    int		c;

    while ( (c = getopt((*pargc),(*pargv),"hv:i:c:o:f:t:a:d:F")) != -1)
	switch (c) {
	  case '?':
	  case 'h':	print_syntax(cmdname,syntax,info); exit(0); break;
	  case 'v':	verbosity=atoi(optarg); break;
	  case 'i':	make_list (&station_exclude,optarg,""); break;
	  case 'c':	make_list (&channel_list,optarg,"??"); break;
	  case 't':	detection_types = optarg; break;
	  case 'F':	flush = 0; break;
	  case 'a':	alive_interval = atoi(optarg); break;
	  case 'd':	debug = atoi(optarg); break;
	  case 'o':	
	    switch (optarg[0]) {
	      case 't':   output_format = TEXT_FORMAT; break;
	      case 'c':   output_format = CARD_FORMAT; break;
	      default:    
		fprintf (stderr, "unknown format: %s\n", optarg);
		exit(1);
	    }
	    break;
	  case 'f':	
	    if ((pt = parse_date (optarg)) == NULL) {
		fprintf (diag, "invalid date: %s\n", optarg);
		exit(1);
	    }
	    start_time = *pt;
	    dstart = unix_time_from_int_time(start_time);
	    ++start_flag;
	    flush = 0;
	    break;
	}

    /*	Skip over all options and their arguments.			*/
    (*pargv) = &((*pargv)[optind]);
    (*pargc) -= optind;

    for (p=detection_types; *p; p++) {
	if (isupper(*p)) *p = tolower(*p);
    }

    /* Collect config on all specified stations.			*/
    /* If no stations were specified, assume ALL stations.		*/
    if ((*pargc) <= 0) 
	make_list(&station_list,"*","");
    else
	make_list(&station_list,(*pargv)[0],"");

    /* If no channels were specified, assume ALL channels.		*/
    if (channel_list.argc == 0) 
	make_list (&channel_list,"???","??");

    return (0);
}

pclient_struc setup_comserv_connections(LIST station_list, LIST station_exclude, LIST channel_list)
{
    pclient_struc me;
    pclient_station this_station;
    tstations_struc stations; 
    string15 station_name;
    char time_str[TIMESTRLEN];
    int i, j;
    char *p;

    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

    /* Generate an entry for all available stations */      
    cs_setup (&stations, CLIENT_NAME, "*", TRUE, FALSE, 10, 
	      1+channel_list.argc, data_mask, 6000);

    /* Loop through the stations, removing any that either do not	*/
    /*  match the include list, or that are on the exclude list.	*/
    for (i=0; i<stations.station_count; ) {
	p = sname_str_cs(stations.station_list[i].stationname);
	strcpy((char *)station_name,p);
	if (! station_match(station_name,&station_list))
	    cs_remove(&stations,i);
	else if (station_match(station_name,&station_exclude))
	    cs_remove(&stations,i);
	else i++;
    }

    terminate_proc = 0;
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

    /* Create my segment and attach to all stations */      
    me = cs_gen (&stations) ;

    /* Set up special selectors. */
    set_selectors (me);

    /* Show beginning status of all stations */
    strcpy(time_str, localtime_string(dtime()));
    for (j = 0; j < me->maxstation; j++) {
	this_station = (pclient_station) ((intptr_t) me + me->offsets[j]) ;
	fprintf (diag, "%s - [%s] Status=%s\n", time_str, sname_str_cs(this_station->name), 
		 (char *)stats[this_station->status]) ;
	/* Set start time if specified.	*/
	if (start_flag) this_station->startdbuf = dstart;
	if (flush) this_station->seqdbuf = CSQ_LAST;
    }
    fflush (diag);
    return (me);
}

int output (char *msgbuf) 
{
    if (msgbuf == NULL || msgbuf[0] == '\0') return (0);
    if (debug) {
	printf ("%s",msgbuf);
	return (strlen(msgbuf));
    }
    while ( send(0,msgbuf,strlen(msgbuf),0) != strlen(msgbuf) ) {
	if (errno == EINTR) continue;
	syserr("Error writing to socket.\n");
	terminate_proc = 1;
	return (-1);
    }
    return (strlen(msgbuf));
}

void finish_handler(int sig)
{
    terminate_proc = 1;
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)	*/
}

