/************************************************************************
 *  Datalog - log packets from comserv to disk files.
 *
 *  Douglas Neuhauser, UC Berkeley Seismological Laboratory
 *	Copyright (c) 1995-2021
 *	The Regents of the University of California.
 *
 *  Based on sample client program written by:
 *	Woodrow H. Owens, Quanterra, Inc.
 *	Copyright 1994 Quanterra, Inc.
 ************************************************************************/

/* Modification History:
------------------------------------------------------------------------
    2020.273 DSN 2020-09-29 DSN Updated for comserv3.
	ver 1.6.0 Modified for 15 character station and client names.
    2021-04-27 DSN ver 1.6.1 (2021.117)
    	Initialize config_struc structure before open_cfg call.
    2021-01-20 DSN ver 1.6.2 (2022.020)
	Added optional debugging info for packets written to disk.
    2022-02-29 DSN ver 1.6.3 (2022.059)
	Allow environment override of STATIONS_INI pathname.
*/

#define	VERSION		"1.6.3 (2022.038)"

#ifdef COMSERV2
#define	CLIENT_NAME	"DLOG"
#else
#define	CLIENT_NAME	"DATALOG"
#endif

#include <stdio.h>

char *syntax[] = {
"%s version " VERSION,
"%s    [-v n] [-c configfile] [-n name] [-h] station",
"    where:",
"	-v n	    Verbosity setting",
"		    1 = print mSEED header for each packet written to disk.",
"		    2 = print receipt line for each packet and display polling info.",
"	-c configfile",
"		    Specify an alternative datalog config file.",
"		    Default config file is the station config file.",
"	-n name",
"		    Specify an alternative comserv client name.",
"		    Default client name is " CLIENT_NAME ".",
"	-h	    Print brief help message for syntax.",
"	station	    Name of comserv station.",
NULL };

#include <stdint.h>
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
#include <libgen.h>

double log2(double);

#include "qlib2.h"

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

pchar seednamestring (seed_name_type *sd, location_type *loc);

#include "datalog.h"
#include "datalog_utils.h"

#define	TIMESTRLEN  	40
#define	CFGSTRLEN	CFGWIDTH
#define	DEFAULT_DATA_MASK \
	(CSIM_DATA | CSIM_EVENT | CSIM_CAL | CSIM_TIMING | CSIM_MSG | CSIM_BLK)
#define	MAX_SELECTORS	CHAN+2

#define ANNOUNCE(cmd,fp)							\
    ( fprintf (fp, "%s - Using STATIONS_INI=%s NETWORK_INI=%s\n", \
	      cmd, get_stations_ini_pathname(), get_network_ini_pathname()) )

/************************************************************************
 *  Externals required in multiple files or routines.
 ************************************************************************/
DURLIST durhead[7];			/* Headers for duration lists.	*/
char *extension[7];			/* Extention char for pkt types.*/
char pidfile[1024];			/* pid file.			*/
char gapfile[1024];			/* pid file.			*/
int gapfd=-1;
short data_mask = DEFAULT_DATA_MASK;	/* data mask for cs_setup.	*/

char *cmdname;				/* Name of this program.	*/
tservername station;			/* station name.		*/
tclientname client_name = CLIENT_NAME;	/* client name			*/
char *default_duration = "1d";		/* default duration of file.	*/
char lockfile[160];			/* Name of optional lock file.	*/
int lockfd;				/* Lockfile file desciptor.	*/
int verbosity;				/* verbosity setting.		*/

static int terminate_proc;		/* flag to terminate program.	*/
static pclient_struc me = NULL;		/* comserv shared mem ptr.	*/
typedef char char23[24];
static char23 stats[11] =
    {	"Good", "Enqueue Timeout", "Service Timeout", "Init Error",
	"Attach Refused", "No Data", "Server Busy", "Invalid Command",
	"Server Dead", "Server Changed", "Segment Error" };

/************************************************************************
 *  Function declarations.
 ************************************************************************/
void finish_handler(int sig);
int terminate_program (int error);

/************************************************************************
 *  main program.
 ************************************************************************/
int main (int argc, char **argv)
{
    tstations_struc tstations;
    pclient_station this;
    short j, k ;
    int ret, status;
    boolean alert;
    pdata_user pdat;
    seed_record_header *pseed;

    config_struc cfg;
    char str1[CFGSTRLEN], str2[CFGSTRLEN];
    char station_dir[CFGSTRLEN], station_desc[CFGSTRLEN], source[CFGSTRLEN];
    char configfile[CFGSTRLEN];
    char time_str[TIMESTRLEN];
    int i;

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind, opterr;
    int		c;

    cmdname = basename(strdup(argv[0]));
    configfile[0] = '\0';

    init_qlib2 (1);
    get_my_wordorder();

    while ( (c = getopt(argc,argv,"hv:c:n:")) != -1)
	switch (c) {
	case '?':
        case 'h':   ANNOUNCE(cmdname,info); print_syntax(cmdname,syntax,info); exit(0);
	case 'v':   verbosity=atoi(optarg); break;
	case 'c':   
	    strncpy(configfile,optarg,CFGSTRLEN); 
	    configfile[CFGSTRLEN-1] = '\0'; 
	    break;
	case 'n':   
	    strncpy(client_name,optarg,CLIENT_NAME_SIZE);
	    client_name[CLIENT_NAME_SIZE-1] = '\0';
	}

    /*	Skip over all options and their arguments.			*/
    argv = &(argv[optind]);
    argc -= optind;

    /* Allow override of station name on command line */
    if (argc > 0) {
	strncpy(station, argv[0], SERVER_NAME_SIZE);
	station[SERVER_NAME_SIZE-1] = '\0';
    }
    else {
	fprintf (info, "%s Error: Missing station name\n", 
		 localtime_string(dtime()));
	exit(1);
    }
    upshift(station);

    ANNOUNCE(cmdname,info);
    if (configfile[0] == '\0') {
	/* Look for station entry in master station file.		*/
	strncpy(configfile, get_stations_ini_pathname(), CFGSTRLEN);
	configfile[CFGSTRLEN-1] = '\0';
	memset(&cfg, 0, sizeof(cfg));
	if (open_cfg(&cfg, configfile, station)) {
	    fprintf (info,"%s Error: Could not find station\n",
		     localtime_string(dtime()));
	    exit(1);
	}

	/* Look for station directory, source, and description.		*/
	while (1) {
	    read_cfg(&cfg, str1, str2);
	    if (str1[0] == '\0') break;
	    if (strcmp(str1, "DIR") == 0)
		strcpy(station_dir, str2);
	    else if (strcmp(str1, "SOURCE") == 0)
		strcpy(source, str2);
	    else if (strcmp(str1, "DESC") == 0) {
		strncpy(station_desc, str2, 60);
		station_desc[59] = '\0';
	    }
	}
	close_cfg(&cfg);
      
	/* Look for client entry in station's station.ini file.		*/
	addslash (station_dir);
	strcpy (configfile, station_dir);
	strcat (configfile, "station.ini");
    }
    else {
	station_desc[0] = '\0';
    }
    printf ("%s %s startup - %s\n", localtime_string(dtime()),
	    station, station_desc);

    /* Open and read station config file.				*/
    memset(&cfg, 0, sizeof(cfg));
    if (open_cfg(&cfg, configfile, client_name)) {
	fprintf (info, "%s Could not find station.ini file\n",
		 localtime_string(dtime()));
	exit(1);
    }
          
    /* Set up any defaults for this program that can be changed by 	*/
    /* directives in the station's config file.				*/
    for (i=0; i<6; i++) {
	append_durlist(&durhead[i], "???", default_duration);
        extension[i] = (char *)malloc(MAXLEN_X+1);
	if (extension[i] == NULL) {
	    fprintf (info, "%s Error: mallocing extension[%d]\n", 
		     localtime_string(dtime()), i);
	    exit(1);
	}
    }
    strcpy (extension[DAT_INDEX], "D");
    strcpy (extension[DET_INDEX], "E");
    strcpy (extension[CLK_INDEX], "T");
    strcpy (extension[CAL_INDEX], "C");
    strcpy (extension[LOG_INDEX], "L");
    strcpy (extension[BLK_INDEX], "O");
    parse_cfg ("SAVE","???");

    /* Read and parse client directives in the station's config file.	*/
    while (read_cfg(&cfg, str1, str2), (int)strlen(str1) > 0) {
	parse_cfg(str1,str2);
    }
    close_cfg (&cfg);

    /* Open the lockfile for exclusive use if lockfile is specified.	*/
    /* This prevents more than one copy of the program running for	*/
    /* a single station.						*/
    if (strlen(lockfile) > 0) {
	if ((lockfd = open (lockfile, O_RDWR|O_CREAT,0644)) < 0) {
	    fprintf (info, "%s Error: Unable to open lockfile: %s\n", 
		     localtime_string(dtime()), lockfile);
	    exit(1);
	}
	if ((status=lockf (lockfd, F_TLOCK, 0)) < 0) {
	    fprintf (info, "%s Error: Unable to lock daemon lockfile: %s status=%d errno=%d\n", 
		     localtime_string(dtime()),
		     lockfile, status, errno);
	    close (lockfd);
	    exit(1);
	}
    }

    terminate_proc = 0;
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

    /* Generate an entry for all available stations */      
    cs_setup (&tstations, client_name, station, TRUE, TRUE, 10, 
	      MAX_SELECTORS, data_mask, 6000);

    /* Create my segment and attach to all stations */      
    me = cs_gen (&tstations);

    /* Set up special selectors. */
    set_selectors (me);

    /* Show beginning status of all stations */
    strcpy(time_str, localtime_string(dtime()));
    for (j = 0; j < me->maxstation; j++) {
	this = (pclient_station) ((intptr_t) me + me->offsets[j]);
	printf ("%s - [%s] Status=%s\n", time_str, sname_str_cs(this->name), 
		(char *)stats[this->status]);
    }
    fflush (stdout);

    /* Loop until we are terminated by outside signal.			*/
    while (! terminate_proc) {
	j = cs_scan (me, &alert);
	if (j != NOCLIENT) {
	    this = (pclient_station) ((intptr_t) me + me->offsets[j]);
	    if (alert) {
		strcpy(time_str, localtime_string(dtime()));
		printf("%s - New status on station %s is %s\n", time_str,
		       sname_str_cs(this->name), (char *)stats[this->status]);
		fflush (stdout);
	    }
	    if (this->valdbuf) {
		pdat = (pdata_user) ((intptr_t) me + this->dbufoffset);
		for (k = 0; k < this->valdbuf; k++) {
		    char seqno[7];
		    pseed = (pvoid) &pdat->data_bytes;
		    if (verbosity & 2) {
			printf("%s [%s] <%2d> %s recvtime=%s ",
			       localtime_string(dtime()),
			       sname_str_cs(this->name), k, 
			       (char *)seednamestring(&pseed->channel_id,&pseed->location_id), 
			       (char *)localtime_string(pdat->reception_time));
			printf("hdrtime=%s ", time_string(pdat->header_time));
			strncpy (seqno, (char *)pdat->data_bytes, 6);
			seqno[6] = '\0';
			printf("seq=%s\n", seqno);
			fflush (stdout);
		    }
		    ret = store_seed(pseed);
                    if (ret == 2) {
			printf("BAD MSEED HEADER %s [%s] <%2d> %s recvtime=%s ",
			       localtime_string(dtime()),
			       sname_str_cs(this->name), k, 
			       (char *)seednamestring(&pseed->channel_id,&pseed->location_id), 
			       (char *)localtime_string(pdat->reception_time));
			printf("hdrtime=%s\n", time_string(pdat->header_time));
			fflush (stdout);
			store_bad_block((char *) &pdat->data_bytes);
                    }
		    pdat = (pdata_user) ((intptr_t) pdat + this->dbufsize);
		}
	    }
	}
	else {
	    if (verbosity & 2) {
		printf ("%s sleeping...",
			localtime_string(dtime()));
		fflush (stdout);
	    }
	    sleep (1);		/* Bother the server once every second */
	    if (verbosity & 2) {
		printf ("%s awake\n", localtime_string(dtime()));
		fflush (stdout);
	    }
	}
    }
    terminate_program (0);
    return(0);
}

void finish_handler(int sig)
{
    terminate_proc = 1;
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)	*/
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
    if (verbosity & 2) {
	printf ("%s %s - Terminating program.\n", time_str, station);
	fflush (stdout);
    }

    /* Perform final cs_scan for 0 records to ack previous records.	*/
    /* Detach from all stations and delete my segment.			*/
    if (me != NULL) {
	for (j=0; j< me->maxstation; j++) {
	    this = (pclient_station) ((intptr_t) me + me->offsets[0]);
	    this->reqdbuf = 0;
	}
	strcpy(time_str, localtime_string(dtime()));
	printf ("%s %s - Final scan to ack all received packets\n", 
		time_str, station);
	fflush (stdout);
	j = cs_scan (me, &alert);
	printf ("cs_scan() = %d\n", j);
	fflush (stdout);
	sleep (3);

	/* 
	sleep (1);
	printf ("%s %s - Second Final scan to ack all received packets\n", 
		time_str, station);
	j = cs_scan (me, &alert);
	printf ("cs_scan() = %d\n", j);
	fflush (stdout);
	sleep (1);
	*/

	cs_off (me);
    }

    if (strlen(pidfile)) unlink(pidfile);
    if (lockfd) close(lockfd);
    if (gapfd > 0) close(gapfd);
    strcpy(time_str, localtime_string(dtime()));
    printf ("%s - Terminated\n", time_str);
    exit(error);
}
