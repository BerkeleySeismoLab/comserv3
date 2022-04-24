/***************************************************************************
 * 
 * A SeedLink client to multicast.  Collects Mini-SEED data records
 * from a SeedLink serverand multicasts them for other multicast clients.
 *
 * Also a comserv client so that it can be start/stopped by netmon.
 *
 * Written by Doug Neuhauser, UC Berkeley Seismological Laboratory
 * Based on slink2mcast written by Doug Neuhauser, which is:
 * Based on slink2ew written by Chad Trabant, ORFEUS/EC-Project MEREDIAN,
 * The liss2ew sources were used as a template, thanks Pete Lombard.
 *
 * Modification History:
 *  2020-09-29 DSN ver 1.04 (2020.?) Updated for comserv3.
 *		Updated for comserv extended length server and client names.
 *  2021-04-27 DSN ver 1.4.1 (2021.117)
 *		Initialize config_struc structure before open_cfg call.
 *  2021-11-05 DSN ver 1.4.2 (2021.309)
 *		Incorporate AA patch to ignore timestamp and used only seqnum
 *		from state file to address presumed problem with RockToSLink
 *		seedlink server.  Use libslink-v2.6/develop branch with AA patch
 *		to support libslink auto-detection of MiniSEED record size.
 *  2022-02-59 DSN ver 1.4.3 (2020.059)
 *		Allow environmental override of STATIONS_INI pathname;
 ***************************************************************************/

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <qlib2.h>
#include <libslink.h>

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "service.h"
#include "seedstrc.h"
#include "cfgutil.h"
#include "timeutil.h"
#include "stuff.h"

#include "retcodes.h"

#define VERSION "1.4.2 (2021.309)"

#ifdef COMSERV2
#define	CLIENT_NAME	"SL2M"
#else
#define	CLIENT_NAME	"SL2MCAST"
#endif

#define TIMESTRLEN      80
#define MAX_SELECTORS CHAN+2
#define MAX_CHARS_IN_SELECTOR_LIST  300

char *syntax[] = {
    "%s   version " VERSION,
    "%s - retrieve MSEED records from a SEEDLINK server and multicast them.",
    "Syntax:",
    "%s [h] [-v] [-n] [-i in_blksize] [-o out_blksize] [-x]",
    "   -I interface_addr -M multicast_addr -p mulitcast_port ",
    "   -s statefile -C net_stalist -s statefile -S server:port station",
    "    where:",
    "     -h          Help - prints syntax message.",
    "     -v          Set verbosity flag.",
    "     -n          No output (for debugging purposes).",
    "     -i in_blksize",
    "                 Input MiniSEED blksize (default = 512)",
    "     -o out_blksize",
    "                 Output MiniSEED blksize (default = in_blksize)",
    "     -x          Do not run as comserv client.",
    "     -I multicast_interface",
    "                 Address of interface to multicast from",
    "     -M multicast_addr",
    "                 Multicast address to use",
    "     -p multicst_port",
    "                 Multicast port number to use",
    "     -s statefile",
    "                 Name of statefile in which to save/restore state.",
    "                 May specify statefile:N to save state every N packets.",
    "     -C net_stalist",
    "                 When connecting to seedlink servers that implement",
    "                 multi-station handshaking, a required comma-delimited list",
    "                 of NET_STA to get from the server.",
    "                 Can be specified on command line or in station.ini file.",
    "                 Do not use for connections to seedlink servers that",
    "                 use uni-station handshaking.",
    "                 In either case, you will receive all channels from the",
    "                 selected stations.",
    "     -S seedlinkserver:port",
    "                 Seedlink server:port.",
    "     station     Comserv/Mserv station name",
    "Notes:",
    "1.  Comserv/mserv client name is " CLIENT_NAME ".",
    "2.  Command line options override settings in station.ini file.",
    "Value only settable in [sl2m] section of station.ini file:",
    "    lockfile=pathname",
    "    pidfile=pathname",
    "optional items (command line options required if not specified):",
    "    statefile=pathname[:interval]",
    "    seedlink_station=NET_STA[,...,NET_STA] for multi-station handshaking.",
    "    seedlink_server=hostname:port",
    "    multicast_interface=hostname|ip_address",
    "    multicast_addr=ip_address",
    "    multicast_port=portnum",
    NULL
};

/***************************************************************************
 *
 * Global parameters
 *
 ***************************************************************************/
SLCD    *slconn;            /* SeedLink connection parameters       */
char    *statefile = 0;     /* State file                           */
int     stateint  = 100;   /* Interval (packets recv'd) to save state */

char    *cmdname;
FILE    *info;
int     output = 1;
struct  in_addr mcast_if;
struct  in_addr mcast_addr;
int     outsocket;
int     mcast_port;
int     run_as_client = 1;
int     slblksize = SLRECSIZE;
int     mcblksize = SLRECSIZE;
char    *new_msrecord = NULL;
char    *msrecord;

int mcast_mseed (void *msg, int nbytes, struct in_addr mcast_addr, 
                 int port, int socket);
void packet_handler (char *msrecord, int packet_type, int seqnum, int packet_size, int sl_packet_size);
int expand_blksize ( SDR_HDR *pseed, int old_blksize, int new_blksize);
static void print_timelog (const char *msg);
volatile sig_atomic_t terminate_proc = 0;
static int verbose = 0;
static pclient_struc me = NULL;
int lockfd;
typedef struct _sel {           /* Structure for storing selectors.     */
    int nselectors;               /* Number of selectors.                 */
    seltype *selectors;           /* Ptr to selector array.               */
} SEL;
static SEL sel[NUMQ];

int debug = 0;

typedef char char23[24];
static char23 stats[11] = { "Good", "Enqueue Timeout", "Service Timeout", 
                            "Init Error",
                            "Attach Refused", "No Data", "Server Busy", 
                            "Invalid Command",
                            "Server Dead", "Server Changed", "Segment Error" } ;

void finish_handler(int sig);
void terminate_program (int error);

#define ANNOUNCE(cmd,fp)							\
    ( fprintf (fp, "%s - Using STATIONS_INI=%s NETWORK_INI=%s\n", \
	      cmd, get_stations_ini_pathname(), get_network_ini_pathname()) )

/***********************************************************************
 *  main program
 ***********************************************************************/
int main (int argc, char **argv)
{
    extern char   *optarg;
    extern int    optind;
    int           c;
    SLpacket * slpack;
    int seqnum;
    int ptype;
    int retval;
    int packetcnt = 0;
    
    char  *multicast_interface = NULL;
    char  *multicast_addr = NULL;
    char  *multicast_port = NULL;
    char  *seedlink_server = NULL;

    int prethrottle = 0;
    int throttlecnt = 1;
    int throttlewait = 0;
    char *seedlink_station = NULL;
    char *seedlink_station_default = NULL;

    /* Start support for Comserv/Mserv interface */

    pclient_station thist;
    short j;
    boolean alert ;
    short data_mask = 0;			/* Data mask for cs_setup.      */
    static tclientname cname = CLIENT_NAME;	/* Default client name          */
    tservername sname = "" ;			/* Default station/mserv name.  */
    tstations_struc stations ;


    extern int save_selectors(int , char *);
    extern int set_selectors (pclient_struc );

    config_struc cfg;
    char str1[160], str2[160], station_dir[CSMAXFILELEN];
    char station_desc[60], source[160];
    char *pidfile = NULL;
    char *lockfile = NULL;
    char filename[CSMAXFILELEN];
    char time_str[TIMESTRLEN];
    FILE *fp;
    int status;
    static char m_channels[100];

    /* End support for Comserv/Mserv interface */

    cmdname = basename(argv[0]);
    info = stdout;

    /* Set stdout (where logs go) and stderr to always flush after a newline */
    setlinebuf(stdout);
    setlinebuf(stderr);
    setlinebuf(info);
    if (debug) { printf("DEBUG: setlinebuf() done.\n"); }

    while ((c = getopt (argc, argv, "hvnxI:M:p:s:C:S:i:o:")) != -1) {
	switch (c) {
	case '?':   
	case 'h':   ANNOUNCE(cmdname,info); print_syntax (cmdname, syntax, info); exit(0);
	case 'v':   verbose = 1; break;
	case 'n':   output = 0; break;
	case 'I':   multicast_interface = optarg; break;
	case 'M':   multicast_addr = optarg; break;
	case 's':   statefile = optarg; break;
	case 'p':   multicast_port = optarg; break;
	case 'C':   seedlink_station = optarg; break;
	case 'S':   seedlink_server = optarg; break;
	case 'i':   slblksize = atoi(optarg); break;
	case 'o':   mcblksize = atoi(optarg); break;
	case 'x':   run_as_client = 0; break;
	}
    }

    if (verbose) {
	debug = 1;
    }

    /*  Validate input and output blksize arguments. */
    if (slblksize != 128 && slblksize != 256 && slblksize != 512) {
	fprintf (info, "Error: invalid input blksize %d\n", slblksize);
	exit(1);
    }
    if (mcblksize != 128 && mcblksize != 256 && mcblksize != 512) {
	fprintf (info, "Error: invalid output blksize %d\n", mcblksize);
	exit(1);
    }
    if (mcblksize < slblksize) {
	fprintf (info, "Error: input blksize %d > output blksize %d\n", slblksize, mcblksize);
	exit(1);
    }
    /* Allocate always for case when input block size is smaller than the output */
    new_msrecord = malloc (mcblksize);
    if (new_msrecord == NULL) {
	fprintf (info, "Error: mallocing multicast buffer of %d bytes\n", mcblksize);
	exit(1);
    }

    /*    Skip over all options and their arguments.                      */
    argv = &(argv[optind]);
    argc -= optind;

    switch (argc) {
    case (0):
	fprintf (info, "Error: Missing comserv/mserv station name.\n");
	exit(1);
    case (1):
	strncpy (sname, argv[0], SERVER_NAME_SIZE);
	sname[SERVER_NAME_SIZE-1] = '\0' ;
	upshift(sname) ;
	break;
    default:
	fprintf (info, "Error: Unknown argument: %s\n", argv[0]);
	print_syntax (cmdname, syntax, info);
	exit (1);
    }

    if (run_as_client) {
	/* Look for info in the station.ini file.           */
	/* open the stations list and look for that station */
	ANNOUNCE(cmdname,info);
	strncpy(filename, get_stations_ini_pathname(), CSMAXFILELEN);
	filename[CSMAXFILELEN-1] = '\0';
	memset (&cfg, 0, sizeof(cfg));
	if (open_cfg(&cfg, filename, sname)) {
	    fprintf (stderr,"Could not find station\n") ;
	    exit(1);
	}
	/* Try to find the station directory, source, and description */
	station_dir[0] = '\0';
	do {
	    read_cfg(&cfg, str1, str2) ;
	    if (str1[0] == '\0')
		break ;
	    if (strcmp(str1, "DIR") == 0) {
		strcpy(station_dir, str2) ;
		if (debug) { printf("DIR = %s\n", station_dir); }
	    }
	    else if (strcmp(str1, "DESC") == 0) {
		strcpy(station_desc, str2) ;
		station_desc[59] = '\0' ;
		if (debug) { printf("DESC = %s\n", station_desc); }
		fprintf (info, "%s %s startup - %s\n", 
			 localtime_string(dtime()),
			 sname, station_desc) ;
	    }
	    else if (strcmp(str1, "SOURCE") == 0) {
		strcpy(source, str2) ;
		if (debug) { printf("SOURCE = %s\n", source); }
	    }
	}
	while (1) ;
	close_cfg(&cfg) ;

	if (station_dir[0] == '\0') {
	    fprintf (info, "%s Could not find station %s in %s file\n",
		     sname, localtime_string(dtime()), filename);
	    exit(1);
	}

	/* Open the station.ini file and read the section for this client.  */
	strcat(station_dir, "/station.ini");
	if (open_cfg(&cfg, station_dir, cname)) {
	    fprintf (info, "%s Could not find station.ini file\n", localtime_string(dtime()));
	    exit(1);
	}   
	while (read_cfg(&cfg, str1, str2), (int)strlen(str1) > 0) {
	    if (strcmp(str1, "PIDFILE") == 0) {
		pidfile=strdup(str2) ;
		if (debug) { printf("PIDFILE = %s\n", pidfile); }
	    }
	    else if (strcmp(str1, "LOCKFILE") == 0) {
		if (! lockfile) lockfile = strdup(str2) ;
		if (debug) { printf("LOCKFILE = %s\n", lockfile); }
	    }
	    else if (strcmp(str1, "STATEFILE") == 0) {
		if (! statefile) statefile = strdup(str2) ;
		if (debug) { printf("STATEFILE = %s\n", statefile); }
	    }
	    else if (strcmp(str1, "SEEDLINK_STATION") == 0) {
		if (! seedlink_station) seedlink_station = strdup(str2) ;
		if (debug) { printf("SEEDLINK_STATION = %s\n", seedlink_station); }
	    }
	    else if (strcmp(str1, "SEEDLINK_SERVER") == 0) {
		if (! seedlink_server) seedlink_server = strdup(str2) ;
		if (debug) { printf("SEEDLINK_SERVER = %s\n", seedlink_server); }
	    }
	    else if (strcmp(str1, "MULTICAST_INTERFACE") == 0) {
		if (! multicast_interface) multicast_interface = strdup(str2) ;
		if (debug) { printf("MULTICAST_INTERFACE = %s\n", multicast_interface); }
	    }
	    else if (strcmp(str1, "MULTICAST_ADDR") == 0) {
		if (! multicast_addr) multicast_addr = strdup(str2) ;
		if (debug) { printf("MULTICAST_ADDR = %s\n", multicast_addr); }
	    }
	    else if (strcmp(str1, "MULTICAST_PORT") == 0) {
		if (! multicast_port) multicast_port = strdup(str2) ;
		if (debug) { printf("MULTICAST_PORT = %s\n", multicast_port); }
	    }
	}
	close_cfg (&cfg);
    }

    /* Ensure required info found either in station.ini or on command line. */
    if (multicast_interface == NULL) {
	fprintf (stderr, "Error: missing -I multicast_interface option\n");
	exit(1);
    }
    if (multicast_addr == NULL) {
	fprintf (stderr, "Error: missing -M multicast_addr option\n");
	exit(1);
    }
    if (multicast_port == NULL) {
	fprintf (stderr, "Error: missing -p portnum option\n");
	exit(1);
    }
    if (statefile == NULL) {
	fprintf (stderr, "Error: missing -s statefile option\n");
	exit(1);
    }
    if (seedlink_server == NULL) {
	fprintf (stderr, "Error: missing -S seedlink_server:slport option\n");
	exit(1);
    }
    mcast_port = atoi(multicast_port);

    /* Grab lockfile if set*/
    if (lockfile && lockfile[0] != '\0') {
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

    /* Write pidfile if it is set*/
    if (pidfile && pidfile[0] != '\0') {
	if ((fp = fopen(pidfile,"w")) == NULL) {
	    fprintf (info, "Unable to open pid file: %s\n", pidfile);
	}
	else {
	    fprintf (fp, "%d\n",(int)getpid());
	    if (debug) { printf("DEBUG: pid = %d\n", (int)getpid()); }
	    fclose(fp);
	}
    } 

    if (run_as_client) {
	/* Create object for defining the Channel selector list. */

	char default_selectors[MAX_CHARS_IN_SELECTOR_LIST];
	strcpy(default_selectors,m_channels);
	save_selectors (0, default_selectors);
	if (debug) { printf("DEBUG: save_selectors() done.\n"); }

	/* Start comserv setup. */
	/* Generate an entry for the specified station. */
	cs_setup (&stations, cname, sname, TN_TRUE, TN_FALSE, 10, 
		  MAX_SELECTORS, data_mask, 6000) ;
	if (debug) { printf("DEBUG: cs_setup() done.\n"); }

	/* Create my segment and attach to all stations */      
	me = cs_gen (&stations);
	if (debug) { printf("DEBUG: cs_gen() done.\n"); }

	/* Set up special selectors. */
	set_selectors (me);
	if (debug) { printf("DEBUG: set_selectors() done.\n"); }

	/* End comserv setup. */
    }

    /* Start seedlink setup. */
    /* Allocate and initialize a new connection description */
    slconn = sl_newslcd();
    if (debug) { printf("DEBUG: set_sl_newslcd() done.\n"); }

    /* Be more aggressive in network timeouts and network reconnections. */
    slconn->netto     = 60;
    slconn->netdly    = 10;

    /* Initialize the verbosity for the sl_log function */
    sl_loginit (verbose, &print_timelog, NULL, &print_timelog, NULL);
    if (debug) { printf("DEBUG: set_loginit() done.\n"); }
    
    /* Report the program version */
    sl_log (1, 1, "%s version: %s\n", cmdname, VERSION);

    /* Parse the 'seedlink_station' string following '-S' */
    if ( seedlink_station ) {
	if (debug) { printf("DEBUG: seedlink_station = %s\n", seedlink_station); }
	if ( sl_parse_streamlist (slconn, seedlink_station, seedlink_station_default) == -1 ) {
	    fprintf (info, "Error: parsing -S %s\n", seedlink_station);
	    terminate_program(1);
	}
    }

    /* Attempt to recover sequence numbers from state file */
    if (debug) { printf("DEBUG: Attempt to recover sequence numbers from state file.\n"); }
    if (statefile) {
	/* Check if interval was specified for state saving */
	char *tptr;
	if ((tptr = strchr (statefile, ':')) != NULL) {
	    char *tail;
	    *tptr++ = '\0';
	    stateint = (unsigned int) strtoul (tptr, &tail, 0);
	    if ( *tail || (stateint < 0 || stateint > 1e9) ) {
		sl_log (2, 0, "state saving interval specified incorrectly\n");
		if (debug) { printf("DEBUG: State saving interval specified incorrectly.\n"); }
		return -1;
	    }
	}
	if (sl_recoverstate (slconn, statefile) < 0) {
	    sl_log (2, 0, "state recovery failed\n");
	    if (debug) { printf("DEBUG: State recovery failed.\n"); }
	}
    }
  
    /* Configure the SeedLink connection description thing */
    if (debug) { printf("DEBUG: Configure the Seedlink connection description.\n"); }
    slconn->sladdr = strdup(seedlink_server);
  
    if ((mcast_addr.s_addr = inet_addr(multicast_addr)) == (in_addr_t)-1) {
	fprintf (stderr, "Error: Invalid multicast addr: %s\n", multicast_addr);
	exit(1);
    }
    if ((mcast_if.s_addr = inet_addr(multicast_interface)) == (in_addr_t)-1) {
	fprintf (info, "Error: Invalid multicast addr: %s\n", multicast_interface);
	terminate_program(1);
    }
    outsocket = socket(AF_INET,SOCK_DGRAM,0);
    if (outsocket == -1) {
	perror("Error opening socket");
	if (debug) { printf("ERROR: Error opening socket.\n"); }
	exit(1);
    }
    if  (setsockopt(outsocket,
		    IPPROTO_IP,
		    IP_MULTICAST_IF, 
		    (char *)&mcast_if.s_addr,
		    sizeof(mcast_if.s_addr) ) == -1) {
	perror("Set Socket Opt IP_MULTICAST_IF error");
	if (debug) { printf("ERROR: Set Socket Opt IP_MULTICAST_IF error.\n"); }
	exit(1);
    }

    terminate_proc = 0;
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);

    /* Loop with checks to both the comserv and seedlink connection manager */

    if (debug) { printf("DEBUG: Enter loop with checks to both comserv and seedlink connection manager.\n"); }

    while (! terminate_proc) {

	if (run_as_client) {
	    /* Comserv code - check in with server. */
	    if (debug>1) { printf("DEBUG: Comserv code - check in with server.\n"); }
	    j = cs_scan (me, &alert) ;
	    if (j != NOCLIENT) {
		thist = (pclient_station) ((long) me + me->offsets[j]) ;
		if (alert) {
		    strcpy(time_str, localtime_string(dtime()));
		    fprintf (info, "%s - New status on station %s is %s\n",
			     time_str,
			     sname_str_cs(thist->name), (char *)(&(stats[thist->status]))) ;
		}
	    }
	}

	/* Seedlink code - get and multicast data. */
	if (debug>1) { printf("DEBUG: Seedlink code - get and multicast data.\n"); }

        /* Ignore timestamp. Use seqnum only to address positioning problem with RockToSLink server. */
        if (slconn->streams!=0)
            slconn->streams[0].timestamp[0] = 0;

	retval = sl_collect_nb_size (slconn, &slpack, slblksize);
	if (retval == SLTERMINATE) {
	    fprintf (info, "%s terminating on seedlink server SLTERMINATE retcode\n", cmdname);
	    terminate_proc = 1;
	    continue;
	}
	/* Check if we are being asked to terminate */
	if (debug) { printf("DEBUG: Check if termination requested.\n"); }
	if (slconn->terminate)
	{
	    fprintf (info, "%s terminating on seedlink server request\n", cmdname);
	    sl_terminate(slconn);
	    terminate_proc = 1;
	    continue;
	}
      
	/* Check if a packet arrived */
	if (debug>1) { printf("DEBUG: Check packet arrival.\n"); }
	if ( retval == SLPACKET )
	{
	    if (debug) { printf("DEBUG: retval == SLPACKET\n"); }
	    ptype  = sl_packettype (slpack);
	    seqnum = sl_sequence (slpack);
          
	    /* Expand/pad this seedlink record if necessary. */
	    if (slpack->reclen < mcblksize) {
		memcpy (new_msrecord,slpack->msrecord,slpack->reclen);
		expand_blksize ((SDR_HDR *)new_msrecord,slpack->reclen,mcblksize);
		msrecord = new_msrecord;
	    }
	    else {
		msrecord = (char *) slpack->msrecord;
	    }

	    packet_handler (msrecord, ptype, seqnum, mcblksize, slpack->reclen);
          
	    /* Save intermediate state file */
	    if ( statefile && stateint )
	    {
		if ( ++packetcnt >= stateint )
		{
		    if (debug) { printf("DEBUG: Save intermediate state file.\n"); }
		    sl_savestate (slconn, statefile);
		    packetcnt = 0;
		}
	    }
          
	    prethrottle = 0;
	}
	else /* Otherwise throttle the loop */
	{
	    /* Throttling scheme:
	     *
	     * First let the loop query the non-blocking socket 10 times (prethrottle),
	     * this should allow the TCP subsystem time enough when data has reached the
	     * host and is in the stack buffers.
	     *
	     * Next progressively throttle the loop by sleeping for 5ms and increasing
	     * the sleep time by 5ms increments until a maximum of 100ms.  Continue to
	     * throttle the loop by sleeping for 100ms until data arrives.
	     */
	    if (debug>1) { printf("DEBUG: retval != SLPACKET\n"); }
	    prethrottle++;
          
	    if ( prethrottle >= 10 )
	    {
		if ( throttlewait < 100 || throttlecnt == 1 )
		    throttlewait = 5 * throttlecnt;
              
		throttlecnt++;
              
		usleep (throttlewait*1000);
	    }
	    else
	    {
		throttlecnt = 1;
	    }
	}
    }
  
    if (debug) { printf("DEBUG: End of while loop.\n"); }

    if (verbose) {
	fprintf (info, "End of loop\n");
	fflush (info);
    }

    /* Make sure everything is shut down and save the state file */
    if ( slconn->link != -1 ) {
	sl_disconnect (slconn);
    }
  
    if ( statefile ) {
	sl_savestate (slconn, statefile);
    }

    terminate_program (0);

    printf("%s terminated\n", cmdname);

    return 0;
}                               /* End of main() */


/***************************************************************************
 * packet_handler():
 * Process a received packet based on packet type.
 ***************************************************************************/
void packet_handler (char *msrecord, int packet_type,
                     int seqnum, int packet_size, int sl_packet_size)
{
    int status;
    /* The following is dependent on the packet type values in libslink.h */
    char *type[]  = { "Data", "Detection", "Calibration", "Timing",
		      "Message", "General", "Request", "Info",
		      "Info (terminated)", "KeepAlive" };
  
    if (debug) { printf("FUNCTION CALL --> packet_handler()\n"); }

    /* Process waveform, detection, calibraion, timing, messages, and opaque data */
    if (debug) { printf("    --> (packet_handler) :  Process waveform, detection, calibration, timing, messages and opaque data.\n"); }

    if ( packet_type >= SLDATA && packet_type <= SLBLK ) {
	DATA_HDR *hdr;
	char *mseed = (char *)msrecord;
	int mseedsize = packet_size;
	sl_log (0, 1, "seq %d, Received %s blockette\n", seqnum, type[packet_type]);
	if (debug) { printf("    --> (packet_handler) : Received blockette\n"); }
      
	hdr = decode_hdr_sdr ((SDR_HDR *)mseed, mseedsize);
	if (hdr) {
	    if (verbose || debug) {
		struct timeval timeNow;                /* current time                */
		double dnnow, dnow, dfirst, dlast, dmid;
		status = gettimeofday(&timeNow, NULL);
		dnnow = (double)timeNow.tv_sec + ((double)(timeNow.tv_usec)/USECS_PER_SEC);
		dnow = nepoch_to_tepoch(dnnow);
		dfirst = int_to_tepoch(hdr->begtime);
		dlast = int_to_tepoch(hdr->endtime);
		dmid = (dfirst + dlast) / 2.;
		printf ("%s.%s.%s.%s rate=%d,%d nsamp=%d time=%s seq_no=%d mport=%d ibs=%d obs=%d latency=(%.3lf,%.3lf,%.3lf)\n",
			hdr->station_id, hdr->network_id, hdr->channel_id, hdr->location_id,
			hdr->sample_rate, hdr->sample_rate_mult,
			hdr->num_samples, time_to_str(hdr->begtime,MONTHS_FMT_1), hdr->seq_no, mcast_port,
			sl_packet_size, packet_size,
			dnow-dfirst, dnow-dmid, dnow-dlast);
	    }
	    if (mcast_port > 0 && (mseedsize == 512 || mseedsize == 256 || mseedsize == 128)) {
		status = 0;
		/* Multicast data ignoring return value, continue regardless of errors */
		if (output) {
		    status = mcast_mseed (mseed, mseedsize, mcast_addr, mcast_port, outsocket);
		}
		if (status != 0) {
		    fprintf (stderr, "Error: multicast %s.%s.%s.%s on port %d\n", 
			     hdr->station_id, hdr->network_id, 
			     hdr->channel_id, hdr->location_id,
			     mcast_port);
		}
	    }
	    else {
		if (verbose || debug) {
		    fprintf (stderr, "Warning: unknown port %d or bad mseedsize %d for %s.%s\n", 
			     mcast_port, mseedsize,
			     hdr->network_id, hdr->station_id);
		}
	    }
	    free_data_hdr(hdr);
	}
	else {
	    if (verbose || debug) {
		printf ("UNKNOWN MSEED record, seq=%d \n", seqnum);
	    }
	}
    }
    else if ( packet_type == SLKEEP )
    {
	sl_log (0, 2, "Keep alive packet received\n");
	if (debug) { printf("    --> (packet_handler) : Keep alive packet reveived\n"); }
    }
    else
    {
	sl_log (0, 1, "seq %d, Received %s blockette\n", seqnum, type[packet_type]);
	if (debug) { printf("    --> (packet_handler) : Received blockette\n"); }
    }
}                               /* End of packet_handler() */

/***************************************************************************
 * mcast_mseed():
 * Multicast a Mini-SEED record.
 * Currently the raw MiniSEED is multicast with no additional header.
 *
 * Return 0 on success and -1 on errors.
 ***************************************************************************/
int mcast_mseed (void *msg, int nbytes, struct in_addr mcast_addr, 
                 int port, int socket)
{
    static struct sockaddr_in name;
    int status = -1;
    int nsent;

    if (debug) { printf("FUNCTION CALL --> mcast_mseed()\n"); }
    memset(&name,0,sizeof(name));  /* Fill with zeros */
    name.sin_family  = AF_INET;
    name.sin_port    = htons( (unsigned short) port);
    name.sin_addr.s_addr = mcast_addr.s_addr;

    nsent = sendto(socket, (char *)msg, nbytes, 0, 
		   (struct sockaddr *) &name, sizeof(name));
    if (nsent == nbytes) status = 0;
    return (status);
}


/***************************************************************************
 * print_timelog:
 * Log message print handler used with sl_loginit() and sl_log().  Prefixes
 * a local time string to the message before printing.
 ***************************************************************************/
static void print_timelog (const char *msg)
{
    char timestr[100];
    time_t loc_time;

    if (debug) { printf("FUNCTION CALL --> print_timelog()\n"); }
    /* Build local time string and cut off the newline */
    time(&loc_time);
    strcpy(timestr, asctime(localtime(&loc_time)));
    timestr[strlen(timestr) - 1] = '\0';

    fprintf (stdout, "%s - %s", timestr, msg);
}



void finish_handler(int sig)
{
    char *message = "In finish_handler\n";
    if (debug) { printf("FUNCTION CALL --> finish_handler()\n"); }
    terminate_proc = 1;
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)      */
    /* fprint is not a safe call in a signal handler. */
    write (fileno(info),message,strlen(message));
}

/***********************************************************************
 *  terminate_program
 *      Terminate prog and return error code.  Clean up on the way out.
 ***********************************************************************/
void terminate_program (int error) 
{
    pclient_station thist;
    char time_str[TIMESTRLEN];
    int j;
    boolean alert ;

    if (debug) { printf("FUNCTION CALL --> terminate_program()\n"); }
    strcpy(time_str, localtime_string(dtime()));
    if (verbose)  {
	fprintf (info, "%s - Terminating program.\n", time_str);
    }

    if (statefile) {
	sl_savestate (slconn, statefile);
    }

    /* Perform final cs_scan for 0 records to ack previous records.       */
    /* Detach from all stations and delete my segment.                    */
    if (me != NULL) {
	for (j=0; j< me->maxstation; j++) {
	    thist = (pclient_station) ((long) me + me->offsets[0]) ;
	    thist->reqdbuf = 0;
	}
	strcpy(time_str, localtime_string(dtime()));
	fprintf (info, "%s - Final scan to ack all received packets\n", 
		 time_str);
	cs_scan (me, &alert);
	cs_off (me) ;
    }

    strcpy(time_str, localtime_string(dtime()));
    fprintf (info, "%s - Terminated\n", time_str);

    close (outsocket);
    if (lockfd) close(lockfd);

    exit(0);
}


char *selector_type_str[] = {"DAT", "EVT", "CAL", "TIM", "MSG", "BLK"};
/***********************************************************************
 *  save_selectors:
 *      Parse and save selectors for specific types of info.
 ***********************************************************************/
int save_selectors(int type, char *str)
{
    char *token;
    seltype *selectors = NULL;
    char *p = str;
    int n = 0;

    if (debug) { printf("FUNCTION CALL --> save_selectors()\n"); }
    if (str == NULL || (int)strlen(str) <= 0) {
	if (debug){ printf("    --> (save_selectors) : Error: Bad input str.\n"); }
	return(TN_FAILURE);
    }
    sel[type].nselectors = 0;
    if (sel[type].selectors) free (sel[type].selectors);
    while ((token = strtok(p,","))) {
	if ((int)strlen(token) > 5) {
	    if (debug) { printf("    --> (save_selectors) : Error in selector list\n"); }
	    fprintf (info, "Error in selector list for %s\n", selector_type_str[type]);
	    if (selectors) free (selectors);
	    return(TN_FAILURE);
	}
	selectors = (selectors == NULL) ? (seltype *)malloc(sizeof(seltype)) : 
	    (seltype *)realloc (selectors, (n+1)*sizeof(seltype));
	if (selectors == NULL) {
	    if (debug) { printf("    --> (save_selectors) : Error allocating selector space\n"); }
	    fprintf (info, "Error allocating selector space for %s\n", selector_type_str[type]);
	    return(TN_FAILURE);
	}
	strcpy(selectors[n++],lead(5,'?',token));
	p = NULL;
    }
    sel[type].selectors = selectors;
    sel[type].nselectors = n;
    return(TN_SUCCESS);
}

/***********************************************************************
 *  set_selectors:
 *      Set selector values for the single station.
 *      Assume sufficient selectors have been reserved.
 ***********************************************************************/
int set_selectors (pclient_struc me)
{
    pclient_station thist;
    pselarray psa ;
    int nsel = 1;
    int type, n, i;

    if (debug) { printf("FUNCTION CALL --> set_selectors()\n"); }

    if (debug) { printf("    --> (set_selectors) : set thist\n"); }
    thist = (pclient_station) ((long) me + me->offsets[0]) ;

    if (debug) { printf("    --> (set_selectors) : set psa\n"); }
    psa = (pselarray) ((long) me + thist->seloffset) ;
        
    if (debug) { printf("    --> (set_selectors) : for loop\n"); }
    for (type=0; type<CHAN; type++) {
	n = sel[type].nselectors;
	if (nsel+n > thist->maxsel) {
	    if (debug) { printf("    --> (set_selectors) : Error: Require %d selectors, allocate %d\n", nsel+n, thist->maxsel); }
	    fprintf (info, "Error: Require %d selectors, allocated %d\n",
		     nsel+n, thist->maxsel);
	    return(TN_FAILURE);
	}
	if (n > 0) {
	    if (debug) { printf("    --> type = %d, n = %d\n", type, n); }
	    thist->sels[type].first = 1;
	    thist->sels[type].last = 1;
	    /*            thist->sels[type].first = nsel; */
	    /*            thist->sels[type].last = nsel + n - 1; */
	    memcpy ((void *)&((*psa)[nsel]), (void *)sel[type].selectors, n*sizeof(seltype));
	    fprintf (info, "selector type = %d, nselectors = %d\n", type, n);
	    if ( debug) { printf("    --> selector type = %d, nselectors = %d\n", type, n); }
	    for (i=0; i<n; i++) fprintf (info, "%s\n",sel[type].selectors[i]);
	}
	nsel += n;
    }
    return(TN_SUCCESS);
}

/***********************************************************************
 *  expand_blksize:
 *      Expand the blocksize of the MiniSEED record to the newly
 *      specified MiniSEED blksize if it is larger.
 *  Return:
 *      1 if record is now specified new_blksize, 0 otherwise.
 *      If reblocked,
 *      a.  Change blksize in blockette 1000 of Mini_SEED record.
 ***********************************************************************/
int expand_blksize ( SDR_HDR *mseed, int old_blksize, int new_blksize)
{
    int status = 0;
    DATA_HDR *hdr;

    /* Determine the new minimum blksize and data_record_len.             */
    /* Limit the minimum size by the MIN_SEED_BLKSIZE.                    */
    if (my_wordorder < 0) get_my_wordorder();
    hdr = decode_hdr_sdr(mseed, old_blksize);

    if (new_blksize < old_blksize) return (status);

    /* If this record is to be expanded, find and                         */
    /* alter the data_rec_len in the blockette 1000.                      */
    if (old_blksize < new_blksize) {
	int new_data_rec_len;
	new_data_rec_len = log2(new_blksize);

	/* Rewrite MiniSEED record to reflect new blksize.                  */
	/* Change the blockette 1000 that contains the blocksize.           */
	BLOCKETTE_1000 *b1000;              /* ptr to blockette 1000.       */
	char *p = (char *)mseed;            /* ptr in SEED data record.     */
	short int bl_type, bl_next;         /* blockette header fields.     */
	int i = hdr->first_blockette;       /* index of blockette in record.*/
	int swapflag;                       /* flag for byteswapping.       */

	swapflag = (my_wordorder != hdr->hdr_wordorder);
	bl_next = hdr->first_blockette;
	while (bl_next > 0) {
	    b1000 = (BLOCKETTE_1000 *)(p+i);
	    bl_type = b1000->hdr.type;
	    bl_next = b1000->hdr.next;
	    if (swapflag) {
		swab2(&bl_type);
		swab2(&bl_next);
	    }
	    if (bl_type == 1000) {
		b1000->data_rec_len = new_data_rec_len;
		hdr->blksize = new_blksize;
		break;
	    }
	}
	/* Pad the expanded MiniSEED record with bytes of 0.                */
	memset((char *)mseed+old_blksize, 0, new_blksize-old_blksize);
    }

    status = 1;
    free_data_hdr(hdr);
    return (status);
}
