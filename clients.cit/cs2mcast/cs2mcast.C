/***********************************************************

File Name :
	cs2mcast.C

Programmer:
	Phil Maechling

Description:
	This is a comserv client that reads packets from a comserv memory
	area and writes the packets to a multicast address.
	This routine is intended to be managed by netmon. It should
	multicast for only one station. The channels and types of data
	to send are selectable.

Limitations or Warnings:

Creation Date:
	16 Feb 1999

Modification History:
    2000/06/27  Doug Neuhauser  Change m_types and m_channels to static for
				initialization purposes.
    2004/04/25  Paul Friberg    Added pidfile and lockfile usage to this program.
				Also added versioning v0.0.3
    2020-09-29  Doug Neuhauser  Updated for comserv3.
				Modified for 15 character station and client names.
    2021-04-27  Doug Neuhauser  Initialize config_struc structures before use.
				Change socket variable name to socket_fd.  
Usage Notes:

**********************************************************/

#define VERSION "1.1.1 (2021.117)"

#ifdef COMSERV2
#define CLIENT_NAME	"CS2M"
#else
#define CLIENT_NAME	"CS2MCAST"
#endif

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h>
#include <math.h>

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "service.h"
#include "seedstrc.h"
#include "cfgutil.h"
#include "timeutil.h"
#include "stuff.h"

#include "RetCodes.h"
#include "multicast_utils.h"
#define MAX_SELECTORS CHAN+2

#define CSMAXFILELEN 1024

const int MAX_CHARS_IN_SELECTOR_LIST = 300;

const char *syntax[] = {
"    [-?] [-h] [-v n] -M ip_addr -I ip_addr -P port_num -T d,e -C Channel1,ChannelN server_name",
"    where:",
"	-?		 	Help - prints syntax message.",
"	-h			Help - prints syntax message.",
"	-v n			Set verbosity level to n.",
"	-I 131.215.65.34	Sets the multicast interface to use as output.",
"	-A 192.0.0.100		Sets the multicast address to send the data to.",
"	-P 10000		Sets the multicast port to multicast.",
"	-T d,e,c,t,m,b or *	Sets the type of data to be multicast.",
"	-C channel_list		Sets the channels to be multicast.",
"				Examples are: ?H? or BHZ,BHN,BHE or *",
"				Wildcard characters must be quoted.",
"	server_name		Comserv server_name.",
"    Client name is " CLIENT_NAME "." ,
NULL };

const int TIMESTRLEN  = 40;
const int MAX_CHANNELS = 20; // This is an abitrary limit. It can be raised
			    // or lowered without problems.

const int MAX_CHARS_IN_CHANNEL_NAME = 10;

/************************************************************************
 *  Externals required in multiple files.
 ************************************************************************/

static tclientname name = CLIENT_NAME ;	/* Default client name		*/
tservername sname = "*" ;		/* Default station list.		*/

FILE *info = stdout;		/* Default FILE for messages.		*/
char	*cmdname;		/* Program name.			*/
short data_mask = 0;		/* data mask for cs_setup.		*/
tstations_struc stations ;
typedef char char23[24] ;
typedef char char5[6] ;

char23 stats[11] = { "Good", "Enqueue Timeout", "Service Timeout", 
		       "Init Error",
                       "Attach Refused", "No Data", "Server Busy", 
		       "Invalid Command",
                       "Server Dead", "Server Changed", "Segment Error" } ;

void finish_handler(int sig);
void terminate_program (int error);
static int terminate_proc;
int verbosity;
int debug = 0;
static pclient_struc me = NULL;

extern int save_selectors(int , char *);
extern int set_selectors (pclient_struc );

void scan_types(short*,
		char*);

extern "C" {
char* seednamestring (seed_name_type *, location_type *);
}

typedef struct _sel {		/* Structure for storing selectors.	*/
    int nselectors;		/* Number of selectors.			*/
    seltype *selectors;		/* Ptr to selector array.		*/
} SEL;

static SEL sel[NUMQ];


/************************************************************************
 *  print_syntax:
 *	Print the syntax description of program.
 ************************************************************************/
void print_syntax(char* cmd)
{
    int i;
    std::cout << cmd << " Version "<< VERSION << std::endl;
    std::cout << cmd;
    for (i=0; syntax[i] != NULL; i++) 
    {
	std::cout << syntax[i] << std::endl;
    }
}

/* Define these at file scope so that the write routine can see them */

struct Multicast_Info minfo;
int lockfd=0;

/************************************************************************
 *  main procedure
 ************************************************************************/
int main (int argc, char *argv[])
{
    pclient_station thist;
    short j, k;
    boolean alert ;
    pdata_user pdat ;
    seed_record_header *pseed ;
    int res;
    int status;
      
    FILE *fp;
    /* Define the option strings */
    char m_interface[100];
    char m_address[100];
    int  m_port;
    static char m_types[100];
    static char m_channels[100];

    config_struc cfg;
    char str1[160], str2[160], station_dir[CSMAXFILELEN];
    char station_desc[60], source[160];
    char filename[CSMAXFILELEN];
    char pidfile[CSMAXFILELEN];
    char lockfile[CSMAXFILELEN];
    char time_str[TIMESTRLEN];

    /* Find the length of the two double times which are at the front */
    /* of the user data returned from a comserv memory area */

    int timehdrinfo = (sizeof(double) * 2); 

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind;
    int		c;

    station_dir[0]='\0';
    lockfile[0]='\0';
    pidfile[0]='\0';

    // Show command line contents

    if(debug)
    {
	for (int jp=0;jp<argc;jp++)
	{
	    std::cout << argv[jp] << std::endl;
	}
    }

    /* Process cmdline options */

    cmdname = argv[0];

    while ( (c = getopt(argc,argv,"?hv:I:A:P:T:C:")) != -1)
	switch (c) 
	{
	case '?':
	case 'h':   print_syntax (cmdname); exit(0);
	case 'v':   verbosity=atoi(optarg); break;
	case 'I':   strcpy(m_interface,optarg); break;
	case 'A':   strcpy(m_address,optarg); break;
	case 'P':   m_port = atoi(optarg); break;
	case 'T':   strcpy(m_types,optarg);
	    scan_types(&data_mask,m_types);
	    break;
	case 'C':   strcpy(m_channels,optarg);
	    data_mask = (data_mask | CSIM_DATA);
	    break;
	}

    /*	Skip over all options and their arguments. */

    argv = &(argv[optind]);
    argc -= optind;
    info = stdout;

/* argc 0 now points past all the - options */
/* There must be a a station name */
/* minimum, otherwise there is an error */

/* This moves the station name into the sname var */
/* The station name is appended to the end by comserv or netmon */

    if (argc > 0)
    {
	strncpy(sname, argv[argc-1], SERVER_NAME_SIZE) ;
	sname[SERVER_NAME_SIZE-1] = '\0' ;
    }
    else
    {
	fprintf (stderr, "cs2mcast: Version %s\n", VERSION);
	fprintf (stderr, "Missing server name\n");
	exit(1);
    }
    upshift(sname) ;

/* open the stations list and look for that station */
    strcpy (filename, "/etc/stations.ini") ;
    memset (&cfg, 0, sizeof(cfg));
    if (open_cfg(&cfg, filename, sname))
    {
	fprintf (stderr,"%s Could not find server %s in file '%s'\n",
                 localtime_string(dtime()), sname, filename);
	exit(1);
    }
/* Try to find the server directory, source, and description */
    do
    {
	read_cfg(&cfg, str1, str2) ;
	if (str1[0] == '\0')
	    break ;
	if (strcmp(str1, "DIR") == 0)
	    strcpy(station_dir, str2) ;
	else if (strcmp(str1, "DESC") == 0)
	{
	    strcpy(station_desc, str2) ;
	    station_desc[59] = '\0' ;
	    fprintf (info, "%s %s startup - %s\n", 
		     localtime_string(dtime()),
		     sname, station_desc) ;
	}
	else if (strcmp(str1, "SOURCE") == 0)
	    strcpy(source, str2) ;
    }
    while (1) ;
    close_cfg(&cfg) ;

    if (station_dir[0] == '\0') {
        fprintf (info, "%s Could not find DIR entry for server %s in file '%s'\n",
                 localtime_string(dtime()), sname, filename);
        exit(1);
    }

/*  read the station.ini and get the lockfile and pidfile */
    /* Open and read station config file.                               */
    strcat(station_dir, "/station.ini");
    memset (&cfg, 0, sizeof(cfg));
    if (open_cfg(&cfg, station_dir, name)) {
        fprintf (info, "%s Could not find [%s] section in file '%s'\n",
                 localtime_string(dtime()), name, station_dir);
        exit(1);
    }	
    while (read_cfg(&cfg, str1, str2), (int)strlen(str1) > 0) {
	if (strcmp(str1, "PIDFILE") == 0)
	    strcpy(pidfile, str2) ;
	else if (strcmp(str1, "LOCKFILE") == 0)
	    strcpy(lockfile, str2) ;
    }
    close_cfg (&cfg);

/* fill pidfile if it is set*/
    if(pidfile[0] != '\0') {
	if ((fp = fopen(pidfile,"w")) == NULL) {
            fprintf (info, "%s Unable to open pid file: %s\n", 
                     localtime_string(dtime()),
		     pidfile);
        }
        else {
            fprintf (fp, "%d\n",(int)getpid());
            fclose(fp);
        }
    } 

/* grab lockfile if set*/
    if(lockfile[0] != '\0') {
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

/*
 * Create object for defining the Channel selector list
 */

    std::cout << "Current m_channels : " << m_channels << std::endl;
    char default_selectors[MAX_CHARS_IN_SELECTOR_LIST];
    strcpy(default_selectors,m_channels);

/* Test for at least one channel */

    if ( (strcmp(default_selectors,"") != 0))
    {
	std::cout << "Data Selector List " << default_selectors << std::endl;
	save_selectors (0, default_selectors);
    }

/* command line configuration processing done */

/* Initialize the multicast interface */

    res = init_multicast_socket(m_interface,
				m_address,
				m_port,
				minfo);

    if (res != TN_SUCCESS)
    {
	std::cout << "Error initializing multicast interface" << std::endl;
	return(res);
    }

    terminate_proc = 0;
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

/* Set time to UTC */

    char ist[] = "TZ=GMT";
    res = putenv(ist);
 
/* Set up a condition handler for SIGPIPE, since a write to a	*/
/* close pipe/socket raises a alarm, not an error return code.	*/

    signal (SIGPIPE,finish_handler);

/* Generate an entry for all available stations */      

    cs_setup (&stations, name, sname, TN_TRUE, TN_FALSE, 10, 
	      MAX_SELECTORS, data_mask, 6000) ;

/* Create my segment and attach to all stations */      
    me = cs_gen (&stations);

/* Set up special selectors. */

    set_selectors (me);

/* Show beginning status of all stations */

    strcpy(time_str, localtime_string(dtime()));
    for (j = 0 ; j < me->maxstation ; j++)
    {
	thist = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
	thist->seqdbuf = CSQ_NEXT;
	fprintf (info, "%s - [%s] Status=%s\n", 
		 time_str, sname_str_cs(thist->name), 
		 (char *)(stats[thist->status])) ;
    }
    fflush (info);

/* Send a message every second to all stations. */
/* Sending an attach command is harmless */

    do
    {
	j = cs_scan (me, &alert) ;
	if (j != NOCLIENT)
	{
	    thist = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
	    if (alert)
	    {
		strcpy(time_str, localtime_string(dtime()));
		fprintf (info, "%s - New status on server %s is %s\n",
			 time_str,
			 sname_str_cs(thist->name), (char *)(stats[thist->status])) ;
		fflush (info);
	    }
	    if (thist->valdbuf)
	    {
		pdat = (pdata_user) ((uintptr_t) me + thist->dbufoffset) ;
		for (k = 0 ; k < thist->valdbuf ; k++)
		{
		    pseed = (seed_record_header*) &pdat->data_bytes ;
		    if (verbosity & 1)
		    {
			fprintf (info, "[%s] <%2d> %s recvtime=%s ",
				 sname_str_cs(thist->name), k, 
			         seednamestring(&pseed->channel_id,
						&pseed->location_id), 
                                 localtime_string(pdat->reception_time)) ;

			printf("hdrtime=%s\n", 
			       time_string(pdat->header_time)) ;

			fflush (info);
		    }


		    int nbytes = thist->dbufsize - timehdrinfo;

		    if(verbosity & 1)
		    {

			char myseqno[7];
			strncpy(myseqno,pseed->sequence,6);
			memset(&myseqno[6],0,1);
			std::cout << "Sequence Number : " << 
			    myseqno << std::endl;
		    } 

		    res = multicast_packet(minfo,
					   (char*)pseed,
					   nbytes);
		    if(res != TN_SUCCESS)
		    {
			if(verbosity & 1)
			{
			    std::cout << "Error Multicasting Packet" << std::endl;
			}
		    }

		    // This moves the pointer to the
		    // next packet in memory

		    pdat = 
			(pdata_user) ((uintptr_t) pdat + thist->dbufsize) ;
		}
	    }
	}
	else
	{
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
    while (! terminate_proc) ;
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
	    thist = (pclient_station) ((uintptr_t) me + me->offsets[0]) ;
	    thist->reqdbuf = 0;
	}
	strcpy(time_str, localtime_string(dtime()));
	fprintf (info, "%s - Final scan to ack all received packets\n", 
		 time_str);
	fflush (info);
	cs_scan (me, &alert);
	cs_off (me) ;
    }

    strcpy(time_str, localtime_string(dtime()));
    fprintf (info, "%s - Terminated\n", time_str);

    close_multicast_socket(minfo.socket_fd);
    if (lockfd) close(lockfd);

    std::cout << "Exiting through signal handler" << std::endl;
    exit(error);
}


const char *selector_type_str[] = {"DAT", "EVT", "CAL", "TIM", "MSG", "BLK"};
/************************************************************************
 *  save_selectors:
 *	Parse and save selectors for specific types of info.
 ************************************************************************/
int save_selectors(int type, char *str)
{
    char *token = NULL;
    seltype *selectors = NULL;
    char *copy_str;
    char *p;
    int n = 0;

    if (str == NULL || (int)strlen(str) <= 0) return(TN_FAILURE);
    copy_str = strdup(str);
    p = copy_str;
    sel[type].nselectors = 0;
    if (sel[type].selectors) free (sel[type].selectors);
    while ((token = strtok(p,","))) {
	if ((int)strlen(token) > 5) {
	    fprintf (info, "Error in selector list for %s\n",
		     selector_type_str[type]);
	    if (selectors) free (selectors);
	    free(copy_str);
	    return(TN_FAILURE);
	}
	selectors = (selectors == NULL) ? (seltype *)malloc(sizeof(seltype)) : 
	    (seltype *)realloc (selectors, (n+1)*sizeof(seltype));
	if (selectors == NULL) {
	    fprintf (info, "Error allocating selector space for %s\n",
		     selector_type_str[type]);
	    free(copy_str);
	    return(TN_FAILURE);
	}
	strcpy(selectors[n++],lead(5,'?',token));
	p = NULL;
    }
    sel[type].selectors = selectors;
    sel[type].nselectors = n;
    free(copy_str);
    return(TN_SUCCESS);
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

    thist = (pclient_station) ((uintptr_t) me + me->offsets[0]) ;
    psa = (pselarray) ((uintptr_t) me + thist->seloffset) ;
	
    for (type=0; type<CHAN; type++) {
	n = sel[type].nselectors;
	if (nsel+n > thist->maxsel) {
	    fprintf (info, "Error: Require %d selectors, allocated %d\n",
		     nsel+n, thist->maxsel);
	    return(TN_FAILURE);
	}
	if (n > 0) {

	    thist->sels[type].first = 1;
	    thist->sels[type].last = 1;

/*	    thist->sels[type].first = nsel; */
/*	    thist->sels[type].last = nsel + n - 1; */
	    memcpy ((void *)&((*psa)[nsel]), (void *)sel[type].selectors, n*sizeof(seltype));
	    fprintf (info, "selector type = %d, nselectors = %d\n", type, n);
	    for (i=0; i<n; i++) fprintf (info, "%s\n",sel[type].selectors[i]);
	}
	nsel += n;
    }
    return(TN_SUCCESS);
}

/************************************************************************
 *  scan_types:
 *	Scan the data types field in the command line,
 *	and fill in comserv selector structure and data mask.
 ************************************************************************/
void scan_types(short* data_msk,
		char* cmd_line)
{
    char* tok;
    char* p = cmd_line;
    int res;
    *data_msk = 0;
    char* token_list[NUMQ];

    if (cmd_line == NULL || (int)strlen(cmd_line) <= 0)
    {
	return;
    }

    tok = strtok(p,",");

    int tok_num = 0;

    do
    {
	token_list[tok_num] = tok;
	tok_num++; 
	if (debug) {
	    printf("tok = `%s'\n", tok);
	}
	tok=strtok(0, ",");
    } while(tok);

    if (debug) {
	std::cout << "Number_of_Tokens Found : " << tok_num << std::endl;
    }

    for (int i = 0 ; i < tok_num; i++)
    {
	res = strcmp(token_list[i],"*");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_DATA | CSIM_EVENT | CSIM_CAL 
			 | CSIM_TIMING | CSIM_MSG | CSIM_BLK);

	    save_selectors(0,(char *)"?");
	    save_selectors(1,(char *)"?");
	    save_selectors(2,(char *)"?");
	    save_selectors(3,(char *)"?");
	    save_selectors(4,(char *)"?");
	    save_selectors(5,(char *)"?");
	}

	res = strcmp(token_list[i],"d");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_DATA);
	    save_selectors(0,(char *)"?");
	}

	res = strcmp(token_list[i],"e");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_EVENT);
	    save_selectors(1,(char *)"?");
	}

	res = strcmp(token_list[i],"c");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_CAL);
	    save_selectors(2,(char *)"?");
	}

	res = strcmp(token_list[i],"t");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_TIMING);
	    save_selectors(3,(char *)"?");
	}

	res = strcmp(token_list[i],"m");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_MSG);
	    save_selectors(4,(char *)"?");
	}

	res = strcmp(token_list[i],"b");
	if(res == 0)
	{
	    *data_msk = (*data_msk | CSIM_BLK);
	    save_selectors(5,(char *)"?");
	}

    }
    return;
}
