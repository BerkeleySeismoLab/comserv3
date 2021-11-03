/************************************************************************
 *  comserv_subs.c
 * 
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>

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
#include <malloc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <qlib2.h>

#include "channel_info.h"
#include "datasock_codes.h"
#include "basics.h"

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#define	TIMESTRLEN	40
#define	MAX_BLKSIZE	512

#define DAT_INDEX	DATAQ
#define DET_INDEX	DETQ
#define	CAL_INDEX	CALQ
#define	CLK_INDEX	TIMQ
#define	LOG_INDEX	MSGQ
#define	BLK_INDEX	BLKQ

#define	boolean_value(str)  ((str[0] == 'y' || str[0] == 'Y') ? 1 : 0)

/*  Comserv variables.	*/
extern tclientname client_name;

short data_mask =		/* data mask for cs_setup.		*/
    (CSIM_DATA);
tstations_struc stations ;
typedef char char23[24] ;
typedef char char5[6] ;
char23 stats[11] = { "Good", "Enqueue Timeout", "Service Timeout", 
		       "Init Error",
                       "Attach Refused", "No Data", "Server Busy", 
		       "Invalid Command",
                       "Server Dead", "Server Changed", "Segment Error" } ;

typedef struct _sel {		/* Structure for storing selectors.	*/
    int nselectors;		/* Number of selectors.			*/
    seltype *selectors;		/* Ptr to selector array.		*/
} SEL;

typedef struct _filter {
    int flag;			/* 0 = ignore all, 1 = use selector list*/
    SEL sel;			/* Ptr to selector list.		*/
} FILTER;

static SEL sel[NUMQ];
static FILTER save [NUMQ];	/* save filtering info for each type.	*/

char lockfile[160];			/* Name of optional lock file.	*/
char pidfile[160];

int save_this_record (seed_record_header *pseed);
int parse_cfg (char *str1, char *str2);

/************************************************************************/

int save_selectors_v(const int , int, CHANNEL_INFO **);
int set_selectors (pclient_struc );
void boolean_mask (short *ps, short bit_mask, char *str);
void store_selectors(int type, SEL *psel, char *str);
int sel_match (char *location, char *channel, SEL *psel);
int selector_match (char *str1, char *str2);

/************************************************************************
 *  Externals variables and functions.
 ************************************************************************/
extern FILE *info;		/* Default FILE for messages.		*/
extern char *cmdname;		/* Program name.			*/
extern int verbosity;		/* Verbosity flag.			*/
extern int terminate_proc;	/* Terminate program flag;		*/
extern int skip_ack;		/* skip comserv ack of packets.		*/
extern int flush;		/* Default is no flush (blocking client).*/
extern int nchannel;		/* number of entries in channelv.	*/
extern CHANNEL_INFO **channelv;	/* list of channels.			*/

extern char* seednamestring (seed_name_type *, location_type *);

/*  Signal handler variables and functions.	    */
void finish_handler(int sig);
void terminate_program (int error);
extern int write_to_ring (seed_record_header *pseed);

/*  Comserv variables.		*/
pclient_struc me = NULL;

/************************************************************************
 *  terminate_program
 *	Terminate prog and return error code.  Clean up on the way out.
 ************************************************************************/
void terminate_program (int error) 
{
    pclient_station thist;
    char time_str[TIMESTRLEN];
    int j;
    boolean alert ;

    strcpy(time_str, localtime_string(dtime()));
    if (verbosity & 2)  {
	fprintf (info, "%s - Terminating program.\n", time_str);
	fflush (info);
    }

    /* Perform final cs_scan for 0 records to ack previous records.	*/
    /* Detach from all stations and delete my segment.			*/
    if (me != NULL) {
	for (j=0; j< me->maxstation; j++) {
	    thist = (pclient_station) ((long) me + me->offsets[0]) ;
	    thist->reqdbuf = 0;
	}
	/* Do NOT ack server if we had problem delivering last packets. */
	if (! skip_ack) {
	    strcpy(time_str, localtime_string(dtime()));
	    fprintf (info, "%s - Using Final scan to ack all received packets\n", 
		time_str);
	    fflush (info);
	    cs_scan (me, &alert);
	    fprintf (info, "%s - Using another Final scan to ack all received packets\n", 
		time_str);
	    cs_scan (me, &alert);
	}
	cs_off (me) ;
    }

    strcpy(time_str, localtime_string(dtime()));
    fprintf (info, "%s - Terminated\n", time_str);

    if (error == 1) {
	fprintf (stdout, "Exiting through signal handler\n");
    }
    exit(error);
}

char *selector_type_str[] = {"DAT", "DET", "CAL", "TIM", "MSG", "BLK"};
/************************************************************************
 *  save_selectors_v:
 *	Parse and save selectors for specific types of info.
 ************************************************************************/
int save_selectors_v(const int type, int nchannel, CHANNEL_INFO **channelv)
{
    char *token;
    seltype *selectors = NULL;
    int n = 0;
    int i;

    if (nchannel <= 0) return(FAILURE);
    sel[type].nselectors = 0;
    if (sel[type].selectors) free ((void *)sel[type].selectors);
    for (i=0; i<nchannel; i++) {
	token = channelv[i]->channel;
	if ((int)strlen(token) > 5) {
	    fprintf (info, "Error in selector list for %s\n",
		     selector_type_str[type]);
	    if (selectors) free ((void *)selectors);
	    return(FAILURE);
	}
	selectors = (selectors == NULL) ? (seltype *)malloc(sizeof(seltype)) : 
	    (seltype *)realloc ((void *)selectors, (n+1)*sizeof(seltype));
	if (selectors == NULL) {
	    fprintf (info, "Error allocating selector space for %s\n",
		     selector_type_str[type]);
	    return(FAILURE);
	}
	strcpy(selectors[n++],lead(5,'?',token));
    }
    sel[type].selectors = selectors;
    sel[type].nselectors = n;
    return(SUCCESS);
}

/************************************************************************
 *  set_selectors:
 *	Set selector values for the single station.
 *	Assume sufficient selectors have been reserved.
 ************************************************************************/
int set_selectors (pclient_struc me)
{
    pclient_station thist;
    pselarray psa ;
    int nsel = 1;
    int type, n, i;

    thist = (pclient_station) ((long) me + me->offsets[0]) ;
    psa = (pselarray) ((long) me + thist->seloffset) ;
	
    for (type=0; type<CHAN; type++) {
	n = sel[type].nselectors;
	if (nsel+n > thist->maxsel) {
	    fprintf (info, "Error: Require %d selectors, allocated %d\n",
		     nsel+n, thist->maxsel);
	    return(FAILURE);
	}
	if (n > 0) {
	    thist->sels[type].first = nsel;
	    thist->sels[type].last = nsel + n - 1;
	    memcpy ((void *)&((*psa)[nsel]), (void *)sel[type].selectors, n*sizeof(seltype));
	  fprintf (info, "selector type = %d, nselectors = %d\n", type, n);
	  for (i=0; i<n; i++) fprintf (info, "%s\n",sel[type].selectors[i]);
	}
	nsel += n;
    }
    return(SUCCESS);
}

/************************************************************************
 *  fill_from_comserv
 *	Fill ring with data received from comserv.
 ************************************************************************/
int fill_from_comserv (char *station) 
{
    pclient_station thist;
    short j, k;
    boolean alert ;
    pdata_user pdat ;
    seed_record_header *pseed ;
    int res = 0;
    int lockfd;				/* Lockfile file desciptor.	*/

    config_struc cfg;
    char str1[160], str2[160], station_dir[160];
    char station_desc[60], source[160];
    char filename[160];
    char time_str[TIMESTRLEN];
    int status;

    lockfile[0] = '\0';
    lockfd = -1;
    pidfile[0] = '\0';

/* open the stations list and look for that station */
    strcpy (filename, "/etc/stations.ini") ;
    memset(&cfg, 0, sizeof(cfg));
    if (open_cfg(&cfg, filename, station)) {
	fprintf (stderr,"Could not find station %s\n", station) ;
	return FAILURE;
//	exit(1);
    }

/* Try to find the station directory, source, and description */
    while (1) {
	read_cfg(&cfg, str1, str2) ;
	if (str1[0] == '\0') break ;
	if (strcmp(str1, "DIR") == 0) 
	  strcpy(station_dir, str2) ;
	else if (strcmp(str1, "SOURCE") == 0)
	  strcpy(source, str2) ;
	else if (strcmp(str1, "DESC") == 0) {
	    strncpy(station_desc, str2, 60) ;
	    station_desc[59] = '\0' ;
	    fprintf (info, "%s %s startup - %s\n", localtime_string(dtime()),
		     station, station_desc) ;
	}
    }
    close_cfg(&cfg) ;
     
    /* Look for client entry in station's station.ini file.		*/
    addslash (station_dir);
    strcpy (filename, station_dir);
    strcat (filename, "station.ini");
    memset(&cfg, 0, sizeof(cfg));
    if (open_cfg(&cfg, filename, client_name)) {
	fprintf (stderr, "Error: Could not find [%s] section in for server %s in station.ini file\n",
		 client_name, station);
	fflush (stderr);
	return FAILURE;
//	exit(1);
    }
          
    /* Read and parse client directives in the station's config file.	*/
    while (read_cfg(&cfg, str1, str2), (int)strlen(str1) > 0) {
	res = parse_cfg (str1, str2);
	if (res != SUCCESS) {
	    return res;
	}
    }
    close_cfg (&cfg);

    /* Open the lockfile for exclusive use if lockfile is specified.	*/
    /* This prevents more than one copy of the program running for	*/
    /* a single station.						*/
    if (strlen(lockfile) > 0) {
	if ((lockfd = open (lockfile, O_RDWR|O_CREAT,0644)) < 0) {
	    fprintf (info, "Unable to open lockfile: %s\n", lockfile);
	    return FAILURE;
//	    exit(1);
	}
	if ((status=lockf (lockfd, F_TLOCK, 0)) < 0) {
	    fprintf (info, "Unable to lock daemon lockfile: %s status=%d errno=%d\n", 
		     lockfile, status, errno);
	    close (lockfd);
	    return FAILURE;
//	    exit(1);
	}
    }

    if (strlen(pidfile) > 0) {
	FILE *fp;
	if ((fp = fopen (pidfile,"w")) == NULL) {
	    fprintf (info, "Unable to open pidfile: %s\n", pidfile);
	    return FAILURE;
//	    exit(1);
	}
	else {
	    fprintf (fp, "%d\n",(int)getpid());
	    fclose(fp);
	}
    }

    terminate_proc = 0;
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

/* Set up a condition handler for SIGPIPE, since a write to a	*/
/* close pipe/socket raises a alarm, not an error return code.	*/

    signal (SIGPIPE,finish_handler);

/* Generate an entry for all available stations */      

    cs_setup (&stations, client_name, station, TRUE, TRUE, 10, nchannel+1, data_mask, 6000) ;

/* Create my segment and attach to all stations */      
    me = cs_gen (&stations) ;

/* Set up special selectors. */
/*::
    save_selectors_v (DATAQ, nchannel, channelv);
    set_selectors (me);
::*/

/* Show beginning status of all stations */

    strcpy(time_str, localtime_string(dtime()));
    for (j = 0 ; j < me->maxstation ; j++) {
	thist = (pclient_station) ((long) me + me->offsets[j]) ;
	/* Blocking clients should use default of CSQ_FIRST. */
	/* They will only get unacked data. */
	/* All other clients may want to always use CSQ_LAST so that they  */
	/* don't get data sent to a previous instance of the client. */
	if (flush) thist->seqdbuf = CSQ_LAST;
	fprintf (info, "%s - [%s] Status=%s\n", 
	         time_str, sname_str_cs(thist->name), 
		 (char *)stats[thist->status]) ;
    }
    fflush (info);

/* Send a message every second to all stations. */
/* Sending an attach command is harmless */

    while (! terminate_proc) {
	skip_ack = 0;
	j = cs_scan (me, &alert) ;
	if (j != NOCLIENT) {
	    thist = (pclient_station) ((long) me + me->offsets[j]) ;
	    if (alert) {
		strcpy(time_str, localtime_string(dtime()));
		fprintf (info, "%s - New status on station %s is %s\n", time_str,
			 sname_str_cs(thist->name), (char *)stats[thist->status]) ;
		fflush (info);
	    }
	    if (thist->valdbuf) {
		pdat = (pdata_user) ((long) me + thist->dbufoffset) ;
		for (k = 0 ; k < thist->valdbuf ; k++) {
		    pseed = (seed_record_header*) &pdat->data_bytes ;
		    if (verbosity & 1) {
			fprintf (info, "[%s] <%2d> %s recvtime=%s ",
				 sname_str_cs(thist->name), k, 
			         (char *)seednamestring(&pseed->channel_id, &pseed->location_id), 
                                 (char *)localtime_string(pdat->reception_time)) ;
			printf("hdrtime=%s\n", time_string(pdat->header_time)) ;
			fflush (info);
		    }
		    if (save_this_record(pseed)) {
			res = write_to_ring(pseed);
			if (res < 0) {
			    printf("Error writing to ring\n");
			    /* Exit on output error. */
			    terminate_proc = 1;	    /* Set program terminate flag. */
			    skip_ack = 1;	    /* Do not ack this server's packets. */
			    break;
			}
		    }
		    pdat = (pdata_user) ((long) pdat + thist->dbufsize) ;
		}
	    }
	}
	else {
	    if (verbosity & 2) {
		fprintf (info, "sleeping...");
		fflush (info);
	    }
	    sleep (1) ; /* Bother the server once every second */
	    if (verbosity & 2)  {
		fprintf (info, "awake\n");
		fflush (info);
	    }
	}
    }
    return(SUCCESS);
}


/************************************************************************
 *  parse_cfg:
 *	Parse config directives in section for for this program.
 ************************************************************************/
int parse_cfg (char *str1, char *str2)
{
    char *p;

    if (strcmp(str1, "PROGRAM") == 0) {	/* program pathname	*/
    }
    else if (strcmp(str1, "PIDFILE") == 0) {	/* pid file for client	*/
	strcpy(pidfile,str2);
    }
    else if (strcmp(str1,"LOCKFILE")==0) {
	strcpy(lockfile,str2);
    }
    /* Set selector and/or data mask for each type of info.		*/
    /* Global selector sets default for data, detection, and cal.	*/
    else if (strcmp(str1, "SELECTOR") == 0) {	/* global selectors 	*/
	store_selectors(DATAQ, &sel[DATAQ],str2);
	store_selectors(DETQ, &sel[DETQ],str2);
	store_selectors(CALQ, &sel[CALQ],str2);
	store_selectors(TIMQ, &sel[TIMQ],str2);
	store_selectors(MSGQ, &sel[MSGQ],str2);
	store_selectors(BLKQ, &sel[BLKQ],str2);
    }
    /* Set data_mask and optional selectors for each type of info.	*/
    else if (strcmp(str1, "DATA_SELECTOR") == 0) {	/* data mask, selectors.*/
	boolean_mask (&data_mask, CSIM_DATA, str2);
	if ((p=strchr(str2,','))) store_selectors(DATAQ, &sel[DATAQ],++p);
    }
    else if (strcmp(str1, "DETECTION_SELECTOR") == 0) {	/* det mask, selectors.	*/
	boolean_mask (&data_mask, CSIM_EVENT, str2);
	if ((p=strchr(str2,','))) store_selectors(DETQ, &sel[DETQ],++p);
    }
    else if (strcmp(str1, "CALIBRATION_SELECTOR") == 0) {	/* cal mask, selectors.	*/
	boolean_mask (&data_mask, CSIM_CAL, str2);
	if ((p=strchr(str2,','))) store_selectors(CALQ, &sel[CALQ],++p);
    }
    else if (strcmp(str1, "TIMING_SELECTOR") == 0) {	/* clock (timing) mask.	*/
	boolean_mask (&data_mask, CSIM_TIMING, str2);
	if ((p=strchr(str2,','))) store_selectors(TIMQ, &sel[TIMQ],++p);
    }
    else if (strcmp(str1, "LOG_SELECTOR") == 0) {	/* log (msgs) mask.	*/
	boolean_mask (&data_mask, CSIM_MSG, str2);
	if ((p=strchr(str2,','))) store_selectors(MSGQ, &sel[MSGQ],++p);
    }
    else if (strcmp(str1, "BLK_SELECTOR") == 0) {	/* opaque blockettes mask.	*/
	boolean_mask (&data_mask, CSIM_BLK, str2);
	if ((p=strchr(str2,','))) store_selectors(BLKQ, &sel[BLKQ],++p);
    }

    /* Set filter selector and/or data flag for each type of info.	*/
    /* Global selector sets default for data, detection, and cal.	*/
    else if (strcmp(str1, "SAVE") == 0) {	/* global selectors 	*/
	save[DAT_INDEX].flag = TRUE;
	store_selectors(DATAQ, &save[DAT_INDEX].sel,str2);
	save[DET_INDEX].flag = TRUE;
	store_selectors(DETQ, &save[DET_INDEX].sel,str2);
	save[CAL_INDEX].flag = TRUE;
	store_selectors(CALQ, &save[CAL_INDEX].sel,str2);
	save[CLK_INDEX].flag = TRUE;
	store_selectors(TIMQ, &save[CLK_INDEX].sel,str2);
	save[LOG_INDEX].flag = TRUE;
	store_selectors(MSGQ, &save[LOG_INDEX].sel,str2);
	save[BLK_INDEX].flag = TRUE;
	store_selectors(BLKQ, &save[BLK_INDEX].sel,str2);
    }
    /* Set data_flag and optional selectors for each type of info.	*/
    else if (strcmp(str1, "DATA_SAVE") == 0) {	/* data flag, selectors.*/
	save[DAT_INDEX].flag = boolean_value(str2);
	if ((p=strchr(str2,','))) store_selectors(DATAQ, &save[DAT_INDEX].sel,++p);
    }
    else if (strcmp(str1, "DETECTION_SAVE") == 0) {	/* det flag, selectors.	*/
	save[DET_INDEX].flag = boolean_value(str2);
	if ((p=strchr(str2,','))) store_selectors(DETQ, &save[DET_INDEX].sel,++p);
    }
    else if (strcmp(str1, "CALIBRATION_SAVE") == 0) {	/* cal flag, selectors.	*/
	save[CAL_INDEX].flag = boolean_value(str2);
	if ((p=strchr(str2,','))) store_selectors(CALQ, &save[CAL_INDEX].sel,++p);
    }
    else if (strcmp(str1, "TIMING_SAVE") == 0) {	/* clock (timing) flag.	*/
	save[CLK_INDEX].flag = boolean_value(str2);
	if ((p=strchr(str2,','))) store_selectors(TIMQ, &save[CLK_INDEX].sel,++p);
    }
    else if (strcmp(str1, "LOG_SAVE") == 0) {		/* log (msgs) flag.	*/
	save[LOG_INDEX].flag = boolean_value(str2);
	if ((p=strchr(str2,','))) store_selectors(MSGQ, &save[LOG_INDEX].sel,++p);
    }
    else if (strcmp(str1, "BLK_SAVE") == 0) {		/* opaque blockette flag.*/
	save[BLK_INDEX].flag = boolean_value(str2);
	if ((p=strchr(str2,','))) store_selectors(BLKQ, &save[BLK_INDEX].sel,++p);
    }
    else {
	fprintf (stderr, "Unknown configuration directive: %s\n", str1);
	return FAILURE;
    }
    return SUCCESS;
}

/************************************************************************
 *  boolean_mask:
 *	Set or clear the bit_mask values in *ps based on Y/N string.
 ************************************************************************/
void boolean_mask (short *ps, short bit_mask, char *str)
{
    switch (str[0]) {
      case 'Y':
      case 'y':
	*ps |= bit_mask;
	break;
      case 'N':
      case 'n':
	*ps &= (! bit_mask);
    }
}
   
/************************************************************************
 *  store_selectors:
 *	Parse and store selectors for specific types of info.
 ************************************************************************/
void store_selectors(int type, SEL *psel, char *str)
{
    char *token;
    int i, l;
    seltype *selectors = NULL;
    char *p, *savep;
    int n = 0;

    if ((int)strlen(str) <= 0) return;
    savep = p = strdup(str);
    psel->nselectors = 0;
    if (psel->selectors) free (psel->selectors);
    while ((token = strtok(p,","))) {
	if ((int)strlen(token) > 5) {
	    fprintf (info, "Error in selector list for %s : %s\n",
		     selector_type_str[type], token);
	    if (selectors) free (selectors);
	    free (savep);
	    return;
	}
	for (i=1,l=strlen(token); i<l; i++) {
	    if (islower(token[i])) token[i] = toupper(token[i]);
	}
	selectors = (selectors == NULL) ? (seltype *)malloc(sizeof(seltype)) : 
	    (seltype *)realloc (selectors, (n+1)*sizeof(seltype));
	if (selectors == NULL) {
	    fprintf (info, "Error allocating selector space for %s\n",
		     selector_type_str[type]);
	    free (savep);
	    return;
	}
	strcpy(selectors[n++],lead(5,'?',token));
	p = NULL;
    }
    psel->selectors = selectors;
    psel->nselectors = n;
    free (savep);
    return;
}


/************************************************************************
 *  save_this_record:
 *	Determine whether this record should be written to the ring.
 *	Return TRUE or FALSE.
 ************************************************************************/
int save_this_record (seed_record_header *pseed)
{
    BS *bs;
    int itype = DAT_INDEX;
    int b1000 = 0;
    int b2000 = 0;
    int n = -1;
    DATA_HDR *hdr;
    int saveit = TRUE;

    hdr = decode_hdr_sdr((SDR_HDR *)pseed, MAX_BLKSIZE);
    if (hdr == NULL) return (FALSE);
    bs = hdr->pblockettes;

    /* Determine the type of packet based on the blockette types.   */
    /* Assume it is a data packet unless proven otherwise.	    */
    /* Check packets in this order:				    */
    /* 2xx => events		*/
    /* 3xx => calibration	*/
    /* 5xx => timing		*/
    /* sample rate = 0 => log	*/
    while (bs != (BS *)NULL) {
	n = bs->type;
	if (n >= 200 && n < 300) {
	    itype = DET_INDEX;
	    break;
	}
	else if (n >= 300 && n <= 400) {
	    itype = CAL_INDEX;
	    break;
	}
	else if (n >= 500 && n < 600) {
	    itype = CLK_INDEX;
	    break;
	}
	else if (n == 1000) {
	    /* Note that we found a blockette 1000, but keep scanning.	*/
	    b1000 = 1;
	}
	else if (n == 2000) {
	    /* Note that we found a blockette 2000, but keep scanning.	*/
	    b2000 = 1;
	}
	else {
	    /* Unknown or unimportant blockette - skip it */
	}
	bs = bs->next;
    }
    /* LOG channel is any channel still identified as a data channel	*/
    /* but with a sample rate of 0 and non-zero sample count.		*/
    if (itype == DAT_INDEX && hdr->sample_rate == 0 && hdr->num_samples != 0) {
	itype = LOG_INDEX;
    }
    if (itype == DAT_INDEX && hdr->sample_rate == 0 && hdr->num_samples == 0 && b2000) {
	itype = BLK_INDEX;
    }
    /* EVERY data packet should have a blockette 1000.			*/
    if (itype == DAT_INDEX && b1000 == 0) {
	fprintf (stderr, "Unknown blockette/packet type %d for %s.%s.%s.%s\n", n,
		 hdr->station_id, hdr->network_id,
		 hdr->channel_id, hdr->location_id);
	saveit = FALSE;
	/*:: dump_data_hdr(hdr); ::*/
    }
    /* Ignore empty end-of-detection packets.	    */
    if (saveit && (itype == DAT_INDEX && hdr->sample_rate == 0 && hdr->num_samples == 0)) {
	saveit = FALSE;
    }

    /* Check the filter to see if this channel and type should be saved	*/
    /* or discarded.							*/
    if (saveit && (save[itype].flag == FALSE)) {
	saveit = FALSE;
    }
    if (saveit && (itype <= CAL_INDEX && 
		   ! sel_match (hdr->location_id, hdr->channel_id, &save[itype].sel))) {
	saveit = FALSE;
    }
    free_data_hdr(hdr);
    return (saveit);
}

/************************************************************************
 *  sel_match:
 *	Attempt to match location and channel to a selector list.
 *	Return TRUE if a match is found, FALSE otherwise.
 ************************************************************************/
int sel_match (char *location, char *channel, SEL *psel)
{
    int i;
    char str[6];
    memcpy (str, location, 2);
    memcpy (str+2, channel, 3);
    str[5] = '\0';
    for (i=0; i<psel->nselectors; i++) {
	if (selector_match(str,psel->selectors[i])) return (TRUE);
    }
    return (FALSE);
}

/************************************************************************
 *  selector_match:
 *	Match a string to a selector.
 *	Return TRUE if match, FALSE otherwise.
 ************************************************************************/
int selector_match (char *str1, char *str2)
{
    int i;
    char a, b;
    for (i=0; i<5; i++) {
	if (str1[i] == '?' || str2[i] == '?') continue;
	a = (islower(str1[i])) ? toupper(str1[i]) : str1[i];
	b = (islower(str2[i])) ? toupper(str2[i]) : str2[i];
	if (a == b) continue;
	return (FALSE);
    }
    return (TRUE);
}

