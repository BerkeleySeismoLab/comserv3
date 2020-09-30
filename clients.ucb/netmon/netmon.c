/************************************************************************
 *  Netmon - Comserv system NETwork MONitor and process manager program.
 *
 *  Douglas Neuhauser, UC Berkeley Seismological Laboratory
 *	Copyright 1995-1998 The Regents of the University of California.
 *      Copyright 1994-1997 Quanterra, Inc.
 ************************************************************************/

/* Modification History:
------------------------------------------------------------------------
    98/02/14 DSN	1.1.6	Asynch shutdown.
    I tried to speed up the response time for station startup and shutdowns,
    but it is limited by the 10 second status update in cs_check.  If you
    poll more often than this limit, netmon will get stale server status and
    WILL NOT WORK PROPERLY.  I added MIN_POLL and MIN_MAX_SHUTDOWN definitions
    and do not allow config file values to be set to less than these values.

    I experimented with a private fast_cs_check function which provides updated
    status information every 1 second instead of every 10 seconds, but it
    used too much system time attaching and detaching the shared memory
    segments with multiple comservs.  

    IGD changes for Linux compiling (commented with IGD)

    2006.017 PAF 1.1.8  modified execve  to pass current environment
    2020-09-29 DSN Updated for comserv3.
		ver 1.2.0 Modified for 15 character station and client names.
*/

#define	VERSION		"1.2.0 (2020.273)"
#define	CLIENT_NAME	"NETMON"
/* #define	CLIENT_NAME	"NETM" */

#include    <stdio.h>

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <libgen.h>
#ifndef	SOLARIS2
#include <sys/param.h>
#else
#include <netdb.h>
#endif
#ifndef	SOLARIS2
#ifndef	LINUX
/* #include <vfork.h> */
#endif
#endif

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "cfgutil.h"
#include "service.h"
#include "timeutil.h"
#include "stuff.h"

#define	QUOTE(x)	#x
#define	STRING(x)	QUOTE(x)

char *syntax[] = {
"%s version " VERSION,
"%s [-B] [-D] [-b] [-l] [-V] [-r] [-v] [-h] [-d n] [-s | -t]",
"       [-g group_list] [station_list]",
"    where:",
"    Daemon options:",
"	-D	    Run in \"daemon\" mode, and monitor the network of stations.",
"		    Start all startable stations.",
"	-B	    Run in background.",
"	-l	    Log output (stdout and stderr) to logfile in logdir.",
"	-b	    Boot mode.  Assume no running processes running.",
"		    Delete pid files.",
"    User options:",
"	-s	    Send START request to daemon for the comma-delimited",
"		    list of stations.  'ALL' or '*' signifies all stations.",
"	-t 	    Send TERMINATE request to daemon for the comma-delimited",
"		    list of stations.  'ALL' or '*' signifies all stations.",
#ifdef	ALLOW_RECONFIG
"	-r	    Send RECONFIGURE request to daemon.",
#endif	/* ALLOW_RECONFIG */
"	-g	    group1,[...,groupN]",
"		    A comma-delimited list of one or more groups of stations.",
"	-v	    Verbose option",
"	-h	    Print brief help message for syntax.",
"	-d n	    Set debug flag N.",
"		    1 = debug station and client status",
"		    2 = debug polling",
"		    4 = debug config",
"		    128 = debug malloc",
"	station_list",
"		    List of stations to start, terminate or query status.",
"		    List can be multiple tokens or comma-delimted list.",
"Examples:",
"	netmon		print status of network.",
"	netmon -v	print verbose status of network",
"	netmon WDC,CMB	print status of stations WDC and CMB",
"	netmon -v WDC	print verbose status of station WDC",
"	netmon -s WDC	startup server (and clients) for station WDC",
"	netmon -t WDC	terminate server (and clients) for station WDC",
"	netmon -t all	terminate server (and clients) for all stations",
"	netmon -g NHFN  print status of all stations in group NHFN.",
" Notes",
" 1.  The CS_CHECK_INTERVAL is " STRING(CS_CHECK_INTERVAL) " seconds.",
"     ALL comserv clients and servers must be compiled with the same value.",
NULL };

#define	NETWORK_FILE	"/etc/network.ini"
#define	STATIONS_FILE	"/etc/stations.ini"
#define	STATION_INI_FILE "station.ini"
#define STARTUP_STRING	"start"
#define SHUTDOWN_STRING	"shutdown"
#define	RECONFIG_STRING	"reconfig"
#define	STARTUP_REQUEST	'S'
#define	SHUTDOWN_REQUEST 'T'

#define	MIN_POLL		CS_CHECK_INTERVAL
#define	MIN_MAX_SHUTDOWN	(2*MIN_POLL)

#define CSCR_STATUS_UNKNOWN	-1

#define	PARTIAL_SHUTDOWN	0
#define	FULL_SHUTDOWN		1
#define	ERROR_SHUTDOWN		-1

#define	PARTIAL_STARTUP		0
#define	FULL_STARTUP		1
#define	ERROR_STARTUP		-1

#define	CLIENT_GOOD	0
#define	CLIENT_DEAD	1

#define	DAEMON_RESTART	0
#define	DAEMON_BOOT	1
#define	DAEMON_RECONFIG	2

#define	debug(flag)	(flag & debug_flag)
#define	DEBUG_STATUS	1
#define	DEBUG_POLL	2
#define	DEBUG_CONFIG	4
#define	DEBUG_STATE	8
#define	DEBUG_MALLOC	128

/************************************************************************
 *  Structures for client and station info used by this program.
 ************************************************************************/
typedef struct _client_info {		/* Client info structure.	*/
    tclientname client;			/* Client name.			*/
    char pidfile[256];			/* Client pid file.		*/
    char program[256];			/* Client program.		*/
    double time_check;			/* Time to check client status.	*/
    int pid;				/* Client pid.			*/
    int	block_count;			/* # of packets blocked.	*/
    int status;				/* Client status.		*/
    int input_client;			/* Client feeds server.		*/
    struct _client_info *next;		/* Ptr to next client.		*/
} CLIENT_INFO;

typedef struct _station_info {		/* Station info structure.	*/
    tservername station;		/* Station name.		*/
    char config_state;			/* Station run state in file.	*/
    char target_state;			/* Station target run state.	*/
    char present_state;			/* Station present run state.	*/
    char queued_request;		/* Queued request for station.	*/
    char shutting_down;			/* Processing shutdown request.	*/
    char source[32];			/* Source of data.		*/
    char dir[256];			/* Station config directory.	*/
    char program[256];			/* Server program.		*/
    double time_check;			/* Time to check server status.	*/
    double time_complete;		/* Time when action must finish.*/
    double time_timeout_started;	/* Time that timeout started.	*/
    double time_last_notified;		/* Time of last timeout notify.	*/
    double queued_request_time;		/* Time last request was queued.*/
    int server_startup_delay;		/* Server startup delay (sec).	*/
    int client_startup_delay;		/* Client startup delay (sec).	*/
    int max_shutdown_wait;		/* Max shutdown wait time (sec).*/
    int min_notify;			/* Minimum timeout notification.*/
    int re_notify;			/* Re-notification timeout.	*/
    int res_notify;			/* Bool to notify when resumes.	*/
    char notify_prog[SECWIDTH];		/* Timeout notification prog.	*/
    int server_segment;			/* Shared segment for server.	*/
    int pid;				/* Server pid.			*/
    int status;				/* Server status.		*/
    linkstat_rec *linkstat;		/* Ptr to linkstat structure.	*/
    int nclients;			/* # of clients.		*/
    CLIENT_INFO *client;		/* Ptr to array of client info.	*/
    struct _station_info *next;		/* Ptr to next station.		*/
} STATION_INFO;

/************************************************************************
 * Externals for command line parsing (and other functions).
 ************************************************************************/
char *cmdname;				/* Name of this program.	*/
int boot_flag;				/* Boot time functionality.	*/
int daemon_flag;			/* Run as monitor daemon.	*/
int background_flag;			/* Run in background.		*/
int verbose;				/* Verbosity flag.		*/
int start_flag;				/* Start station(s) flag.	*/
int term_flag;				/* Terminate station(s) flag.	*/
int reconfig_flag = 0;			/* Force runtime reconfig.	*/
int debug_flag;				/* Debugging flag.		*/
int terminate_proc;			/* Flag to terminate.		*/
int log_to_file;			/* Send output to logfile.	*/
FILE *info;				/* Output FILE for info.	*/
char *all_stations;			/* String denoting all stations.*/
char logfile[2048];			/* Full pathname of logfile.	*/
char hostname[MAXHOSTNAMELEN+1];	/* My hostname.			*/
char version[80] = VERSION;		/* Current version.		*/
char *groupstring = NULL;		/* List of station groups.	*/

/************************************************************************
 * Structures required for station status.
 ************************************************************************/
pclient_struc me;			/* Ptr to shared mem for prog.	*/
STATION_INFO *st_head;			/* Head ptr to station list.	*/

/************************************************************************
 * Network init file parameters and initial default values.
 ************************************************************************/
char logdir[SECWIDTH];			/* Log directory.		*/
char cmddir[SECWIDTH];			/* Cmd diredtory.		*/
int poll_interval = 10;			/* Polling interval (sec).	*/
int net_server_startup_delay = 10;	/* Server startup delay (sec).	*/
int net_client_startup_delay = 10;	/* Client startup delay (sec).	*/
int net_max_shutdown_wait = 15;		/* Max shutdown wait time (sec).*/
int net_min_notify = 300;		/* Minimum timeout notification.*/
int net_re_notify = 3600;		/* Re-notification timeout.	*/
int net_res_notify = 1;			/* Notify when back up.		*/
char net_notify_prog[SECWIDTH];		/* Timeout notification prog.	*/
char lockfile[SECWIDTH];		/* Daemon lockfile.		*/
int lockfd;				/* Lockfile file desciptor.	*/

/************************************************************************
 *  Function declarations, both local and external.
 ************************************************************************/
int monitor_network();
STATION_INFO *collect_station_info (char *station);
STATION_INFO *find_station_entry (char *station, int create_flag);
CLIENT_INFO *find_client_entry (STATION_INFO *s, char *client, int create_flag);
void dump_stations ();
void dump_station (STATION_INFO *s);
void dump_clients (STATION_INFO *s);
void dump_client (STATION_INFO *s, CLIENT_INFO *c);
int server_status (STATION_INFO *s);
int client_status (STATION_INFO *s, CLIENT_INFO *c);
int spawn_process (char *prog, char *station);
int write_request(char *station, char *op);
int print_syntax(char *cmd, char *syntax[], FILE *fp);
int shutdown_station (STATION_INFO *s);
int startup_station (STATION_INFO *s);
int station_index (char *s);
char station_state (STATION_INFO *s);
int suspend_link (STATION_INFO *s);
int resume_link (STATION_INFO *s);
int process_requests();
int timeout_notify(char *station, char *notify_prog, double timeout_started, 
		   double time_last_notified, double last_good, 
		   double now);
int resume_notify(char *station, char *notify_prog, double timeout_started, 
		  double time_last_notified, double last_good, 
		  double now);
void check_station_telemetry(STATION_INFO *s);
void set_initial_states (STATION_INFO *s, int flag);
void reconfig_daemon();
void read_network_cfg (char *config_file, char *section);

void finish_handler(int sig);
void hup_handler(int sig);
#ifndef	SOLARIS2
#ifndef	LINUX
char *strerror (int errno);
#endif	/* LINUX */
#endif	/* SOLARIS2 */
int collect_group_info (char *group);

/************************************************************************
 * Server status strings, borrowed from other comserv clients.
 ************************************************************************/
typedef char char23[24] ;
typedef char char7[8] ;
char23 stats[11] = {
    "Good", "Enqueue Timeout", "Service Timeout", "Init Error",
    "Attach Refused", "No Data", "Server Busy", "Invalid Command",
    "Server Dead", "Server Changed", "Segment Error" } ;
char23 cstats[2] = {
    "Good", "Client Dead" };

/************************************************************************
 *  main:   Main program.
 ************************************************************************/
int main(int argc, char **argv)
{
    int pid, status;
    int display_status;
    char *station_list, *station, *operation;
    STATION_INFO *s;
    CLIENT_INFO *c;
    char *p;
    struct stat sb;
    struct dirent *dirent;
    DIR *dirp;

    /* Variables needed for getopt. */
    extern char	*optarg;
    extern int	optind, opterr;
    int		ch;

    info = stdout;
    cmdname = basename(strdup(argv[0]));
    all_stations = "*";
    while ( (ch = getopt(argc,argv,"hBDbvlstrd:g:")) != -1)
	switch (ch) {
	case '?':
	case 'h':   print_syntax(cmdname,syntax,info); exit(1);
	case 'B':   background_flag = 1; break;
	case 'D':   daemon_flag = 1; break;
	case 'b':   boot_flag=DAEMON_BOOT; break;
	case 'v':   verbose=1; break;
	case 'l':   log_to_file=1; break;
#ifdef	ALLOW_RECONFIG
	case 'r':   reconfig_flag = 1; break;
#endif
	case 's':   start_flag = 1; break;
	case 't':   term_flag = 1; break;
	case 'd':   debug_flag = atoi(optarg); break;
	case 'g':   groupstring = optarg; break;
	default:
	    fprintf (info, "Unsupported option: -%c\n", ch);
	    exit(1);
	}

    /*	Skip over all options and their arguments.			*/
    argv = &(argv[optind]);
    argc -= optind;

    hostname[MAXHOSTNAMELEN]='\0';
    if (gethostname (hostname, MAXHOSTNAMELEN) < 0) {
	perror("Unable to read hostname\n");
	exit(1);
    }

    if (boot_flag + start_flag + term_flag + reconfig_flag > 1) {
	fprintf (info, "Too many flags set\n");
	print_syntax(cmdname,syntax,info);
	exit(1);
    }

    if (daemon_flag + start_flag + term_flag +reconfig_flag > 1) {
	fprintf (info, "Too many flags set\n");
	print_syntax(cmdname,syntax,info);
	exit(1);
    }

    if (boot_flag && ! daemon_flag) {
	fprintf (info, "Cannot use boot option without daemon option\n");
	print_syntax(cmdname,syntax,info);
	exit(1);
    }

    read_network_cfg (NETWORK_FILE, CLIENT_NAME);

    if (daemon_flag + background_flag + boot_flag > 0 && strlen(lockfile) > 0) {
	/* Perform preliminary lock on lockfile to see if another copy	*/
	/* of the daemon process is running.				*/
	if ((lockfd = open (lockfile, O_RDWR|O_CREAT,0644)) < 0) {
	    fprintf (info, "Unable to open lockfile: %s\n", lockfile);
	    exit(1);
	}
	if ((status=lockf (lockfd, F_TLOCK, 0)) < 0) {
	    fprintf (info, "Unable to lock daemon lockfile: %s status=%d errno=%d\n", 
		     lockfile, status, errno);
	    exit(1);
	}
	close(lockfd);
	lockfd = 0;
    }

    if (daemon_flag && log_to_file) {
	fflush (stdout);
	fflush (stderr);
	sprintf (logfile, "%s/%s.log", logdir,CLIENT_NAME);
	if (freopen (logfile, "a", stdout) == NULL) {
	    fprintf (stderr, "Unable to open logfile: %s\n", logfile);
	    exit(1);
	}
	fflush (stderr);
	dup2 (1,2);
	info = stderr;
    }

    /* Spawn child if daemon flag is set. */
    if (background_flag) {
	pid = fork();
	switch (pid) {
	case -1:	/* error */
	    fprintf (info, "error %d from fork\n", errno);
	    return(1);
	    break;
	case 0:	/* child */
	    break;
	default:	/* parent */
	    return(0);
	}
	if (strlen(lockfile) > 0) {
	    /* Child must lock the lockfile to prevent other copies of	*/
	    /* the daemon from running.					*/
	    if ((lockfd = open (lockfile, O_RDWR|O_CREAT,0644)) < 0) {
		fprintf (info, "Unable to open lockfile: %s\n", lockfile);
		exit(1);
	    }
	    if ((status=lockf (lockfd, F_TLOCK, 0)) < 0) {
		fprintf (info, "Unable to lock daemon lockfile: %s status=%d errno=%d\n", 
			 lockfile, status, errno);
		exit(1);
	    }
	}
    }

    signal (SIGHUP,hup_handler);
    signal (SIGINT,finish_handler);
    signal (SIGTERM,finish_handler);
    if (background_flag || daemon_flag) {
	fprintf (info, "%s %s version %s (%d) (check interval %d) started\n", localtime_string(dtime()),
		 cmdname, VERSION, getpid(), CS_CHECK_INTERVAL);
	fflush (info);

	/* Ensure that daemon config parameters are within limits.	*/
	if (poll_interval < MIN_POLL) {
	    poll_interval = MIN_POLL;
	    fprintf (info, "%s Warning: MIN_POLL reset to %d\n", 
		     localtime_string(dtime()), MIN_POLL);
	}
	if (net_server_startup_delay < MIN_POLL) {
	    net_server_startup_delay = MIN_POLL;
	    fprintf (info, "%s Warning: SERVER_STARTUP_DELAY reset to %d\n", 
		     localtime_string(dtime()), net_server_startup_delay);
	}
	if (net_client_startup_delay < MIN_POLL) {
	    net_client_startup_delay = MIN_POLL;
	    fprintf (info, "%s Warning: CLIENT_STARTUP_DELAY reset to %d\n", 
		     localtime_string(dtime()), net_client_startup_delay);
	}
	if (net_max_shutdown_wait < MIN_MAX_SHUTDOWN) {
	    net_max_shutdown_wait = MIN_MAX_SHUTDOWN;
	    fprintf (info, "%s Warning: MAX_SHUTDOWN_WAIT reset to %d\n", 
		     localtime_string(dtime()), net_max_shutdown_wait);
	}
	fflush (info);
    }

    /* Collect config on all specified groups of stations.		*/
    if (groupstring) {
	char *group_list = groupstring;
	char *group;
	while ((group = strtok (group_list,","))) {
	    group_list = NULL;
	    if (collect_group_info(group) == 0) {
		fprintf (info, "Group %s not found\n", group);
		continue;
	    }
	}
    }

    /* Collect config on all specified stations.			*/
    /* If no stations or groups were specified, assume ALL stations.	*/
    if (groupstring == NULL && argc <= 0) {
	argc = 1;
	p = all_stations;
	argv = &p;
    }
    while (argc-- > 0) {
	station_list = strdup(*(argv++));	 /* IGD To avoid segfault , get a copy first */
	station = strtok(station_list,",");	/* IGD First call to strtok needs the 1 argument */
	while (station) {
	    station_list = NULL;
	    if (strcasecmp(station,"ALL")==0) station = all_stations;
	    if ((s=collect_station_info (station)) == NULL) {
		fprintf (info, "Station %s not found\n", station);
	    }
	    station= strtok(NULL, ",") ; /*IGD  Second call to strtoc needs NULL as an argument*/
	}
    }

    /* If startup or terminate option was specified, write cmd file.	*/
    if (start_flag || term_flag) {
	operation = (start_flag) ? STARTUP_STRING : SHUTDOWN_STRING;
	for (s=st_head; s!=NULL; s=s->next) {
	    if (s->config_state == 'I') continue;
	    write_request (s->station, operation);
	}
	exit(0);
    }

#ifdef	ALLOW_RECONFIG
    if (reconfig_flag) {
	write_request (CLIENT_NAME, RECONFIG_STRING);
	exit(0);
    }
#endif

    /* Get initial status of all servers and their clients.		*/
    /* Display the results if we are in neither daemon nor boot mode.	*/
    display_status = (daemon_flag == 0);
    for (s=st_head; s!=NULL; s=s->next) {
	if (s->config_state == 'I') continue;
	status = server_status(s);
	for (c=s->client; c!= NULL; c=c->next) {
	    status = client_status(s,c);
	}
	set_initial_states(s, DAEMON_RESTART);
	if (display_status) {
	    dump_station(s);
	    for (c=s->client; c!=NULL; c=c->next) dump_client(s,c);
	}
    }

    /* If we are in boot mode, clear any outstanding requests and	*/
    /* and perform any initial startup or shutdown.			*/
    if (boot_flag) {
	/* Remove all pending cmd file.					*/
	if ((dirp = opendir(cmddir)) != NULL) {
	    char path[2048];
	    while ((dirent = readdir(dirp)) != NULL) {
		sprintf (path, "%s/%s", cmddir, dirent->d_name);
		if (stat(path,&sb)==0 && S_ISREG(sb.st_mode)) unlink(path);
	    }
	    closedir(dirp);
	}
	else {
	    fprintf (info, "Unable to open dir %s\n", cmddir);
	}
	/* Setup state flags and perform initial startup or shutdown.	*/
	for (s=st_head; s!=NULL; s=s->next) {
	    if (s->config_state == 'I') continue;
	    set_initial_states(s, boot_flag);
	    /* Remove any client pid files.				*/
	    for (c=s->client; c!=NULL; c=c->next) {
		if (strlen(c->pidfile) != 0) unlink(c->pidfile);
	    }
	}
	boot_flag = 0;
    }

    /* Daemon operation - continuously monitor the network programs.	*/
    if (daemon_flag) {
	monitor_network();
    }
	
    /* Wrapup and sign off.						*/
    if (me) {
	cs_off (me) ;
    }
    if (daemon_flag) {
	fprintf (info, "%s %s (%d) terminated\n", localtime_string(dtime()), 
		 cmdname, getpid());
    }
    return(0);
}
    
/************************************************************************/
/*
    State variables for server:	
	C = Config_state.
	T = target_state.
	P = Present_state.
    State values for variables:	
	N = Not runnable / Not running.
	R = Runnable by explicit startup command / Running.
	S = Start on reboot.
	A = Auto-restart: Restart if it dies.
	U = Undetermined.  Take no action at this point.
	I = Ignore station.
	NA = Not applicable

		Precise meaning of values in each state
	Config_state		Target_state		Present_state
	----------------------------------------------------------------
    N	Not_Runnable		Shutdown/Do_Not_Start	Not_Running
    R	Runable_by_start_cmd	Startable_by_cmd	Server&Clients_Running
    S	Start_at_boot		Station_being_started	NA
    A	Auto_Restart		Auto_Restart_if_dies	NA
    I	Ignore_station		NA			NA
    U	NA			NA			Undetermined_state
*/
/************************************************************************/

/************************************************************************
 *  monitor_network:
 *  a.	Continuously monitor all stations in the network, 
 *	and performs startup and shutdown of processes as required.
 *  b.	Process queued requests when station is in a stable state.
 *  c.	Perform timeout notification for telemetry problems.
 ************************************************************************/
int monitor_network()
{
    double now;
    int status;
    int new_requests;
    STATION_INFO *s;
    CLIENT_INFO *c;

    while (! terminate_proc) {
	for (s=st_head; s!=NULL; s=s->next) {
	    if (s->config_state == 'I') continue;
	    now = dtime();

	    /* Update the present server status.			*/
	    status = server_status(s);
	    if (status == CSCR_STATUS_UNKNOWN) continue;
	    /* Monitor each client.					*/
	    for (c=s->client; c!= NULL; c=c->next) {
		status = client_status(s,c);
	    }

	    /* Compute the state of the station.			*/
	    /* Cancel any time completion or time check conditions if	*/
	    /* the station has reached its fully up configured state.	*/
	    s->present_state = station_state(s);
	    if (s->present_state == 'R' && 
		(s->target_state == 'A' ||
		 s->target_state == 'S' || 
		 s->target_state == 'R')) {
		s->time_complete = s->time_check = 0;
	    }

	    /* Process queued request if we are not waiting for an	*/
	    /* an action to complete.					*/
	    if (s->queued_request && now > s->time_complete) {
		/* Process queued command. */
		switch (s->queued_request) {
		case STARTUP_REQUEST:
		    switch (s->config_state) {
		    case 'A': s->target_state = 'A'; s->present_state = 'U'; break;
		    case 'S':
		    case 'R': s->target_state = 'S'; s->present_state = 'U'; break;
		    case 'N': s->target_state = 'N'; break;
		    default:
			fprintf (info, "%s Invalid config state %c for %s\n",
				 localtime_string(dtime()), s->config_state, 
				 s->station);
		    }
		    s->queued_request = 0;
		    break;
		case SHUTDOWN_REQUEST:
		    s->target_state = 'N';
		    s->queued_request = 0;
		    break;
		default:
		    fprintf (info, "%s %s (%d) Unknown request '%c' for %s\n", 
			     localtime_string(dtime()), cmdname, getpid(),
			     s->queued_request, s->station);
		    s->queued_request = 0;
		    break;
		}
	    }

	    /* Perform action based on state settings.			*/
	    switch (s->present_state) {
	    case 'R':
		if (s->target_state == 'S' && s->linkstat && 
		    s->linkstat->suspended == 0) {
		    s->target_state = 'R';
		}
		else if (s->target_state == 'N') {
		    shutdown_station(s);
		    s->present_state = station_state(s);
		}
		break;
	    case 'N':
		if (s->target_state == 'A' || s->target_state == 'S') {
		    startup_station(s);
		    s->present_state = station_state(s);
		}
		break;
	    case 'U':
		if (s->target_state == 'N' || s->target_state == 'R') {
		    shutdown_station(s);
		    s->present_state = station_state(s);
		}
		else if (s->target_state == 'A' || s->target_state == 'S') {
		    startup_station(s);
		    s->present_state = station_state(s);
		}
		break;
	    default:	break;
	    }
	}

	/* Process all outstanding requests.				*/
	new_requests = 0;
	new_requests = process_requests();
#ifdef	ALLOW_RECONFIG
	if (reconfig_flag) reconfig_daemon();
#endif	/* ALLOW_RECONFIG */
	if (! new_requests) {
	    if (debug(DEBUG_POLL)) {
		fprintf (info, "sleeping...");
		fflush (info);
	    }
	    sleep (poll_interval);
	    if (debug(DEBUG_POLL)) {
		fprintf (info,"awake\n");
		fflush (info);
	    }
	}
	fflush (info);
    }
    return 0;
}

/************************************************************************
 *  collect_station_info:
 *	Collect information for the specified station(s).
 *	station_name may be a wildcard "*" denoting all stations.
 *	Add to global linked list pointed to by st_head.
 *	Initialize all fields in case this is a station reconfiguration.
 ************************************************************************/
STATION_INFO *collect_station_info (char *station_name)

{
    STATION_INFO *s;
    CLIENT_INFO *c;
    tservername station;
    char str1[SECWIDTH], str2[SECWIDTH];
    config_struc station_cfg;
    char *p;
    char station_cfg_file[256];
    config_struc master_cfg;		/* Master station file cfg.	*/

    /* If this is a rescan of all stations, reset state of existing	*/
    /* stations to ignore.						*/
    if (strcasecmp(station_name,all_stations)==0) {
	for (s=st_head; s!=NULL; s=s->next) {
	    s->config_state = 'I';
	}
    }

    /* Scan station list.						*/
    if (open_cfg(&master_cfg,STATIONS_FILE,station_name)) {
	fprintf (info, "Could not find station %s\n", station_name);
	exit(1);
    }

    /* Loop until section is not found in order to handle wildcards.	*/
    if debug(DEBUG_CONFIG) {
	    fprintf (info, "Looking in master config for [%s].\n", station_name);
	    fflush (info);
	}
    while (skipto(&master_cfg,station_name)==0) {
	if debug(DEBUG_CONFIG) {
		fprintf (info, "Found [%s] in master config.\n", station_name);
		fflush (info);
	    }
	/* Pick up station name from the section heading.		*/
	strcpy (station,master_cfg.lastread+1);
	station[strlen(station)-1] = '\0';

	/* Find existing entry for this station, or create new entry.	*/
	/* (Re)initialize important entries, and free old clients.	*/
	s = find_station_entry(station, 1);
	s->dir[0] = s->source[0] = s->program[0] = s->notify_prog[0] = '\0';
	s->config_state = 'I';		/* Assume we ignore station.	*/
	if (s->linkstat) {
	    free(s->linkstat);
	    s->linkstat = NULL;
	}
	while ((c=s->client) != NULL) {
	    s->client = c->next;
	    free(c);
	}
	
	/* Read and verify basic station info from master config file.	*/
	while (1) {
	    read_cfg(&master_cfg,&str1[0],&str2[0]);
	    if (str1[0] == '\0') break;
	    if (strcmp(str1,"DIR")==0) {
		strcpy(s->dir,str2);
	    }
	    if (strcmp(str1,"SOURCE")==0) {
		strcpy(s->source,str2);
	    }
	}
	if (s->source[0] == '\0') {
	    fprintf (info,"Missing source line for station %s\n", station_name);
	    fflush (info);
	    continue;
	}
	if (s->dir[0] == '\0') {
	    fprintf (info,"Missing dir line for station %s\n", station_name);
	    fflush (info);
	    continue;
	}

	/* Open the station config file, and look for source section.	*/
	sprintf (station_cfg_file, "%s/%s", s->dir, STATION_INI_FILE);
	if (open_cfg(&station_cfg,station_cfg_file,s->source)) {
	    fprintf (info, "Unable to find section %s in station file %s\n",
		     s->source, s->dir);
	    fflush (info);
	    close_cfg(&station_cfg);
	    continue;
	}

	/* Extract required info in source section of this station.	*/
	while (1) {
	    read_cfg(&station_cfg,str1,str2);
	    if (str1[0] == '\0') break;
	    if (strcmp(str1,"SEGID")==0) {
		s->server_segment = atoi(str2);
	    }
	}
	if (s->server_segment == 0) {
	    fprintf (info, "Missing segid line in [%s] for station %s\n",
		     s->source, s->station);
	    fflush (info);
	    close_cfg(&station_cfg);
	    continue;
	}

	/* Skip to and read our own named section.			*/
	if (skipto(&station_cfg,CLIENT_NAME)) {
	    fprintf (info, "No client section [%s] for station %s\n",
		     CLIENT_NAME, station);
	    fflush (info);
	    close_cfg(&station_cfg);
	    continue;
	}
	while (1) {
	    read_cfg(&station_cfg,str1,str2);
	    if (debug(DEBUG_CONFIG)) {
		fprintf (info, "Config entry: %s=%s\n", str1, str2);
		fflush (info);
	    }
	    if (str1[0] == '\0') break;
	    if (strcmp(str1,"STATE")==0) {
		s->config_state = toupper(str2[0]);
	    }
	    else if (strcmp(str1,"SERVER")==0) {
		strcpy(s->program,str2);
	    }
	    else if (strncmp(str1,"CLIENT",6)==0) {
		if ((p = strchr(str2,',')) != NULL) {
		    *p = '\0';
		    c = find_client_entry (s, str2, 1);
		    strcpy(c->program, ++p);
		}
	    }
	    else if (strncmp(str1,"INCLIENT",8)==0) {
		if ((p = strchr(str2,',')) != NULL) {
		    *p = '\0';
		    c = find_client_entry (s, str2, 1);
		    strcpy(c->program, ++p);
		    c->input_client = 1;
		}
	    }
	    else if (strcmp(str1,"SERVER_STARTUP_DELAY")==0) {
		s->server_startup_delay = atoi(str2);
	    }
	    else if (strcmp(str1,"CLIENT_STARTUP_DELAY")==0) {
		s->client_startup_delay = atoi(str2);
	    }
	    else if (strcmp(str1,"MAX_SHUTDOWN_WAIT")==0) {
		s->max_shutdown_wait = atoi(str2);
	    }
	    else if (strcmp(str1,"MIN_NOTIFY")==0) {
		s->min_notify = atoi(str2);
	    }
	    else if (strcmp(str1,"RE_NOTIFY")==0) {
		s->re_notify = atoi(str2);
	    }
	    else if (strcmp(str1,"RES_NOTIFY")==0) {
		s->res_notify = atoi(str2);
	    }
	    else if (strcmp(str1,"NOTIFY_PROG")==0) {
		strcpy(s->notify_prog,str2);
	    }
	}
	if (s->program[0] == '\0') {
	    fprintf (info,"Missing server line for station %s\n", s->station);
	    fflush (info);
	    close_cfg(&station_cfg);
	    continue;
	}
	/* Pick up defaults from global network configuration.		*/
	if (s->server_startup_delay < 0) 
	    s->server_startup_delay = net_server_startup_delay;
	if (s->client_startup_delay < 0) 
	    s->client_startup_delay = net_client_startup_delay;
	if (s->max_shutdown_wait < 0) 
	    s->max_shutdown_wait = net_max_shutdown_wait;
	if (s->min_notify < 0) 
	    s->min_notify = net_min_notify;
	if (s->re_notify < 0) 
	    s->re_notify = net_re_notify;
	if (s->res_notify < 0) 
	    s->res_notify = net_res_notify;
	if (strlen(s->notify_prog) <= 0)
	    strcpy (s->notify_prog, net_notify_prog);

	/* Check for minimum values for daemon operation. */
	if (daemon_flag) {
	    if (s->server_startup_delay < MIN_POLL) {
		s->server_startup_delay = MIN_POLL;
		fprintf (info, "%s Warning: SERVER_STARTUP_DELAY reset to %d for %s\n", 
			 localtime_string(dtime()), s->server_startup_delay, s->station);
	    }
	    if (s->client_startup_delay < MIN_POLL) {
		s->client_startup_delay = MIN_POLL;
		fprintf (info, "%s Warning: CLIENT_STARTUP_DELAY reset to %d for %s\n", 
			 localtime_string(dtime()), s->client_startup_delay, s->station);
	    }
	    if (s->max_shutdown_wait < MIN_MAX_SHUTDOWN) {
		s->max_shutdown_wait = MIN_MAX_SHUTDOWN;
		fprintf (info, "%s Warning: MAX_SHUTDOWN_WAIT reset to %d for %s\n", 
			 localtime_string(dtime()), s->max_shutdown_wait, s->station);
	    }
	}

	/* For each client, find its section and get its pidfile path.	*/
	for (c=s->client; c!=NULL; c=c->next) {
	    if (skipto(&station_cfg,c->client)) {
		fprintf (info, "No client section [%s] for station %s\n",
			 c->client, station);
		fflush (info);
		continue;
	    }
	    while (1) {
		read_cfg(&station_cfg,str1,str2);
		if (str1[0] == '\0') break;
		if (strcmp("PIDFILE",str1)==0) {
		    strcpy(c->pidfile,str2);
		}
	    }
	    if (c->pidfile[0] != '\0') {
		FILE *fp;
		int pid;
		c->pid = -1;
		if ((fp = fopen(c->pidfile, "r")) != NULL) {
		    fscanf(fp, "%d", &pid);
		    c->pid = pid;
		    fclose(fp);
		}
	    }
	}
	close_cfg(&station_cfg);
	if (strcmp(station_name,"*") != 0) break;
    }
    close_cfg(&master_cfg);
    return (s);
}

/************************************************************************
 *  find_station_entry:
 *	Find existing entry or create new entry for station.
 *	List is in alphabetic order by station name.
 ************************************************************************/
STATION_INFO *find_station_entry (char *station, int create_flag)
{
    STATION_INFO *s = st_head;
    STATION_INFO *p = NULL;

    while (s != NULL && strcasecmp(station,s->station)>=0) {
	if (strcasecmp(station,s->station)==0) return (s);
	p = s;
	s = s->next;
    }
    if (! create_flag) return (NULL);

    /* Station not found.  Allocate new station structure.		*/
    if (debug(DEBUG_MALLOC)) {
	fprintf (info, "%s %s (%d) malloc station_info for %s\n", 
		 localtime_string(dtime()), cmdname, getpid(), station);
    }
    if ((s = (STATION_INFO *)malloc(sizeof(STATION_INFO))) == NULL) {
	fprintf (info, "Error mallocing station_info\n");
	exit(1);
    }
    memset ((char *)s, 0, sizeof(STATION_INFO));
    strcpy(s->station,station);
    upshift(s->station);
    s->config_state = s->present_state = s->target_state = '\0';
    s->status = CSCR_STATUS_UNKNOWN;
    s->server_startup_delay = -1;
    s->client_startup_delay = -1;
    s->max_shutdown_wait = -1;
    s->min_notify = -1;
    s->re_notify = -1;
    s->res_notify = -1;
    s->pid = -1;
    if (p == NULL) {
	/* Put at head of the list. */
	s->next = st_head;
	st_head = s;
    }
    else {
	s->next = p->next;
	p->next = s;
    }
    return (s);
}

/************************************************************************
 *  find_client_entry:
 *	Find existing entry or create new entry for client.
 *	List is in alphabetic order by client name.
 ************************************************************************/
CLIENT_INFO *find_client_entry (STATION_INFO *s, char *client, int create_flag)
{
    CLIENT_INFO *c = s->client;
    CLIENT_INFO *p = NULL;

    while (c != NULL && strcmp(client,c->client)<=0) {
	if (strcmp(client,c->client)==0) return (c);
	p = c;
	c = c->next;
    }
    if (! create_flag) return (NULL);

    /* Client not found.  Allocate new client structure.		*/
    if (debug(DEBUG_MALLOC)) {
	fprintf (info, "%s %s (%d) malloc client_info for %s\n", 
		 localtime_string(dtime()), cmdname, getpid(), client);
    }
    if ((c = (CLIENT_INFO *)malloc(sizeof(CLIENT_INFO))) == NULL) {
	fprintf (info, "Error mallocing client_info\n");
	exit(1);
    }
    memset((char *)c, 0, sizeof(CLIENT_INFO));
    strcpy(c->client,client);
    upshift(c->client);
    c->status = CSCR_STATUS_UNKNOWN;
    if (p == NULL) {
	/* Put at head of the list. */
	c->next = s->client;
	s->client = c;
    }
    else {
	c->next = p->next;
	p->next = c;
    }
    ++(s->nclients);
    return (c);
}

/************************************************************************
 *  dump_stations:
 *	Dump station info (for diagnostic or monitoring purposes).
 ************************************************************************/
void dump_stations()
{
    STATION_INFO *s;
    for (s = st_head; s != NULL; s =s->next) {
	if (s->config_state == 'I') continue;
	dump_station(s);
    }
}

/************************************************************************
 *  dump_station:
 *	Dump station info (for diagnostic or monitoring purposes).
 ************************************************************************/
void dump_station (STATION_INFO *s)
{
    fprintf (info, "\nstation=%s config_state=%c target_state=%c present_state=%c\n",
	     s->station, s->config_state, s->target_state, s->present_state);
    fprintf (info, "    server=%s pid=%d status=%s\n", s->source, s->pid, 
	     (s->status == 0 && s->time_timeout_started > 0) ? "TIMED_OUT" :
	     (s->status<0) ? "Unknown" : stats[s->status]);

    if (verbose) {
	fprintf (info, "\tconfig_dir=%s\n", s->dir);
	fprintf (info, "\tprogram=%s\n", s->program);
	fprintf (info, "\tseg=%d [0x%x] monitored_clients=%d\n",
		 s->server_segment, s->server_segment, s->nclients);
	if (s->linkstat) {
	    char s1[80];
	    linkstat_rec *pls = s->linkstat;
	    /* Code for printing the link status was lifted from dpda.c	*/
	    /* Changed to print out local time instead of UCT time.	*/
	    char7 formats[3] = { "QSL", "Q512", "MM256" } ;
	    fprintf (info, "\tUltra=%d, Link Recv=%d, Ultra Recv=%d, Suspended=%d, Data Format=%s\n",
		     pls->ultraon, pls->linkrecv, pls->ultrarecv, pls->suspended,
		     (char *)formats[pls->data_format]) ;
	    fprintf (info, "\tTotal Packets=%d, Sync Packets=%d, Sequence Errors=%d\n",
		     pls->total_packets, pls->sync_packets, pls->seq_errors) ;
	    fprintf (info, "\tChecksum Errors=%d, IO Errors=%d, Last IO Error=%d (%s)\n",
		     pls->check_errors, pls->io_errors, pls->lastio_error,
		     strerror(pls->lastio_error)) ;
	    fprintf (info, "\tBlocked Packets=%d, Seed Format=%5.5s, Seconds in Operation=%d\n",
		     pls->blocked_packets, pls->seedformat, pls->seconds_inop) ;
	    if (pls->last_good > 1)
		fprintf (info, "\tLast Good Packet Received at=%s\n", localtime_string(pls->last_good)) ;
	    if (pls->last_bad > 1)
		fprintf (info, "\tLast Bad Packet Received at=%s\n", localtime_string(pls->last_bad)) ;
	    strpcopy (s1, pls->description) ;
	    fprintf (info, "\tStation Description=%s\n", (char *)s1) ;
	    /* End of code lifted from dpda.c */
	}
    }
}

/************************************************************************
 *  dump_clients:
 *	Dump client list (for diagnostic or monitoring purposes).
 ************************************************************************/
void dump_clients (STATION_INFO *s)
{
    CLIENT_INFO *c;
    for (c=s->client; c!=NULL; c=c->next) dump_client(s,c);
}

/************************************************************************
 *  dump_client:
 *	Dump client info (for diagnostic or monitoring purposes).
 ************************************************************************/
void dump_client (STATION_INFO *s, CLIENT_INFO *c)
{
    fprintf (info, "    client=%s pid=%d status=%s\n",
	     c->client, c->pid, (c->status<0) ? "Unknown" : cstats[c->status]);
    if (verbose) {
	fprintf (info, "\tpidfile=%s\n\tprogram=%s\n",
		 c->pidfile, c->program);
    }
}

/************************************************************************
 *  server_status:
 *	Check on the status of the server.
 *	Return extended comserv status.
 ************************************************************************/
int server_status (STATION_INFO *s)
{
    pclient_station this;
    comstat_rec *pcomm ;
    int status, err;
    int i;
    double now;

    now = dtime();
    if (now < s->time_check) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "skipping station %s - %.0lf more seconds\n",
		     s->station, s->time_check - now);
	status = CSCR_STATUS_UNKNOWN;
	s->status = status;
	return(status);
    }

    if ((i = station_index(s->station)) < 0) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Warning - %-4.4s not found\n", s->station);
	status = CSCR_DIED;
	s->status = status;
	return (status);
    }

    this = (pclient_station) ((intptr_t) me + me->offsets[i]);
    status = cs_check (me, i, now);
    pcomm = (pvoid) ((intptr_t) me + this->comoutoffset) ;
    pcomm->completion_status = CSCS_IDLE;

    if (debug(DEBUG_STATUS))
	fprintf (info, "%-4.4s server_status check - status = %d\n", 
		 s->station, status);
    if (status == CSCR_NODATA) status = CSCR_GOOD;
    /*:: Cannot convert CSCR_CHANGE to CSCR_GOOD.  */
    /*:: if (status == CSCR_CHANGE) status = CSCR_GOOD; */
    s->status = status;

    if (status == CSCR_DIED) {
	cs_detach(me, i);
	if (debug(DEBUG_STATUS))
	    fprintf (info, "%-4.4s server died\n", s->station);
	s->pid = -1;
	return(status);
    }

    if (status == CSCR_GOOD) {
	s->pid = (this->base)->server_pid;
	this->command = CSCM_LINKSTAT;
	err = cs_svc(me, i) ;
	if (err == CSCR_GOOD) {
	    if (s->linkstat == NULL) {
		if (debug(DEBUG_MALLOC)) {
		    fprintf (info, "%s %s (%d) malloc linkstat_rec for %s\n", 
			     localtime_string(dtime()), cmdname, getpid(),
			     s->station);
		}
		if ((s->linkstat = (linkstat_rec *)malloc(sizeof(linkstat_rec))) == NULL) 
		    fprintf (info, "Unable to malloc linkstat_rec for %s\n", s->station);
	    }
	    if (s->linkstat != NULL) {
		memcpy ((void *)s->linkstat, (void *)&pcomm->moreinfo, sizeof(linkstat_rec)) ;
		check_station_telemetry(s);
	    }
	}
	else {
	    fprintf (info, "%s %s (%d) Unable to get linkstat for %s, status=%d\n",
		     localtime_string(dtime()), cmdname, getpid(), s->station, err);
	    fflush (info);
	    if (s->linkstat != NULL) {
		if (debug(DEBUG_MALLOC)) {
		    fprintf (info, "%s %s (%d) free linkstat_rec for %s\n", 
			     localtime_string(dtime()), cmdname, getpid(), s->station);
		}
		free (s->linkstat);
		s->linkstat = NULL;
	    }
	}
	pcomm->completion_status = CSCS_IDLE;
    }
    else {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "%-4.4s Warning %d getting status from server - %s\n",
		     s->station, status, stats[status]);
    }
    return (status);
}

/************************************************************************
 *  client_status:
 *	Check on the status the specified client for the station.
 *	Return extended comserv status.
 ************************************************************************/
int client_status (STATION_INFO *s, CLIENT_INFO *c)
{
    pclient_station this;
    comstat_rec *pcomm ;
    client_info *pci ;
    one_client *poc ;
    char *client_name;
    int client_found = 0;
    int status;
    int i, j;
    double now;

    c->status = status = CSCR_STATUS_UNKNOWN;
    now = dtime();
    if (now < s->time_check) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "skipping station %s - %.0lf more seconds\n",
		     s->station, s->time_check - now);
	return(c->status);
    }
    if (c->time_check > now) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "skipping client %s - %.0lf more seconds\n",
		     c->client, c->time_check - now);
	return(c->status);
    }

    if ((i = station_index(s->station)) < 0) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Warning - %-4.4s not found\n", s->station);
	return (c->status);
    }

    this = (pclient_station) ((intptr_t) me + me->offsets[i]);
    pcomm = (pvoid) ((intptr_t) me + this->comoutoffset) ;

    /* Check server status first. */
    status = cs_check (me, i, now);
    if (status == CSCR_NODATA) status = CSCR_GOOD;
    if (status != CSCR_GOOD) return (c->status);

    /* Get client info from server.					*/
    this->command = CSCM_CLIENTS;
    status = cs_svc (me, i);
    pcomm->completion_status = CSCS_IDLE;
    if (status != CSCR_GOOD) return (c->status);

    /* Find client in server client list.				*/
    pci = (client_info *)&pcomm->moreinfo;
    for (j=0; j<pci->client_count && !client_found; j++) {
	poc = &(pci->clients[j]);
	client_name = cname_str_cs(poc->client_name);
	if (strcasecmp(c->client,client_name)==0) {
	    ++client_found;
	    break;
	}
    }
    if (! client_found) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Warning: %-4.4s client %s not registered\n", 
		     s->station, c->client);
	c->status = CLIENT_DEAD;
	return (c->status);
    }
    c->block_count = poc->block_count;

    /* Perform various checks to ensure client is alive and well.	*/
    if (! poc->active || (poc->client_pid <= 0)) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Client inactive or bad client PID\n");
	c->status = CLIENT_DEAD;
	return (c->status);
    }
    if (poc->client_pid != c->pid) {
	if (c->pid != -1 && debug(DEBUG_STATUS)) {
	    fprintf (info, "Warning = server says pid = %d, pidfile says pid = %d\n",
		     poc->client_pid, c->pid);
	    fprintf (info, "Updating cached client pid\n");
	}
	c->pid = poc->client_pid;
    }
    status = kill (c->pid, 0);
    if (status != 0) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Warning: %-4.4s client %s pid=%d not found\n",
		     s->station, c->client, c->pid);
	c->status = CLIENT_DEAD;
    }
    else { 
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Client %s for server %s found\n", c->client, s->station);
	c->status = CLIENT_GOOD;
    }
    return (c->status);
}

#define	MAXARGS		32
/************************************************************************
 *  spawn_process:
 *	Spawn a new process.
 ************************************************************************/
int spawn_process (char *prog, char *station)
{
    int pid, pid2;
    char *p;
    char path[2048];
    char tmpstr[2048];
    char *argv[MAXARGS+1];
    char *progname;
    char logfile[2048];
    char cmd[2048];
    int n = 0;
    int fd;

    fprintf (info, "%s Spawning for %s: %s %s\n", 
	     localtime_string(dtime()), station, prog, station);
    /* Create the server command string and parse it.			*/
    sprintf (cmd, "%s %s", prog, station);
    p = strtok(cmd," ");
    if (p == NULL) {
	fprintf (info, "Empty server program for %s\n", station);
	return(-1);
    }
    strcpy(path, p);

    /* Create argv[0] with program name.				*/
    strcpy(tmpstr,path);
    progname = basename(tmpstr);
    argv[n++] = progname;
    sprintf (logfile, "%s/%s.%s.log",logdir,station,argv[0]);

    /* Tokenize the rest of the command line. */
    while ((p=strtok(NULL," ")) != NULL) {
	if (n >= MAXARGS) {
	    fprintf (info, "max args exceeded\n");
	    return(-1);
	}
	argv[n++] = p;
    }
    argv[n] = 0;

    fflush (info);
    pid = fork();
    switch (pid) {
    case 0:		    /* child process				*/
	pid2 = fork();
	switch (pid2) {
	case 0:	    /* grandchild				*/
	    fd = open (logfile, O_WRONLY|O_APPEND|O_CREAT,0666);
	    close(0);
	    fclose(stdout);
	    fclose(stderr);
	    dup2(fd,1);
	    dup2(fd,2);
	    close(fd);
	    /* Close any misc units left open by libraries...		*/
	    for (fd=3;fd<16;fd++) close(fd);
	    /* execve (path, argv, new_environ); */
	    execv (path, argv);  /* preserve existing environment */
	    perror("vfork");
	    _exit(-1);
	    break;
	case -1:
	    perror("vfork");
	    _exit(-1);
	default:	    /* child process.				*/
	    _exit(0);
	    break;
	}
	break;
    case -1:
	perror("fork");
	return (-1);
	break;
    default: {	    /* Parent process.				*/
	int ret, status;
	do
	    ret = wait(&status);
	while ((ret != pid) && (ret != -1));
	if (ret == -1) {
	    perror("wait");
	    return (-1);
	}
	return(WEXITSTATUS(status));
    }
    }
}

/************************************************************************
 *  write_request:
 *	Write request file for monitor daemon.
 ************************************************************************/
int write_request(char *station, char *op)
{
    FILE *fp;
    char file[2048];
    sprintf (file, "%s/%s", cmddir, station);
    if ((fp=fopen(file,"w"))==NULL) {
	fprintf (info, "Unable to open for writing: %s\n", file);
	return(-1);
    }
    fprintf (fp, "%s\n", op);
    fclose (fp);
    fprintf (info, "%s request queued for %s\n", op, station);
    return (0);
}

/************************************************************************
 *  print_syntax:
 *	Print the syntax description of program.
 ************************************************************************/
int print_syntax (char	*cmd,		/* program name.			*/
		  char	*syntax[],	/* syntax array.			*/
		  FILE	*fp)		/* FILE ptr for output.			*/
{
    int i;
    for (i=0; syntax[i] != NULL; i++) {
	fprintf (fp, syntax[i], cmd);
	fprintf (fp, "\n");
    }
    return (0);
}

/************************************************************************
 *  shutdown_station:
 *	Shutdown server and known blocking clients for station.
 *	Allow asynchronous shutdowns.
 ************************************************************************/
int shutdown_station (STATION_INFO *s)
{
    CLIENT_INFO *c;
    pclient_station this;
    comstat_rec *pcomm ;
    double now;
    int i, err;
    int status = 0;
    int ok;
    int block_count;
    int clients_alive;
    int n;

    if (s->present_state == 'N') {
	s->shutting_down = 0;
	s->time_complete = s->time_check = 0;
	return(FULL_SHUTDOWN);
    }

    status = server_status(s);
    switch (status) {
    case CSCR_GOOD:		/* Continue with shutdown.		*/
	break;
    case CSCR_STATUS_UNKNOWN:	/* Unknown server state.		*/
    case CSCR_CHANGE:		/* Server in transistion state up.	*/
	/* Defer shutdown. */
	return (PARTIAL_SHUTDOWN);
	break;
    case CSCR_DIED:		/* Server is already dead.		*/
    default:			/* Unexpected status - mark shutdown.	*/	
	s->shutting_down = 0;
	s->time_complete = s->time_check = 0;
	return (FULL_SHUTDOWN);
	break;
    }

    /* Terminate all managed input clients.				*/
    if (s->shutting_down == 0) {
	n = 0;
	now = dtime();
	for (c=s->client; c!=NULL; c=c->next) {
	    if (! c->input_client) continue;
	    n++;
	    if (c->pid > 1) {
		if ((err=kill(c->pid,SIGINT)) == 0) {
		    fprintf (info, "%s Terminated client %s for %s\n",
			     localtime_string(dtime()), c->client, s->station);
		    c->status = CSCR_STATUS_UNKNOWN;
		}
		else {
		    fprintf (info, "%s Unable to terminate client %s for %s - kill error=%d\n",
			     localtime_string(dtime()), c->client, s->station, err);
		    c->status = CSCR_STATUS_UNKNOWN;
		}
	    }
	}
	s->shutting_down = 1;
	if (n > 0) {
	    s->time_check = now + s->client_startup_delay;
	    s->time_complete = now + s->max_shutdown_wait;
	    return (PARTIAL_SHUTDOWN);
	}
    }
    
    /* Wait until all input clients die (at most max_shutdown_wait).	*/
    /* Mark them as DEAD once we determine that they actually died.	*/
    clients_alive = 0;
    if (s->shutting_down == 1) {
	now = dtime();
	for (c=s->client; c!=NULL; c=c->next) {
	    if (! c->input_client) continue;
	    if (c->pid > 0) {
		if (kill(c->pid,0) == 0) {
		    ++clients_alive;
		}
		else {
		    c->status = CLIENT_DEAD;
		    c->pid = -1;
		}
	    }
	}
	if (clients_alive && now < s->time_complete) return (PARTIAL_SHUTDOWN);
	if (clients_alive > 0) {
	    fprintf (info, "%s Warning - %d input clients for %s still alive\n",
		     localtime_string(dtime()), clients_alive, s->station);
	}
	s->shutting_down = 2;
    }

    /* Suspend the server's input link.					*/
    if (s->shutting_down == 2) {
	err = suspend_link(s);
	if (err != CSCR_GOOD && err != CSCR_INVALID) {
	    s->shutting_down = 0;
	    status = (err == CSCR_DIED) ? FULL_SHUTDOWN : ERROR_SHUTDOWN;
	    s->status = err;
	    return (status);
	}
	s->time_check = now + s->client_startup_delay;
	s->time_complete = now + s->max_shutdown_wait;
	s->shutting_down = 3;
	status = PARTIAL_SHUTDOWN;
	return (status);    /* allow time before killing clients. */
    }

    /* Query all managed clients to ensure that they have no packets	*/
    /* blocked for them.  Wait at most max_shutdown_wait seconds before	*/
    /* terminating the server.						*/
    if (s->shutting_down == 3) {
	now = dtime();
	block_count = 0;
	for (c=s->client; c!=NULL; c=c->next) {
	    if ((ok = client_status(s,c)) == CLIENT_GOOD && c->block_count)
		++block_count;
	}
	if (block_count > 0 && now < s->time_complete) return (PARTIAL_SHUTDOWN);
	if (block_count > 0) {
	    fprintf (info, "%s Warning - %d clients for %s still had blocked packets\n",
		     localtime_string(dtime()), block_count, s->station);
	}
	s->shutting_down = 4;
    }

    /* Terminate all managed clients.					*/
    if (s->shutting_down == 4) {
	for (c=s->client; c!=NULL; c=c->next) {
	    if (c->pid > 1) {
		if ((err=kill(c->pid,SIGINT)) == 0) {
		    fprintf (info, "%s Terminated client %s for %s\n",
			     localtime_string(dtime()), c->client, s->station);
		    c->status = CSCR_STATUS_UNKNOWN;
		}
		else {
		    fprintf (info, "%s Unable to terminate client %s for %s - kill error=%d\n",
			     localtime_string(dtime()), c->client, s->station, err);
		    c->status = CSCR_STATUS_UNKNOWN;
		}
	    }
	}
	s->time_check = now + s->client_startup_delay;
	s->time_complete = now + s->max_shutdown_wait;
	s->shutting_down = 5;
    }

    /* Wait until all clients actually die (at most max_shutdown_wait).	*/
    /* Mark them as DEAD once we determine that they actually died.	*/
    if (s->shutting_down == 5) {
	now = dtime();
	clients_alive = 0;
	for (c=s->client; c!=NULL; c=c->next) {
	    if (c->pid > 0) {
		if (kill(c->pid,0) == 0) {
		    ++clients_alive;
		}
		else {
		    c->status = CLIENT_DEAD;
		    c->pid = -1;
		}
	    }
	}
	if (clients_alive && now < s->time_complete) return (PARTIAL_SHUTDOWN);
	if (clients_alive > 0) {
	    fprintf (info, "%s Warning - %d clients for %s still alive\n",
		     localtime_string(dtime()), clients_alive, s->station);
	}
	s->shutting_down = 6;
    }

    /* Terminate the server.						*/
    if (s->shutting_down == 6) {
	s->time_check = now + s->server_startup_delay;
	s->time_complete = s->time_check;
	if ((i = station_index(s->station)) < 0) {
	    fprintf (info, "%s Warning - %-4.4s not found\n", 
		     localtime_string(dtime()), s->station);
	    s->shutting_down = 0;
	    return (FULL_SHUTDOWN);
	}
	this = (pclient_station) ((intptr_t) me + me->offsets[i]);
	pcomm = (pvoid) ((intptr_t) me + this->comoutoffset) ;
	this->command = CSCM_TERMINATE;
	err = cs_svc(me, i);
	if (err == CSCR_DIED) {
	    fprintf (info, "%s Tried to terminate dead server %s - status=%s\n",
		     localtime_string(dtime()), s->station, stats[err]);
	    s->time_check = s->time_complete = 0;
	    status = FULL_SHUTDOWN;
	}
	else if (err == CSCR_GOOD) {
	    fprintf (info, "%s Terminated server for %s\n", 
		     localtime_string(dtime()), s->station);
	    s->status = CSCR_DIED;	/* Assume that it has died.	*/
	    status = FULL_SHUTDOWN;
	}
	else {
	    fprintf (info, "%s Unable to terminate server for %s - status=%s\n", 
		     localtime_string(dtime()), s->station, stats[err]);
	    status = ERROR_SHUTDOWN;
	}
	pcomm->completion_status = CSCS_IDLE;
	s->shutting_down = 0;
	return (status);
    }

    /* Unknown state.	*/
    fprintf (info, "%s Unknown termination for %s - state=%d\n",
	     localtime_string(dtime()), s->station, s->shutting_down);
    s->shutting_down = 0;
    return (FULL_SHUTDOWN);
}

/************************************************************************
 *  startup_station:
 *	Startup server and known blocking clients for station.
 *	Allow asynchronous startups.
 *	Return station state.
 ************************************************************************/
int startup_station (STATION_INFO *s)
{
    CLIENT_INFO *c;
    int status;
    double now;

    s->shutting_down = 0;
    
    status = server_status(s);
    now = dtime();
    switch (status) {
    case CSCR_DIED:
	s->time_check = now + s->server_startup_delay;	
	s->time_complete = s->time_check;
	fprintf (info, "%s Spawning server for %s\n", 
		 localtime_string(now), s->station);
	fflush (info);
	spawn_process (s->program, s->station);
	if (s->linkstat != NULL) {
	    if (debug(DEBUG_MALLOC)) {
		fprintf (info, "%s %s (%d) free linkstat_rec\n", 
			 localtime_string(dtime()), cmdname, getpid());
	    }
	    free (s->linkstat);
	    s->linkstat = NULL;
	}
	return (PARTIAL_STARTUP);
	break;
    case CSCR_GOOD:
	if (s->linkstat && s->linkstat->suspended) 
	    status = resume_link(s);
	if (status != CSCR_GOOD) return (PARTIAL_STARTUP);
	break;
    case CSCR_STATUS_UNKNOWN:
    default:
	return (PARTIAL_STARTUP);
	break;
    }

    for (c=s->client; c!= NULL; c=c->next) {
	status = client_status(s,c);
	if (status != CLIENT_GOOD && now >= s->time_check && now >= c->time_check) {
	    c->time_check = now + s->client_startup_delay;
	    s->time_complete = c->time_check;
	    fprintf (info, "%s Spawning client %s for %s\n", 
		     localtime_string(dtime()), c->client, s->station);
	    fflush (info);
	    spawn_process (c->program, s->station);
	}
    }
    return(FULL_STARTUP);
}

/************************************************************************
 *  station_index:
 *	Return index of station for multi-station connections.
 ************************************************************************/
int station_index (char *s)
{
    int i;
    pclient_station this;
    char *station_name;

    /* One-time setup to get server segment info.			*/
    /* Set global ptr me.						*/
    if (me == NULL) {
	int data_mask = CSIM_MSG;
	tstations_struc stations; 
	cs_setup (&stations, CLIENT_NAME, "*", TRUE, FALSE, 10, 1,
		  data_mask, 6000);
	me = cs_gen(&stations);
    }

    /* Loop through each station looking for the station we want.	*/
    for (i=0; i< me->maxstation; i++) {
	this = (pclient_station) ((intptr_t) me + me->offsets[i]);
	station_name = sname_str_cs(this->name);
	if (strcasecmp(s,station_name)==0) return (i);
    }
    return (-1);
}

/************************************************************************
 *  station_state:
 *	Determine the state of station from server and client status.
 *	If server is down, then station is considered down (N).
 *	If server is up and all clients are up, then station is up (R).
 *	If the server is neither good nor dead, then server is
 *	undetermined ('U').
 *	If server undetermined, or 1 or more clients are down or
 *	    undetermined, then station is undetermined ('U').
 ************************************************************************/
char station_state (STATION_INFO *s) 
{
    CLIENT_INFO *c;
    char state = 0;
    char *reason = "unknown";
    char msg[80];
    
    if (s->config_state == 'I') {
	if debug(DEBUG_STATE) {
		fprintf (info, "State of %s = <null>, reason = %s\n", 
			 s->station, reason);
	    }
	return (0);
    }

    /* Check server status.						*/
    switch (s->status) {
    case CSCR_NODATA:
    case CSCR_CHANGE:
    case CSCR_GOOD:	state = 0; break;	/* Go check clients.	*/
    case CSCR_DIED:	state = 'N'; reason = "server dead"; break;	/* Server dead -> N.	*/
    default:	state = 'U'; 
	sprintf (msg, "unknown server status = %d", s->status);
	reason = msg; break;	/* Unknown -> U.	*/
    }

    /* Check clients if no state assigned yet.				*/
    for (c=s->client; state==0 && c!=NULL; c=c->next) {
	switch (c->status) {
	case CLIENT_GOOD: continue;		/* Check other clients.	*/
	case CLIENT_DEAD: state = 'U'; reason = "client dead"; break;	/* Undetermined state.	*/
	default:	    state = 'U'; 
	    sprintf (msg, "unknown client status = %d", c->status);
	    reason = msg; break;	/* Unknown client -> U.	*/
	}
    }

    if (state == 0) {
	state = 'R';	/* Server & all clients up -> R.*/
	reason = "all checks ok";
    }
    if debug(DEBUG_STATE) {
	    fprintf (info, "State of %s = %c, reason = %s\n", s->station, state, reason);
	}
    return (state);
}

/************************************************************************
 *  suspend_link:
 *	Suspend the link.
 *	Return comserv status.
 ************************************************************************/
int suspend_link (STATION_INFO *s) 
{
    int i, err;
    pclient_station this;
    comstat_rec *pcomm;

    if ((i = station_index(s->station)) < 0) {
	fprintf (info, "%s Warning - %-4.4s not found\n", 
		 localtime_string(dtime()), s->station);
	s->status = CSCR_DIED;
	return (CSCR_DIED);
    }

    this = (pclient_station) ((intptr_t) me + me->offsets[i]);
    pcomm = (pvoid) ((intptr_t) me + this->comoutoffset) ;
    this->command = CSCM_SUSPEND;
    err = cs_svc(me, i);
    pcomm->completion_status = CSCS_IDLE;
    if (err == CSCR_GOOD)
	fprintf (info, "%s Suspended link for %s\n", localtime_string(dtime()), 
		 s->station);
    else 
	fprintf (info, "%s Error Suspending link for %s\n", localtime_string(dtime()),
		 s->station);
    return (err);
}

/************************************************************************
 *  resume_link:
 *	Resume the link.
 *	Return comserv status.
 ************************************************************************/
int resume_link (STATION_INFO *s) 
{
    int i, err;
    pclient_station this;
    comstat_rec *pcomm;

    if ((i = station_index(s->station)) < 0) {
	if (debug(DEBUG_STATUS))
	    fprintf (info, "Warning - %-4.4s not found\n", s->station);
	s->status = CSCR_DIED;
	return (CSCR_DIED);
    }

    this = (pclient_station) ((intptr_t) me + me->offsets[i]);
    pcomm = (pvoid) ((intptr_t) me + this->comoutoffset);
    this->command = CSCM_RESUME;
    err = cs_svc(me, i);
    pcomm->completion_status = CSCS_IDLE;
    if (err == CSCR_GOOD)
	fprintf (info, "%s Resumed link for %s\n", localtime_string(dtime()), 
		 s->station);
    else 
	fprintf (info, "%s Error Resuming link for %s\n", localtime_string(dtime()),
		 s->station);
    return (err);
}

/************************************************************************
 *  process_requests:
 *	Process any startup, shutdown, or reconfig requests.
 *	Return number of requests processed.
 ************************************************************************/
int process_requests()
{
    STATION_INFO *s;
    struct stat sb;
    struct dirent *dirent;
    DIR *dirp;
    int new_requests = 0;
    struct passwd *pwent;
    double now;

    if ((dirp = opendir(cmddir)) != NULL) {
	char path[2048];
	while ((dirent = readdir(dirp)) != NULL && ! reconfig_flag) {
	    sprintf (path, "%s/%s", cmddir, dirent->d_name);
	    if (stat(path,&sb)==0 && S_ISREG(sb.st_mode)) {
		FILE *fp;
		char line[80];
		if ((s=find_station_entry(dirent->d_name, 0)) == NULL &&
		    strcasecmp(dirent->d_name,CLIENT_NAME) != 0) {
		    fprintf (info, "Cmd file for unknown station %s\n",
			     dirent->d_name);
		    unlink(path);
		    continue;
		}
		now = dtime();
		if ((fp=fopen(path,"r")) != NULL && 
		    fgets (line,79,fp) != NULL) {
		    line[79] = '\0';
		    line[strlen(line)-1] = '\0';
		    setpwent();
		    pwent = getpwuid(sb.st_uid);
		    endpwent();
		    if (strcasecmp(line,STARTUP_STRING) == 0) {
			/* Re-collect info for this station, in case of change. */
			if ((s=collect_station_info (dirent->d_name))) {
			    s->queued_request = STARTUP_REQUEST;
			    s->queued_request_time = now;
			    ++new_requests;
			    fprintf (info, "%s Startup request for %s by %s",
				     localtime_string(now), s->station, pwent->pw_name);
			    if (s->config_state == 'N' || s->config_state == 'I') 
				fprintf (info, " - ignored for config_state=%c", s->config_state);
			    fprintf (info, "\n");
			}
			else {
			    fprintf (info, "%s Station %s not found during startup request\n", 
				     localtime_string(now), dirent->d_name);
			    break;
			}
		    }
		    else if (strcasecmp(line,SHUTDOWN_STRING) == 0) {
			if ((s=collect_station_info (dirent->d_name))) {
			    s->queued_request = SHUTDOWN_REQUEST;
			    s->queued_request_time = now;
			    ++new_requests;
			    fprintf (info, "%s Shutdown request for %s by %s\n",
				     localtime_string(now), s->station, pwent->pw_name);
			}
			else {
			    fprintf (info, "%s Station %s not found during shutdown request\n", 
				     localtime_string(now), dirent->d_name);
			    break;
			}
		    }
#ifdef	ALLOW_RECONFIG
		    else if (strcasecmp(line,RECONFIG_STRING) == 0) {
			++reconfig_flag;
			++new_requests;
		    }
#endif	/* ALLOW_RECONFIG */
		    else {
			fprintf (info, "Invalid cmd for %s: %s",
				 dirent->d_name, line);
		    }
		    if (fp) fclose(fp);
		}
		unlink(path);
	    }
	}
	closedir(dirp);
    }
    else {
	fprintf (info, "Unable to open dir %s\n", cmddir);
    }
    return (new_requests);
}

/************************************************************************
 *  timeout_notify:
 *	Setup input info and run timeout notification program.
 ************************************************************************/
int timeout_notify(char *station, char *notify_prog, double timeout_started, 
		   double time_last_notified, double last_good, 
		   double now) 
{
    char namebuf[] = TMPFILE_PREFIX "netm" TMPFILE_SUFFIX;
    FILE *fp;
    int seconds, minutes, hours;
    int status = -1;

    seconds = now - timeout_started;
    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    fprintf (info, "%s Timeout for %s: %d hour %d minutes\n", 
	     localtime_string(now), station, hours, minutes);
    fflush (info);

    if ((fp = tmpfile_open(namebuf, "w")) != NULL) {
	char cmd[2048];
	pchar last_good_text = "Never";
	if (last_good > 0) {
	    last_good_text = localtime_string(last_good);
	}
	fprintf (fp, "************************************************\n");
	fprintf (fp, "Report from %s on host %s\n", cmdname, hostname);
	fprintf (fp, "Timeout: station %s for %d hours %d minutes\n",
		 station, hours, minutes);
	fprintf (fp, "Last good packet = %s\n", last_good_text);
	fprintf (fp, "Current time = %s\n", localtime_string(now));
	fprintf (fp, "************************************************\n");
	fclose(fp);
	sprintf (cmd, "%s < %s", notify_prog, namebuf);
	status = system (cmd);
    }
    return (status);
}

/************************************************************************
 *  resume_notify:
 *	Setup input info and run notification program.
 ************************************************************************/
int resume_notify(char *station, char *notify_prog, double timeout_started, 
		       double time_last_notified, double last_good, 
		       double now) 
{
    char namebuf[] = TMPFILE_PREFIX "netm" TMPFILE_SUFFIX;
    FILE *fp;
    int seconds, minutes, hours;
    int status = -1;

    seconds = now - timeout_started;
    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    fprintf (info, "%s Telemetry resumes for %s: %d hour %d minutes\n", 
	     localtime_string(now), station, hours, minutes);
    fflush (info);

    if ((fp = tmpfile_open(namebuf, "w")) != NULL) {
	    char cmd[2048];
            pchar timeout_started_text = "Startup";
            if (timeout_started > 0) {
                timeout_started_text = localtime_string(timeout_started);
            }
	    fprintf (fp, "************************************************\n");
	    fprintf (fp, "Report from %s on host %s\n", cmdname, hostname);
	    fprintf (fp, "Telemetry resumes: station %s after %d hours %d minutes\n",
		     station, hours, minutes);
	    fprintf (fp, "Timeout started = %s\n", timeout_started_text);
	    fprintf (fp, "Current time = %s\n", localtime_string(now));
	    fprintf (fp, "************************************************\n");
	    fclose(fp);
	    sprintf (cmd, "%s < %s", notify_prog, namebuf);
	    status = system (cmd);
    }
    return (status);
}

/************************************************************************
 *  check_station_telemetry:
 *	Check to ensure we still have good telemetry from station.
 *	If not, notify the higher authorities.
 ************************************************************************/
void check_station_telemetry(STATION_INFO *s)
{
    linkstat_rec *pls = s->linkstat;
    int timed_out = 0;
    double now = dtime();

    if (pls == NULL) return;
    timed_out = (s->min_notify >= 0 &&
		 ((pls->last_good <= 1 && pls->seconds_inop > s->min_notify) ||
		  (pls->last_good > 1 && now - pls->last_good > s->min_notify)));
    if (timed_out) {
	if (s->time_last_notified == 0 || 
	    (s->re_notify >= 0 && s->time_last_notified + s->re_notify <= now)) {
	    if (s->time_timeout_started == 0) {
		s->time_timeout_started = (pls->last_good > 1) ?
		    pls->last_good : now - pls->seconds_inop;
	    }
	    if (daemon_flag && strlen(s->notify_prog)>0) {
		int stat;
		stat = timeout_notify(s->station, s->notify_prog, s->time_timeout_started, 
				      s->time_last_notified, pls->last_good, now);
		if (stat == 0) s->time_last_notified = now;
	    }
	}
    }
    else {
	if (s->res_notify > 0 && s->time_last_notified != 0) {
	    /* Notify that station is back up.				*/
	    int stat;
	    stat = resume_notify(s->station, s->notify_prog, s->time_timeout_started, 
				 s->time_last_notified, pls->last_good, now);
	    if (stat) {
	    }
	}
	/* Cancel any current timeout.	*/
	s->time_last_notified = s->time_timeout_started = 0;
    }
}

/************************************************************************
 *  set_initial_states:
 *	Set initial states and actions for the station based on	the
 *	config_state, mode flag, and station state.
 ************************************************************************/
void set_initial_states (STATION_INFO *s, int flag)
{
    switch (flag) {
    case DAEMON_RESTART:	/* Daemon process in restart mode.	*/
    case DAEMON_BOOT:		/* Daemon process in boot mode.		*/
    case DAEMON_RECONFIG:	/* Runtime reconfiguration mode.	*/
    default:
	switch (s->config_state) {
	case 'A':	if (! s->target_state) s->target_state = 'A'; break;
	case 'S':	if (! s->target_state) s->target_state = 'R'; break;
	case 'R':	if (! s->target_state) s->target_state = 'I'; break;
	case 'N':	if (! s->target_state) s->target_state = 'N'; break;
	case 'I':	s->target_state = '\0'; break;
	}
	break;
    }
    s->present_state = station_state(s);
}

#ifdef	ALLOW_RECONFIG
/************************************************************************
 *  reconfig_daemon:
 *	Reconfigure the daemon.
 ************************************************************************/
void reconfig_daemon() 
{
    STATION_INFO *s;

    /* Determine if reconfiguration is allowed at this point.		*/
    /* If not, defer until later.					*/
    /* NOTE: No checks are currently implemented.  If you reconfig the	*/
    /* daemon while station startup or shutdown is in progress, all	*/
    /* bets are off.  CAVEAT EMPEATOR.					*/

    /* Signoff from servers if currently connected.			*/
    if (me) {
        cs_off (me) ;
	me = NULL;
    }
    /* Free all existing station structures.				*/
    while (st_head != NULL) {
	s = st_head;
	st_head = s->next;
	free (s);
    }
    /* Read network configuration file.					*/
    read_network_cfg (NETWORK_FILE, CLIENT_NAME);

    /* Reopen the logfile.						*/
    if (daemon_flag && log_to_file) {
	fflush (stdout);
	fflush (stderr);
	sprintf (logfile, "%s/%s.log", logdir,CLIENT_NAME);
	if (freopen (logfile, "a", stdout) == NULL) {
	    fprintf (stderr, "Unable to open logfile: %s\n", logfile);
	    exit(1);
	}
	fflush (stderr);
	dup2 (1,2);
	info = stderr;
    }

    fprintf (info, "%s Reconfig %s daemon pid %d\n", 
	     localtime_string(dtime()), cmdname, getpid());
    /* Read station configuration files.				*/
    collect_station_info(all_stations);
    for (s = st_head; s != NULL; s =s->next) {
	set_initial_states(s, DAEMON_RECONFIG);
    }
    reconfig_flag = 0;
}
#endif	/* ALLOW_RECONFIG */

/************************************************************************
 *  read_network_cfg:
 *	Read network config file.
 ************************************************************************/
void read_network_cfg (char *config_file, char *section)
{
    config_struc network_cfg;		/* structure for config file.	*/
    char str1[SECWIDTH], str2[SECWIDTH];
    struct stat sb;
    int fatal = 0;

    /* Scan network initialization file.				*/
    if (open_cfg(&network_cfg,config_file,section)) {
	fprintf (info, "Could not find [%s] section in network file %s\n", section, config_file);
	exit(1);
    }
    while (1) {
	read_cfg(&network_cfg,&str1[0],&str2[0]);
	if (str1[0] == '\0') break;
	/* Mandatory daemon configuration parameters.		*/
	else if (strcmp(str1,"LOGDIR")==0) {
	    strcpy(logdir,str2);
	}
	else if (strcmp(str1,"CMDDIR")==0) {
	    strcpy(cmddir,str2);
	}
	/* Optional daemon configuration parameters.		*/
	else if (strcmp(str1,"LOCKFILE")==0) {
	    strcpy(lockfile,str2);
	}
	else if (strcmp(str1,"POLL_INTERVAL")==0) {
	    poll_interval = atoi(str2);
	}
	/* Default Startup/Shutdown parameters.			*/
	/* Can be overridden on a per-station basis.		*/
	else if (strcmp(str1,"SERVER_STARTUP_DELAY")==0) {
	    net_server_startup_delay = atoi(str2);
	}
	else if (strcmp(str1,"CLIENT_STARTUP_DELAY")==0) {
	    net_client_startup_delay = atoi(str2);
	}
	else if (strcmp(str1,"MAX_SHUTDOWN_WAIT")==0) {
	    net_max_shutdown_wait = atoi(str2);
	}
	/* Default notification parameters.				*/
	/* Can be overridden on a per-station basis.		*/
	else if (strcmp(str1,"MIN_NOTIFY")==0) {
	    net_min_notify = atoi(str2);
	}
	else if (strcmp(str1,"RE_NOTIFY")==0) {
	    net_re_notify = atoi(str2);
	}
	else if (strcmp(str1,"RES_NOTIFY")==0) {
	    net_res_notify = atoi(str2);
	}
	else if (strcmp(str1,"NOTIFY_PROG")==0) {
	    strcpy(net_notify_prog,str2);
	}
    }
    close_cfg(&network_cfg);
    if (strlen(logdir) == 0) {
	fprintf (info, "Missing LOGDIR entry in %s\n", NETWORK_FILE);
	++fatal;
    }
    else {
	if (!(stat(logdir,&sb)==0 && S_ISDIR(sb.st_mode))) {
	    fprintf (info, "LOGDIR dir not found: %s\n", logdir);
	    ++fatal;
	}
    }
    if (strlen(cmddir) == 0) {
	fprintf (info, "Missing CMDDIR entry in %s\n", NETWORK_FILE);
	++fatal;
    }
    else {
	if (!(stat(cmddir,&sb)==0 && S_ISDIR(sb.st_mode))) {
	    fprintf (info , "CMDDIR dir not found: %s\n", cmddir);
	    ++fatal;
	}
    }
    if (fatal) exit(1);
}

/************************************************************************
 *  finish_handler:
 *	Trap signal and set terminate flag for orderly shutdown.
 ************************************************************************/
void finish_handler(int sig)
{
    terminate_proc = 1;
    signal (sig,finish_handler);    /* Re-install handler (for SVR4)	*/
}

/************************************************************************
 *  hup_handler:
 *	Trap signal and set reconfig flag for runtime reconfiguration.
 ************************************************************************/
void hup_handler(int sig)
{
#ifdef	ALLOW_RECONFIG
    reconfig_flag = 1;
#endif	/* ALLOW_RECONFIG */
    signal (sig,hup_handler);    /* Re-install handler (for SVR4)	*/
}

/************************************************************************
 *  strerror:
 *	Return error msg string for system error (provided in Solaris2.
 ************************************************************************/
#ifndef SOLARIS2
#ifndef LINUX
extern int sys_nerr;
extern char *sys_errlist[];

char *strerror (int errno) 
{
    char *p;

    if (errno < sys_nerr && errno >=0) {
	return (sys_errlist[errno]);
    }
    else {
	return ("Unknown error number");
    }
}
#endif	/* LINUX */
#endif	/* SOLARIS2 */

/************************************************************************
 *  collect_group_info:
 *	Collect infomation on all stations within the specified group.
 ************************************************************************/
int collect_group_info (char *group)
{
    STATION_INFO *s;
    char station[8];
    char str1[SECWIDTH], str2[SECWIDTH];
    config_struc master_cfg;		/* Master station file cfg.	*/
    int groupsize = 0;
    char *station_name = "*";

    /* Scan station list.						*/
    if (open_cfg(&master_cfg,STATIONS_FILE,station_name)) {
	fprintf (info, "Could not find station %s\n", station_name);
	exit(1);
    }

    /* Loop until section is not found in order to handle wildcards.	*/
    if debug(DEBUG_CONFIG) {
	    fprintf (info, "Looking in master config for group %s.\n", group);
	    fflush (info);
	}
    while (skipto(&master_cfg,"*")==0) {
	/* Pick up station name from the section heading.		*/
	strcpy (station,master_cfg.lastread+1);
	station[strlen(station)-1] = '\0';

	/* Read and verify basic station info from master config file.	*/
	while (1) {
	    read_cfg(&master_cfg,&str1[0],&str2[0]);
	    if (str1[0] == '\0') break;
	    if (strncasecmp(str1,"GROUP",5)==0) {
		if (strcasecmp (str2, group) == 0) {
		    if debug(DEBUG_CONFIG) {
			    fprintf (info, "Found [%s] in group %s.\n", station, group);
			    fflush (info);
			}
		    if ((s=collect_station_info (station)) == NULL) {
			fprintf (info, "Station %s not found\n", station);
			continue;
		    }
		    ++groupsize;
		}
	    }
	}
    }
    return (groupsize);
}
