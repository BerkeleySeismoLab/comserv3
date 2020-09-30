/************************************************************************
 * Program:	evtalarm
 *	Alarm notification program for detector messages.
 * Author:
 *	Douglas Neuhauser, UC Berkeley Seismographic Station, 
 *
 *	Adopted from a demo program by written by 
 *	Woodrow H. Owens for Quanterra, Inc.
 *	Copyright 1994 Quanterra, Inc.
 *	Copyright (c) 1998-2000 The Regents of the University of California.
 * 

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 14 JUL 95 DSN First created (from Quanterra demo program).
    2020.273 DSN Updated for comserv3.
		ver 1.1.0 Modified for 15 character station and client names.
 ************************************************************************/

#define	VERSION		"1.1.0 (2020.273)"
#define	CLIENT_NAME	"EVTALARM"
/* #define	CLIENT_NAME	"EVTA" */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <math.h>
#include <string.h>
#include <fnmatch.h>
#include <ctype.h>
#include <libgen.h>

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
#define	DEFAULT_DETECTION_TYPE	"t"

#define	EMAIL_CMD   "/usr/ucb/mail"
#define	PAGER_CMD   "pager"
#define	DEFAULT_ACTION	"email"

char *syntax[] = {
"%s version " VERSION " - read comserv detection msgs and notify user list.",
"%s   [-h] [-d n] [-v n] [-F] [-f datetime] [-t types]",
"	[-c channels] [-i stations] [-n notify_list] stations",
"    where:",
"	-h	    Help - prints syntax message.",
"	-d n	    Debug level n.  Output to stdout.",
"	-v n	    Set verbosity level to n.",
"	-F	    Do not flush old detections.",
"	-t types    String containing types of detections to process.",
"		    m = Murdock Hutt (201)",
"		    t = Threshold (200)",
"		    Default detection type is " DEFAULT_DETECTION_TYPE ".",
"	-f datetime Set start datetime for first detections.",
"	-c channels A comma-delimited list of channels to use.",
"		    If no channels are listed, all channels will be used.",
"		    Wildcard characters ('*' or '?') may be used if quoted.",
"	-i stations A comma-delimited list of stations to ignore.",
"	-n notify_list",
"		    A list of users and actions.",
"		    Valid actions are: email, pager.",
"		    Default action is " DEFAULT_ACTION ".",
"	stations    A comma-delimited list of stations to use.",
"		    'ALL' or '*' indicates all stations.",
"		    If no stations were specified, use all stations.",
NULL };

#define	UA_LIST	char *
#define	U_LIST	char *
#define	A_LIST	char *
#define	G_LIST	char *

/************************************************************************/
/*  Externals required in multiple routines.				*/
/************************************************************************/

char *cmdname;				/* command name.		*/
int verbosity;				/* verbosity flag.		*/
short data_mask = (CSIM_EVENT);		/* data mask for cs_setup.	*/
char *detection_types = "t";		/* default detection type.	*/
string15 sname = "*" ;			/* default station list.	*/
int flush = 1;				/* flag to flush old detections.*/
int start_flag = 0;			/* flag to set start time.	*/
double dstart = 0.0;			/* detection start time.	*/
INT_TIME start_time;			/* detection start time.	*/
static int terminate_proc;		/* terminate program flag.	*/
int debug;				/* debug value.			*/
UA_LIST *notify_list;			/* notification list.		*/

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
G_LIST *parse_glist (char *str, char **pp, char *delim);
UA_LIST *parse_nitem (char *str, char **pp);
UA_LIST *parse_notify_list (char *str);
UA_LIST *listcat (UA_LIST *l1, UA_LIST *l2);

pclient_struc me;

int parse_cmdline (int *pargc, char ***pargv);

/************************************************************************
 *  Main routine.
 ************************************************************************/
int main (int argc, char *argv[])
{
    pclient_station this_station;
    int j, k;
    boolean alert;
    pdata_user pdat;
    seed_record_header *pseed;
    char *msgbuf;
    char time_str[TIMESTRLEN];
    int status;

    cmdname = basename(strdup(argv[0]));
    get_my_wordorder();

    parse_cmdline (&argc, &argv);

    me = setup_comserv_connections(station_list, station_exclude, channel_list);

    /* Send a message every second to all stations. Sending an attach command is harmless */
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
			fprintf(info, "[%-s] <%2d> %s recvtime=%s ",
				sname_str_cs(this_station->name), k, 
				(char *)seednamestring(&pseed->channel_id, &pseed->location_id), 
				(char *)localtime_string(pdat->reception_time)) ;

			fprintf(info, "hdrtime=%s\n", time_string(pdat->header_time)) ;
			fflush (info);
		    }
		    msgbuf = print_evt(pseed, TEXT_FORMAT, detection_types);
		    status = output (msgbuf);
		    if (status < 0 && verbosity) {
			fprintf (info, "Error writing to notify list\n");
		    }
		    fflush (stdout);
		    fflush (stderr);
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

void upcase(char *str)
{
    unsigned char c;
    while ((c = *str)) *(str++) = islower(c) ? toupper(c) : c;
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
    }
    return;
}
    
/************************************************************************
 *  set_selectors:
 *	Set selector values for the single station.
 *	Assume sufficient selectors have been reserved.
 ************************************************************************/
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
	if (fnmatch(list->argv[i],station,0)==0) {
	    printf ("match: %s %s\n", list->argv[i],station);
	    result = 1;
	    break;
	}
    }
    return(result);
}

/************************************************************************
 *  print_evt:
 *	Print event event detection packet.
 ************************************************************************/
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
		    sprintf (msgbuf, "%-4.4s%c %s %s %4d %10.0f %10.0f %s",
			     hdr->station_id, 'Q', hdr->channel_id, date_time, type, 
			     ((BLOCKETTE_200 *)bh)->signal_amplitude,
			     ((BLOCKETTE_200 *)bh)->background_estimate,
			     detector_name);
		    msgbuf[80] = '\0';	    /* limit to 80 char line. */
		    break;
		case TEXT_FORMAT:
		    sprintf (msgbuf, "%-5s %-3s ", hdr->station_id, hdr->channel_id);
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
		    sprintf (msgbuf, "%-4.4s%c %s %s %3d %c%c%c%c%c%c%c%c %8.0f %6.3f %8.0f %s",
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
		    sprintf (msgbuf, "%-5s %-3s ", hdr->station_id, hdr->channel_id);
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

/************************************************************************
 *  parse_cmdline
 *	Parse command line.
 ************************************************************************/
int parse_cmdline (int *pargc, char ***pargv)
{
    INT_TIME	*pt;
    char *p;

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind, opterr;
    int		c;

    while ( (c = getopt((*pargc),(*pargv),"hv:i:c:n:f:t:a:d:F")) != -1)
	switch (c) {
	case '?':
	case 'h':	print_syntax(cmdname,syntax,info); exit(0); break;
	case 'v':	verbosity=atoi(optarg); break;
	case 'i':	make_list (&station_exclude,optarg,""); break;
	case 'c':	make_list (&channel_list,optarg,"??"); break;
	case 't':	detection_types = optarg; break;
	case 'F':	flush = 0; break;
	case 'd':	debug = atoi(optarg); break;
	case 'n':	notify_list = parse_notify_list(optarg); break;
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

    if (notify_list == NULL) {
	fprintf (stderr, "Error - missing notify list\n");
	exit(1);
    }

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
	fprintf (diag, "%s - [%s] Status=%s\n", 
		 time_str, 
		 sname_str_cs(this_station->name), 
		 (char *)&stats[this_station->status]) ;
	/* Set start time if specified.	*/
	if (start_flag) this_station->startdbuf = dstart;
	if (flush) this_station->seqdbuf = CSQ_LAST;
    }
    fflush (diag);
    return (me);
}

int output (char *msgbuf) 
{
    char namebuf[] = TMPFILE_PREFIX "evta" TMPFILE_SUFFIX;
    FILE *fp;
    UA_LIST *nl;
    char user[16],action[16];
    char cmd[128];
    char *p;
    int n, len, status;

    n = 0;
    if (msgbuf == NULL || msgbuf[0] == '\0') return (0);
    if ((fp = tmpfile_open(namebuf, "w")) == NULL) {
	fprintf (stderr, "Error opening msg file %s\n", namebuf);
	return (-1);
    }
    fwrite (msgbuf, 1, strlen(msgbuf), fp);
    fclose (fp);
    if (debug) {
	printf ("msg is: %s",msgbuf);
	printf ("file is: %s\n", namebuf);
    }

    for (nl=notify_list; *nl!=NULL; nl++) {
	if ((p = strchr(*nl,':')) == NULL) {
	    fprintf (stderr, "Error in notify entry: %s\n", *nl);
	    continue;
	}
	len = p-*nl;
	strncpy(user,*nl,len);
	user[len]= '\0';
	strcpy(action,++p);

	if (strcasecmp(action,"EMAIL")==0) {
	    sprintf (cmd, "%s %s < %s", EMAIL_CMD, user, namebuf);
	}
	else if (strcasecmp(action,"PAGER")==0) {
	    sprintf (cmd, "%s %s < %s", PAGER_CMD, user, namebuf);
	}
	else {
	    fprintf (stderr, "Unknown action for notify entry: %s\n", *nl);
	    continue;
	}
	if (debug) {
	    printf("%s\n", cmd);
	}
	else {
	    if ((status = system (cmd)) != 0) {
		fprintf (stderr, "Errod %d executing: %s\n", status, cmd);
		continue;
	    }
	}
	n++;
    }

    return (n);
}

void finish_handler(int sig)
{
    terminate_proc = 1;
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)	*/
}

/************************************************************************
 *	notify_list	::= nitem | nitem,notify_list
 *	nitem	::= ulist | ulist:alist
 *	ulist	::= user | (userlist)
 *	userlist	::= user | user,userlist
 *	user	::= [a-z0-9.-]+
 *	alist	::= action | (actionlist)
 *	actionlist	::= action | action,actionlist
 *	action	::= "email"|"pager"|"pager1"|"pager2"
 *	
 *	For each nitem, if alist is not specified, alist ::= "email".
 *	
 *	foreach nitem
 *		foreach user
 *			foreach action
 *				notify user by action
 ************************************************************************/

/************************************************************************
 *  parse_notify_list:
 *	Parse a complete notify list.
 *  return:
 *	ptr to ua_list.
 ************************************************************************/
UA_LIST *parse_notify_list (char *str)
{
    UA_LIST *plist;
    UA_LIST *nlist;
    char *p;
    plist = (UA_LIST *)NULL;
    nlist = (UA_LIST *)NULL;
    while (*str != '\0') {
	nlist = parse_nitem(str, &p);
	if (plist == NULL) plist = nlist;
	else {
	    plist = listcat (plist,nlist);
	    free(nlist);
	}
	str = p;
	if (*str == ',') ++str;
    }
    return (plist);
}

/************************************************************************
 *  parse_nitem:
 *	Parse a user:action list.
 *  return:
 *	ptr to ua_list.
 ************************************************************************/
UA_LIST *parse_nitem (char *str, char **pp)
{
    char *p;
    U_LIST *ulist;
    U_LIST *pu;
    A_LIST *alist;
    A_LIST  *pa;
    UA_LIST *ualist;
    UA_LIST s;
    int n, i, j;

    ualist = (UA_LIST *)malloc (sizeof(UA_LIST));
    ualist[0] = NULL;

    /* Parse user portion of list.	    */
    if (*str == '(') {
	ulist = parse_glist(++str,&p,")");
	if (*p != ')') {
	    fprintf (stderr, "error parsing notify list - expected ')' not found\n");
	    exit(1);
	}
	(p)++;
    }
    else {
	ulist = parse_glist(str,&p,",:");
    }

    /* Look for user:action list separator. */
    if (*p == ':') {
	str = ++p;
	/* Parse action portion of list */
	if (*str == '(') {
	    alist = parse_glist(++str,&p,")");
	    if (*p != ')') {
		fprintf (stderr, "error parsing notify list - expected ')' not found\n");
		exit(1);
	    }
	    (p)++;
	}
	else {
	    alist = parse_glist(str, &p, ",");
	}
    }
    else if (*p == ',' || *p == '\0') {
	alist = parse_glist (DEFAULT_ACTION, NULL, ",");
    }
    else {
	fprintf (stderr, "error parsing notify list - no end to action list.\n");
	exit(1);
    }

    /* Create cross product of user and action list.	*/
    n = 0;
    for (pu=ulist; *pu!=NULL; pu++) {
	for (pa=alist; *pa!=NULL; pa++) {
	    i = strlen(*pu);
	    j = strlen(*pa);
	    s = (UA_LIST)malloc((i+j+2)*sizeof(char));
	    strcpy (s,*pu);
	    strcat (s,":");
	    strcat (s,*pa);
	    ualist[n++] = (UA_LIST)s;
	    ualist = (UA_LIST *)realloc(ualist, (n+1)*sizeof(UA_LIST));
	}
    }
    /* Free users, actions, ulist, and alist.	*/
    ualist[n] = NULL;
    for (pu=ulist; *pu!=NULL; pu++) {
	free (*pu);
    }
    free (ulist);
    for (pa=alist; *pa!=NULL; pa++) {
	free(*pa);
    }
    free (alist);
    /* Return useraction list */
    if (pp != NULL) *pp = p;
    return (ualist);
}

/************************************************************************
 *  parse_glist:
 *	Parse generic list up to a specified delimiter OR end of string.
 *	List is split by commas (which may be in the delimeter string.)
 *  return:
 *	ptr to list.
 ************************************************************************/
G_LIST *parse_glist (char *str, char **pp, char *delim)
{
    G_LIST *list;
    char *p, *endp;
    char *s;
    int n = 0;
    int len;

    list = (G_LIST *)malloc (sizeof(UA_LIST));
    list[0] = NULL;
    endp = strpbrk(str,delim);
    if (endp == NULL) endp = str+strlen(str);
    while (str < endp) {
	p = strchr(str,',');
	if (p == NULL) p = str + strlen(str);
	if (p > endp) p = endp;
	len = p-str;
	s = (char*)malloc((len+1)*sizeof(char));
	strncpy (s,str,len);
	s[len] = '\0';
	list[n++] = (G_LIST)s;
	list = (G_LIST *)realloc(list, (n+1)*sizeof(UA_LIST));
	/* skip over string and the comma, if present.	*/
	str += len;
	if (p != endp) ++str;
    }
    list[n] = NULL;
    if (pp != NULL) *pp = endp;
    return (list);
}

/************************************************************************
 *  listcat:
 *	Concatenate 2 lists.  Return ptr to new list.
 ************************************************************************/
UA_LIST *listcat (UA_LIST *l1, UA_LIST *l2)
{
    int n1,n2;
    for (n1=0; l1[n1] != NULL; n1++) ;
    for (n2=0; l2[n2] != NULL; n2++) ;
    l1 = (UA_LIST *)realloc (l1, (n1+n2+1)*sizeof(UA_LIST));
    memcpy((void *)&l1[n1],(void *)l2,n2*sizeof(UA_LIST));
    l1[n1+n2] = NULL;
    return (l1);
}
