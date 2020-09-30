/*
 * Program: q330serv
 *
 * File:
 *  q330serv
 *
 * Purpose:
 *  Connect to a Quanterra Q330, acquire and distribute MiniSEED data.
 *  Optionally multicast selected lower-latency uncompressed data channels.
 *
 * Author:
 *   Doug Neuhauser, adapted from qmaserv2 by Phil Maechling, Hal Schechner @ ISTI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 * Modification History:
 *  2020-05-10
 *  2020-09-29 DSN Updated for comserv3.
 */

#include <iostream>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

/*#include <stdio.h>*/

// our includes
#define	DEFINE_EXTERNAL
#include "global.h"
#include "Logger.h"
#include "Verbose.h"
#include "ConfigVO.h"
#include "q330serv.h"
#include "ReadConfig.h"
#include "lib330Interface.h"
#include "q330servcfg.h"
#include "portingtools.h"

// comserv includes
#include "cslimits.h"
#include "comserv_calls.h"
#include "srvc.h"

extern "C" {
    void cs_sig_alrm (int signo);
}

extern "C" {
    void define_comserv_vars();
}

//
// These are the instantiations of the variables declared in the global.h
//
Verbose          g_verbosity;
ConfigVO         g_cvo;
bool             g_reset;
bool             g_done;
Verbose*         g_verbList; // Not needed. Just for testing

Logger		 g_log;
Lib330Interface  *g_libInterface = NULL;
int		 log_inited;

#ifdef ENDIAN_LITTLE
#define MY_ENDIAN_STRING	"LITTLE_ENDIAN"
#else
#define MY_ENDIAN_STRING	"BIG_ENDIAN"
#endif
char *my_endian_string = (char *)MY_ENDIAN_STRING;

// *****************************************************************************
// showVersion:
//	Output version string for the program.
// *****************************************************************************

void showVersion() {
    g_log <<     APP_VERSION_STRING << " - " << my_endian_string << std::endl;
}

// *****************************************************************************
// print_syntax:
//	Output syntax info for the program.
// *****************************************************************************

void print_syntax(char *cmdname)
{
    showVersion();
    g_log <<     APP_IDENT_STRING << " [-h] server_name" << std::endl;
}

// *****************************************************************************
// main program.
// *****************************************************************************

int main(int argc, char *argv[]) {
    char *cmdname;				// program name.
    char server_name[MAX_CHARS_IN_SERVER_NAME];
    time_t nextStatusUpdate;
    char log_basename[2048];
    int log_mode;
    csconfig cs_cfg;
    int lockfd;
    int status;
    bool ok;

    // Variables needed for getopt.
    extern int	optind;
    int		c;

    setlinebuf(stdout);
    setlinebuf(stderr);
    define_comserv_vars();

    cmdname = basename(strdup(argv[0]));
    while ( (c = getopt(argc,argv,"hv:c:n:")) != -1)
	switch (c) {
	case '?':
        case 'h':   print_syntax(cmdname); exit(0);
	}
    
    // Skip over all options and their arguments.
    argv = &(argv[optind]);
    argc -= optind;

    if (argc > 0) {
	strlcpy (server_name, argv[0], sizeof(server_name));
    }
    else {
	g_log << "Error: missing server_name" << std::endl;
	print_syntax (cmdname);
	exit (1);
    }

    // Get this server_name's and program_name's config parameters.
    ok = readConfigFile(server_name);
    if (! ok) {
	g_log << "Error: server exiting due to configuration errors" << std::endl;
	exit (12);
    }

    // Get this server_name's comserv-subsystem config parameters.
    initialize_csconfig(&cs_cfg);
    status = getAllCSServerParams (&cs_cfg, server_name);
    if (status != 0) {
	g_log << "Error: server exiting due to comerv subsystem configuration errors" << std::endl;
	exit (12);
    }

#ifdef COMSERV2
    // Copy paramters that used to be in COMSERV2 section to new server config.
    if (strlen(cs_cfg.logdir) > 0)   g_cvo.setLogDir(cs_cfg.logdir);
    if (strlen(cs_cfg.logtype) > 0)  g_cvo.setLogType(cs_cfg.logtype);
    if (strlen(cs_cfg.lockfile) > 0) g_cvo.setLockFile(cs_cfg.lockfile);
    if (strlen(cs_cfg.udpaddr) > 0)  g_cvo.setQ330UdpAddr(cs_cfg.udpaddr);
#endif

    /* Open the lockfile for exclusive use if lockfile is specified.*/
    /* This prevents more than one copy of the program running for */
    /* a single station.      */
    char *lockfile = g_cvo.getLockFile();
    if (strlen(lockfile) > 0)
    {
        if ((lockfd = open (lockfile, O_RDWR | O_CREAT, 0644)) < 0)
        {
	    g_log << "Error: Unable to open lockfile: " << lockfile << std::endl;
            exit (12) ;
        }
        if ((status = lockf (lockfd, F_TLOCK, 0)) < 0)
        {
	    g_log << "Error:  Unable to lock lockfile: " << lockfile << std::endl;
            close (lockfd);
            exit (12) ;
        }
    }

    // Initialize datetime tagged logging used by Logger class and LogMessage();
    char *LogType = g_cvo.getLogType();
    char *LogDir = g_cvo.getLogDir();
	
    if ( (strcasecmp(LogType, "LOGFILE") == 0 && 
	  strlen(LogDir) > 0 && strcmp(LogDir,".") != 0) )
    {
	log_mode = CS_LOG_MODE_TO_LOGFILE;
	g_log << "Logging to file: LOGTYPE=" << LogType
	      << " LOGDIR=" << LogDir << std::endl;
    } 
    else 
    {
	log_mode = CS_LOG_MODE_TO_STDOUT;
	g_log << "Logging to stdout: LOGTYPE=" << LogType
	      << " LOGDIR=" << LogDir << std::endl;
    } 
    sprintf(log_basename, "%s.%s", server_name,cmdname);
    if (LogInit(log_mode, LogDir, log_basename, 2048) != 0)
    {
	g_log << "Error: LogInit() problems - exiting" << std::endl;
	exit(12) ;
    }

    // Initialize Logger class and set global flag for logging initialization.
    g_log.logToStdout ((log_mode == CS_LOG_MODE_TO_STDOUT));
    g_log.logToFile ((log_mode == CS_LOG_MODE_TO_LOGFILE));
    log_inited = 1;

    // Announce ourselves now that logging has been initialized.
    showVersion();

    // Limit the max number of open file descriptors to the compliation value 
    // FD_SETSIZE, since this is used to create the fd_set options used by select().
    struct rlimit rlp;
    status = getrlimit (RLIMIT_NOFILE, &rlp);
    if (status != 0) {
	g_log << "XXX Program unable to query max_open_file_limit RLIMIT_NOFILE" << std::endl;
	return (1);
    }
    if (rlp.rlim_cur > FD_SETSIZE) {
	rlp.rlim_cur = FD_SETSIZE;
	status = setrlimit (RLIMIT_NOFILE, &rlp);
	if (status != 0) {
	    g_log << "XXX Program unable to set max_open_file_limit RLIMIT_NOFILE" << std::endl;
	    return (1);
	}
    }
    g_log << "XXX Program max_open_file_limit = " << (uint64_t)rlp.rlim_cur << std::endl;

    initializeSignalHandlers();
    g_done  = false;
    g_reset = false;
    g_verbosity.setVerbosity(0);
    g_verbosity.setDiagnostic(0);
    initialize_comserv(&cs_cfg, server_name);

    nextStatusUpdate = time(NULL) + g_cvo.getStatusInterval();

    // Main program loop.  Loop forever until we are told to terminate.

    int first_pass = 1;
    while(!g_done) {
	g_reset = 0;

	// Make sure we have a Lib interface, create one if not.
	if(!g_libInterface) {
	    g_libInterface = new Lib330Interface(server_name, g_cvo);
	    g_log << "+++ Server thread address is " << static_cast<void *>(g_libInterface) << std::endl;
	}

	// We may have gotten here if there is no place to put MiniSEED data.
	if(!g_libInterface->processPacketQueue()) {
	    g_log << "XXX Comserv queue full or bad packet...  Sleeping for 5 seconds" << std::endl;
	    struct timespec t;
	    t.tv_sec = 0;
	    t.tv_nsec = 250000000;
	    for(int i=0; i <= 20; i++) {
		nanosleep(&t, NULL);
		scan_comserv_clients();
	    }
	    continue;
	}

	// Register and wait for data to flow.

	// 1.  Wait until we get to state RUNWAIT. /
	g_libInterface->startRegistration();
	if(!g_libInterface->waitForState(LIBSTATE_RUNWAIT, 120, scan_comserv_clients)) {
	    delete g_libInterface;
	    g_libInterface = NULL;
	    continue;
	} 

	// 2. Optionally wait a bit more time to allow clients to be 
	// externally started before we start data flowing.
	// Keep scanning comserv clients while we wait.
	int wait_for_clients = g_cvo.getWaitForClients();
	if (first_pass && wait_for_clients) {
	    g_log << "+++ Wait " << wait_for_clients << " seconds for clients to start. " << std::endl;
	    if(!g_libInterface->waitForState(LIBSTATE_RUN, wait_for_clients, scan_comserv_clients)) {
	    }
	}
	first_pass = 0;
    
	// If we are still in RUNWAIT state, start data flowing by moving to RUN state.
	// Otherwise, reset and try again.
	if (g_libInterface->getLibState() == LIBSTATE_RUNWAIT) {
	    g_libInterface->startDataFlow();      
	}
	else {
	    delete g_libInterface;
	    g_libInterface = NULL;
	    continue;
	} 

	// This is where we live for most of the life of the process.  
	// Scan the clients and do what we're told.
	while(!g_reset) {
	    int packetQueueEmptied;
	    scan_comserv_clients();
	    if(time(NULL) >= nextStatusUpdate) {
		g_libInterface->displayStatusUpdate();
		nextStatusUpdate = time(NULL) + g_cvo.getStatusInterval();
	    }
	    packetQueueEmptied = g_libInterface->processPacketQueue();
	    if(!packetQueueEmptied && g_libInterface->queueNearFull()) {
		g_log << "XXX Intermediate packet queue too full - need to halt dataflow." << std::endl;
		g_reset = 1;
	    }
	    // If Lib has for any reason transitioned to a RUNWAIT state,
	    // move to a RUN state.
	    if (g_libInterface->getLibState() == LIBSTATE_RUNWAIT) {
		g_libInterface->startDataFlow();      
	    }
	}

	// We get here when g_reset is set to 1
	g_libInterface->startDeregistration();
    
    }
    cleanup(0);
    g_log << "Main thread ending.  Should NEVER GET HERE." << std::endl;
    return 0;
}

//
// Initialize the signal handlers and the variables
//
void initializeSignalHandlers() {

    //
    // Client will send a SIGALRM signal when it puts it's segment ID into the
    // service queue. Make sure we don't die on it, just exit sleep.
    //

    struct sigaction action;
    /* Set up a permanently installed signal handler for SIG_ALRM. */
    action.sa_handler = cs_sig_alrm;
    action.sa_flags = 0;
    sigemptyset (&(action.sa_mask));
    sigaction (SIGALRM, &action, NULL);

    signal (SIGPIPE, SIG_IGN) ;

    //
    // Intercept various abort type signals so they send a disconnect message
    // to the server library if we control C out of this program.
    // Netmon may also signal this program to cause it to exit.
    //

    signal (SIGHUP ,cleanupAndExit) ;
    signal (SIGINT ,cleanupAndExit) ;
    signal (SIGQUIT,cleanupAndExit) ;
    signal (SIGTERM,cleanupAndExit) ;

}


void readOrRereadConfigFile(char* serverName) {
    bool res = readConfigFile(serverName);

    if(res) {
	g_verbosity.setVerbosity(g_cvo.getVerbosity());
    }
    else {
	g_log << "xxx Error reading " << APP_IDENT_STRING << " configuration values for server: " 
	      << serverName << std::endl;
	cleanup(0);
    }
}

void scan_comserv_clients() {
    static int areSuspended = 0;
    static int flushData = 0;

    int clientCmd = comserv_scan();
    if(clientCmd != 0) {
	//g_log << "+++ Found signal from client: " << clientCmd << std::endl;
	switch(clientCmd) {
	case CSCM_SUSPEND:
	    //cleanup(1);
	    g_log << "--- Suspending link (Requested)" << std::endl;
	    g_libInterface->startDeregistration();
	    areSuspended = 1;
	    flushData = 1;
	    break;
	case CSCM_TERMINATE:
	    g_log << "--- Terminating server (Requested)" << std::endl;
	    cleanupAndExit(1);
	    break;
	case CSCM_RESUME:
	    if(areSuspended) {
		g_log << "--- Resuming link (Requested)" << std::endl;
		g_reset = 1;
		areSuspended = 0;
	    }
	    else {
		g_log << "--- Asked to resume unsuspended link.  Ignoring." << std::endl;
	    }
	}
    }

    // If we are shutting down, flush data queues if we are not in RUN state.
    if (areSuspended && flushData && (g_libInterface->getLibState() != LIBSTATE_RUN)) {
	g_libInterface->flushData();
	flushData = 0;
    }
}

void initialize_comserv(csconfig *pcs_cfg, char *serverName) {
    g_log << "+++ Initializing comserv subsystem (Server: " << serverName << ")." << std::endl;
    comserv_init(pcs_cfg,serverName);
    g_log << "+++ Comserv subsystem initialized." << std::endl;
}

void cleanup(int arg) {
    // perform any cleanup that's required
    if(g_libInterface) {
	delete g_libInterface;
    }
}

void cleanupAndExit(int arg) {
    g_log << "+++ Shutdown started" << std::endl;
    cleanup(arg);
    g_log << "+++ Application ended" << std::endl;
    exit(0);
}
