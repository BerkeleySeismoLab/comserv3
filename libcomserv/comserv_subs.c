/*
 *
 * File     :
 *   server_subs.c
 *
 * Purpose  :
 *   This is a port of the comserv main program (server.c) to a subroutine
 *   function. The subroutine version should be callable from a C++ program.
 *
 * Author   :
 *  Phil Maechling
 *
 *
 * Mod Date :
 *  27 March 2002
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
 */
/*$Id: cserv.c,v 1.3 2005/06/17 19:29:53 isti Exp $*/
/*   Server Main file
     Copyright 1994-1998 Quanterra, Inc.
     Written by Woodrow H. Owens
     Linux porting by ISTI  (www.isti.com)
     with modifications by KIM GEUNYOUNG fo Linux UDP 	

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 11 Mar 94 WHO First created.
    1 23 Mar 94 WHO First testing done, start splitting code of into modules.
    2 27 Mar 94 WHO Merged blockette ring changed to separate rings.
    3  7 Apr 94 WHO Individual fields for link statistics removed in favor
                    linkstat_rec. 
    4 16 Apr 94 WHO Define seed format and extension level. Show station
                    description.
    5 30 May 94 WHO Add global comm_mask variable.
    6  6 Jun 94 WHO client_wait field changed to microseconds. add setting
                    of privusec and nonusec fields. Don't send SIGALRM if
                    not same UID as client. Add user privilege variables
                    (handled by cscfg and commands).
    7  9 Jun 94 WHO Add support to remove foreign clients after timeout.
                    Cleanup to avoid warnings.
    8 16 Jun 94 WHO Fix calculation of foreign_to in procedure check_clients.
    9  9 Aug 94 WHO Fix initialization of linkstat record.
   10 11 Aug 94 WHO Add network support.
   11 18 Aug 94 WHO Fix = instead of == in detach_client.
   12 25 Sep 94 WHO Add REUSEADDR and KEEPALIVE options to socket.
   13  3 Nov 94 WHO Add SOLARIS2 definition to use nanosleep call instead of usleep.
                    SOLARIS2 also makes slight syntax differences in socket calls.
   14 13 Dec 94 WHO Add size of data only blockette to ring buffers allocations.
                    Change SEED version extension from A to B.
   15 14 Jun 95 WHO Add ignoring the SIGPIPE signal so program doesn't terminate when
                    the socket is disconnected.
   16 20 Jun 95 WHO Process netto and netdly parameters. Allows giving up on network
                    connection and trying again. Check for timeout of ack grouping.
   17  2 Oct 95 WHO Don't do automatic 60 second link_req polling if sync set.
   18  8 Oct 95 WHO Allow enabling RTS/CTS flow control.
   19  2 Jun 96 WHO Start of conversion to run on OS9. Don't compare
                    the result of "kill" with zero, use ERROR.
   20 10 Jun 96 WHO Simplify file upload, just send packet every second.
   21 19 Jun 96 WHO Allocate cal ring buffer room based on random_calibration
                    instead of sine_calibration, which is 4 bytes shorter.
   22 30 Jun 96 WHO Don't set active flag to FALSE in detach_client if this
                    is a reserved client that hasn't timed out yet, or else
                    the timeout will never occur.
   23  4 Aug 96 WHO If noultra flag is on, don't poll for ultra packet.
   24  3 Dec 96 WHO Add support for Blockette Records.
   25  7 Dec 96 WHO Add support for UDP.
   26 16 Oct 97 WHO Change in cscfg. Add reporting of all module versions.
   27 22 Oct 97 WHO Add vopttable access for OS9 version.
   28 29 Nov 97 DSN Added optional lockfile directive again.
   29  8 Jan 98 WHO lockfile stuff doesn't apply to OS9 version.
   30 23 Jan 98 WHO Add link_retry variable to use with linkpoll.
   31  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
   32 10 Nov 99 IGD A third parameter in semop set to 1 instead of 0
   33  6 Dec 00 IGD The above change is #ifdef'ed for Linux only for full compatibility
			with the Solaris version with the version 30	
   34  8 Sep 00 KIM Change timespec definition struct timespec var
   35  8 Sep 00 KIM Change ipaddr, ipaddr type conversion
   36  7 Dec 00 IGD Changes are incorporated into the root Linux version
   37 18 Apr 07 DSN Test for failure when shmat() to client shared memory.
   38 24 Aug 07 DSN Separate ENDIAN_LITTLE from LINUX logic.
   39 12 Jul 2020 DSN	Removed code needed only by original comserv program.
   			Removed _OSK conditional code.
   40 29 Sep 2020 DSN Updated for comserv3.
*/           

#define EDITION 39

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/file.h>

#include "quanstrc.h"
#include "stuff.h"
#include "service.h"
#include "cfgutil.h"
#include "server.h"
#include "timeutil.h"
#include "logging.h"
#include "comserv_vars.h"
#include "csconfig.h"

/* Comserv module version numbers */
extern short VER_TIMEUTIL ;
extern short VER_CFGUTIL ;
extern short VER_SEEDUTIL ;
extern short VER_STUFF ;
extern short VER_COMLINK ;
extern short VER_CSCFG ;
extern short VER_BUFFERS ;
extern short VER_COMMANDS ;

/* From seedutil.c */
extern char log_channel_id[4] ;
extern char log_location_id[3] ;
extern char clock_channel_id[4] ;
extern char clock_location_id[3] ;

/* External variables used only in this file */
int retVal = 0;
int segkey = 0 ;
short clientpoll = 0 ;
double lastsec ;
pclient_struc cursvc ;
pclient_station curclient = NULL;

int32_t tscan;
short did;
int32_t services;
int32_t ctcount;
float cttotal;
short uppoll;
#ifdef SOLARIS2
timespec_t rqtp, rmtp ;
#endif
#ifdef LINUX /*ADDED BY KIM */
struct timespec rqtp, rmtp ;
#endif

/* External functions used in this file. */
void unblock (short clientnum) ;
byte client_handler (pclient_struc svc, short clientnum) ;
void setupbuffers (void) ;
 
/***********************************************************************
 *  terminate
 *	Terminate the program after printing provided message.
 ***********************************************************************/

void terminate (pchar s)
{
    LogMessage(CS_LOG_TYPE_ERROR, "%s Exit: %s", localtime_string(dtime()), s) ;
    exit(12) ;
}

/***********************************************************************
 *  set_verb
 *	Set debugging variables based on specified verbose value.
 ***********************************************************************/

void set_verb (int i)
{
    verbose = i > 0 ;
    rambling = i > 1 ;
    insane = i > 2 ;
}

/***********************************************************************
 *  detach_client
 *	Detach a client.
 ***********************************************************************/

void detach_client (short clientnum, boolean timedout)
{
    short i ;
    tclients *pt ;
    boolean sameuid ;

    if (combusy == clientnum)
        combusy = NOCLIENT ;
    pt = &clients[clientnum] ;
    pt->client_pid = NOCLIENT ;
    pt->client_uid = NOCLIENT ;
    if (pt->client_address)
    {
	sameuid = clients[clientnum].client_address->client_uid == base->server_uid ;
	if (shmdt((pchar) pt->client_address) == 0)
	    if (sameuid) /* Don't delete foreign client, we don't know for sure it is dead */
		shmctl(pt->client_memid, IPC_RMID, NULL) ;
	pt->client_address = NULL ;
    }
    if (timedout)
    {
	pt->active = FALSE ;
	unblock (clientnum) ;
	clr_bit (&blockmask, clientnum) ;
    }
    if (clientnum >= resclient)
    { /* remove this client from list */
	pt->active = FALSE ;
	for (i = clientnum ; i < highclient - 1 ; i++)
            clients[i] = clients[i + 1] ;
	highclient-- ; /* contract the window */
    }
}
         
/***********************************************************************
 *  check_clients
 *	Check the state of registerd clients.
 *	Detach client if it appears dead.
 ***********************************************************************/

void check_clients (void)
{
    tclients *pt ;
    boolean timedout, died, foreign_to, sameuid, alive ;
    char msgbuf[256];

    if (++clientpoll >= highclient)
        clientpoll = 0 ;
    pt = &clients[clientpoll] ;
    if (! pt->active)
        return ;
    timedout = (pt->blocking) && ((curtime - pt->last_service) > pt->timeout) ;
    alive = (pt->client_address) && (pt->client_pid != NOCLIENT) ;
    sameuid = alive && (pt->client_uid == base->server_uid) ;
    died =  alive && sameuid && (kill(pt->client_pid, 0) == ERROR) ;
    foreign_to = alive && (! sameuid) && ((curtime - pt->last_service) > NON_PRIVILEGED_TO)
	&& (combusy != clientpoll) ;
    msgbuf[0] = '\0';
    if (timedout && verbose)
        sprintf(msgbuf,"Client %s timed out at %s", (char *)&pt->client_name, time_string(dtime())) ;
    else if (died && verbose)
        sprintf(msgbuf,"Client %s has died at %s", (char *)&pt->client_name, time_string(dtime())) ;
    else if (foreign_to && verbose)
        sprintf(msgbuf,"Foreign Client %s presumed dead at %s", (char *)&pt->client_name, time_string(dtime())) ;
    if (died || timedout || foreign_to)
    {
	if (verbose)
	{
	    if (timedout)
		strcat(msgbuf,", unblocking") ;
	    if (clientpoll >= resclient)
		strcat(msgbuf,", Removed from client table\n") ;
	    else
		strcat(msgbuf,", Reserved client, not removed\n") ;
	}
	detach_client (clientpoll, timedout) ;
    }
    if (msgbuf[0]) LogMessage(CS_LOG_TYPE_INFO, msgbuf);
}

/***********************************************************************
 *  comserv_init
 *	Initialize all elements of the comserv subsystem.
 ***********************************************************************/

int comserv_init (csconfig *cs_cfg, char* server_name)
{
    int shmid ;
    int semid;
    short i, j ;
    struct sembuf notbusy = { 0, 1, 0 } ;
    int32_t bufsize, size ;
       
    string15 station_name ;
      
    start_time = (int32_t) dtime () ;

    /* Initialize the comserv station name from the provided server_name, */
    strncpy(station_name, server_name,SERVER_NAME_SIZE) ;
    station_name[SERVER_NAME_SIZE-1] = '\0';
    copy_sname_cs_str(station,"");

    /* Report versions of all comserv subsystem modules */
    LogMessage (CS_LOG_TYPE_INFO, "Configuring comserv subsystem for server_name : %s\n",server_name);
    LogMessage (CS_LOG_TYPE_INFO, "%s comserv_init Edition %d started for station %s\n", 
		localtime_string(dtime()), EDITION, &station_name) ;
    LogMessage (CS_LOG_TYPE_INFO, "      Quanstrc Ver=%d, DPstruc Ver=%d, Seedstrc Ver=%d, Timeutil Ver=%d\n",
		VER_QUANSTRC, VER_DPSTRUC, VER_SEEDSTRC, VER_TIMEUTIL) ;
    LogMessage (CS_LOG_TYPE_INFO, "      Cfgutil Ver=%d, Seedutil Ver=%d, Stuff Ver=%d, Comlink Ver=%d\n",
		VER_CFGUTIL, VER_SEEDUTIL, VER_STUFF, VER_COMLINK) ;
    LogMessage (CS_LOG_TYPE_INFO, "      Cscfg Ver=%d, Buffers Ver=%d, Commands Ver=%d\n",
		VER_CSCFG, VER_BUFFERS, VER_COMMANDS) ;

    /* The stations.ini and network.ini file have already been read. */
    /* Set the required comserv global variables from the provide cs_cfg structure */
    strcpy (station_desc, cs_cfg->server_desc);
    if (strlen(cs_cfg->log_channel_id) > 0) {
	strcpy (log_channel_id, cs_cfg->log_channel_id);
	strcpy (log_location_id, cs_cfg->log_location_id);
    }
    if (strlen(cs_cfg->timing_channel_id) > 0) {
	strcpy (clock_channel_id, cs_cfg->timing_channel_id);
	strcpy (clock_location_id, cs_cfg->timing_location_id);
    }
    set_verb(cs_cfg->verbose);
    segkey = cs_cfg->segid;
    if (cs_cfg->pollusecs > 0) 
	polltime = cs_cfg->pollusecs;
    override = cs_cfg->override;

    /* Set user privilege structure info from config info. */
    /* Note that the cs_cfg client structure MAY NOT be the same as the server client structure! */
    for (i = 0 ; i < cs_cfg->n_uids ; i++ )
    {
	user_privilege[i].user_id = cs_cfg->user_privilege[i].user_id;
	user_privilege[i].user_mask = cs_cfg->user_privilege[i].user_mask;
    }

    /* Set client control structure info from config info. */
    /* Note that the cs_cfg client structure MAY NOT the same as the server client structure! */
    for (i = 0 ; i < MAXCLIENTS ; i++ )
    {
	clients[i].client_memid = NOCLIENT ;
	clients[i].client_pid = NOCLIENT ;
	clients[i].client_uid = NOCLIENT ;
	clear_cname_cs(clients[i].client_name);
        clients[i].client_address = NULL ;
	clients[i].blocking = FALSE ;
	clients[i].timeout = 0 ; /* blocking not allowed */
	clients[i].active = FALSE ;
	clients[i].last_service = 0.;
	/* Override any client defaults with config info */
	if (i < cs_cfg->n_clients) {
	    copy_cname_cs_str(clients[i].client_name,cs_cfg->clients[i].client_name);
	    clients[i].timeout = cs_cfg->clients[i].timeout;
	    if (clients[i].timeout)
	    {
		set_bit (&blockmask, i);
		clients[i].active = TRUE;
		clients[i].blocking = TRUE;
		clients[i].last_service = dtime ();
	    }
	}
	for (j = DATAQ ; j < NUMQ ; j++)
	{
	    clients[i].last[j].scan = NULL ;
	    clients[i].last[j].packet = -1 ;
	}
    }
    highclient = cs_cfg->n_clients;

    /* Initialize ring structure */
    for (i = DATAQ ; i < NUMQ ; i++)
    {
	int count;
	if (i == DATAQ) count = cs_cfg->databufs;
	else if (i == DETQ) count = cs_cfg->detbufs;
	else if (i == CALQ) count = cs_cfg->calbufs;
	else if (i == TIMQ) count = cs_cfg->timbufs;
	else if (i == MSGQ) count = cs_cfg->msgbufs;
	else if (i == BLKQ) count = cs_cfg->blkbufs;
	else terminate ("Unknown ring number\n");
	rings[i].head = NULL ;
	rings[i].tail = NULL ;
	rings[i].count =  count ;
	rings[i].spare = 0 ;
	rings[i].size = 0 ;
    }

    resclient = highclient ; /* reserved clients */

    /* Create access control semaphore */
    semid = semget(segkey, 1, IPC_CREAT | PERM) ;
    if (semid == ERROR)
    {
	LogMessage (CS_LOG_TYPE_ERROR, "Exit: Could not create semaphore with key %d\n", segkey) ;
	exit (12) ;
    }

    /* Set the size of each ring buffer and add them up to get total size */
    bufsize = 0 ;
    for (i = DATAQ ; i < NUMQ ; i++)
    {
	/* For servers that store ONLY MiniSEED records, all ring elements are all the same size. */
	size = sizeof(tdata_user);

	rings[i].xfersize = size ; /* number of bytes to transfer to client */
	size = (size + 31) & 0xfffffff8 ; /* add in overhead and double word align */
	bufsize = bufsize + size * rings[i].count ;
	rings[i].size = size ;
    }
    bufsize = (bufsize + 15) & 0xfffffff8 ; /* double word align */

    /* Create shared memory segment and install my process id */
    shmid = shmget(segkey, sizeof(tserver_struc) + 
		   bufsize, IPC_CREAT | PERM) ;
    if (shmid == ERROR)
    {
	LogMessage (CS_LOG_TYPE_ERROR, "Exit: Could not create server segment with key %d\n", segkey) ;
	exit (12) ;
    }
    base = (pserver_struc) shmat(shmid, NULL, 0) ;
    if (base == (pserver_struc) ERROR)
    {
	LogMessage (CS_LOG_TYPE_ERROR, "Exit: Could not attach to server segment ID %d\n", shmid) ;
	exit (12) ;
    }
    base->init = '\0' ;
    base->server_pid = getpid () ;
    base->server_uid = getuid () ;
    base->client_wait = MAXWAIT * 1000000 ; /* convert to microseconds */
    base->privusec = PRIVILEGED_WAIT ;
    base->nonusec = NON_PRIVILEGED_WAIT ;
    base->server_semid = semid ;
    base->servcode = dtime () ;

    /* Initialize service queue */
    for (i = 0 ; i < MAXCLIENTS ; i++)
    {
	base->svcreqs[i].clientseg = NOCLIENT ;
	copy_cname_cs_cs(base->svcreqs[i].clientname,clients[i].client_name) ;
    }

    /* Initialize next packet numbers */
    base->next_data = 0 ;

    /* Call routine to setup ring buffers for data and blockettes */
    setupbuffers () ;
   
    /* Allow access to service queue */ 
    if (semop(semid, &notbusy, 1) == ERROR) 
    {
	LogMessage (CS_LOG_TYPE_ERROR, "Exit: Could not set semaphore ID %d to not busy\n", semid) ;
	exit (12) ;
    }
          
    /* Server shared memory segment is now initialized */
    base->init = 'I' ;

    stop = 0 ;
    uppoll = 0 ;
    tscan = 0 ;
    services = 0 ;
    ctcount = 0 ;
    cttotal = 0 ;
    dest = (pchar) &dbuf;
 
    rqtp.tv_sec = (1000 * polltime) / 1000000000 ;
    rqtp.tv_nsec = (1000 * polltime) % 1000000000 ;

    lastsec = dtime () ;

    return 1;
}

/***********************************************************************
 *  comserv_scan
 *    	Routine to check for service requests from clients.
 *	The main server program should poll this routine on a regular
 *	basis to ensure client data requests are met in a timely manner.
 *	Routine sleeps for polltime usecs before returning.
 *    Returns:  retVal (set by client_handler), or 0 if no requests.
 ***********************************************************************/

int comserv_scan()
{
    char *client_tag;
    short cur;
    int clientid;
    short i,found;
    int32_t ct;

    retVal = 0;
    // LogMessage (CS_LOG_TYPE_DEBUG, comserv_scan checking clients.\n");
    tscan++ ;
    did = 0 ;
/* Look for service requests */
    for (cur = 0 ; cur < MAXCLIENTS ; cur++)
    {
	clientid = base->svcreqs[cur].clientseg ;
	if (clientid != NOCLIENT)
	{
/* Search in table to find previously registered client */
	    cursvc = NULL ;
	    for (i = 0 ; i < highclient ; i++)
		if (clients[i].client_memid == clientid)
		{
		    cursvc = clients[i].client_address ;
		    if (cursvc->client_pid != clients[i].client_pid)
			cursvc = NULL ; /* not a complete match */
		    else
			break ; /* found a complete match of segment ID and PID */
		}
	    if (cursvc == NULL)
	    {
		cursvc = (pclient_struc) shmat(clientid, NULL, 0) ;
/*:: DSN start add */
		/* If we cannot attach to client's shared memory, skip client. */
		if (cursvc == (pclient_struc) ERROR)
		{
		    clients[i].client_address = NULL;
		    continue;
		}
/*:: DSN end add */
		found = FALSE ;
		for (i = 0 ; i < highclient ; i++)
		    if (client_match(clients[i].client_name,cursvc->myname) &&
			((clients[i].client_pid == cursvc->client_pid) ||
			 (clients[i].client_pid == NOCLIENT) || (i < resclient)))
		    {
			found = TRUE ; /* Name and PID match, new segment */
			clients[i].client_memid = clientid ;
			clients[i].client_pid = cursvc->client_pid ;
			clients[i].client_uid = cursvc->client_uid ;
			clients[i].client_address = cursvc ;
			if (verbose)
			{
			    if (i < resclient)
				if (clients[i].timeout)
				    client_tag = "Blocking";
				else
				    client_tag ="Reserved" ;
			    else
				client_tag = "Returning" ;
			    LogMessage (CS_LOG_TYPE_DEBUG, "%s client %s, with PID of %d with memory ID %d\n",
					client_tag,
					clients[i].client_name,
					cursvc->client_pid, clientid) ;
			}
			if (clients[i].timeout) /* client can be blocking */
			{
			    curclient = (pclient_station) ((uintptr_t) cursvc +
							   cursvc->offsets[cursvc->curstation]) ;
			    if (curclient->blocking)
			    { /* client is taking blocking option */
				clients[i].blocking = TRUE ;
				set_bit (&blockmask, i) ;
			    }
			    else
			    { /* client elected not to block */
				clients[i].blocking = FALSE ;
				unblock(i) ;
				clr_bit (&blockmask, i) ;
			    }
			}
			break ;
		    }
		if (! found)
		{
		    i = highclient++ ;
		    if (i >= MAXCLIENTS)
		    {
			cursvc->error = CSCR_REFUSE ;
			highclient-- ;
		    }
		    else
		    {
			clients[i].client_memid = clientid ;
			clients[i].client_pid = cursvc->client_pid ;
			clients[i].client_uid = cursvc->client_uid ;
			clients[i].client_address = cursvc ;
			copy_cname_cs_cs(clients[i].client_name,cursvc->myname) ;
			if (verbose)
			{
			    if (cursvc->client_uid == base->server_uid)
				LogMessage (CS_LOG_TYPE_DEBUG, "New client %s, with PID %d and memory ID %d\n", 
					    clients[i].client_name,
					    cursvc->client_pid, clientid) ;
			    else
				LogMessage (CS_LOG_TYPE_DEBUG, "New Foreign client %s, with PID %d, UID %d, and memory ID %d\n", 
					    clients[i].client_name, cursvc->client_pid,
					    cursvc->client_uid, clientid) ;
			}
		    }
		}
	    }
	    if (cursvc->error == OK)
	    {
		did++ ;
		services++ ;
		if ((services % 1000) == 0)
		{
		    ct = cttotal / ctcount ;
		    if (rambling)
		    {
			LogMessage(CS_LOG_TYPE_DEBUG,"%d CLient services, %d read calls, %d avg. time\n",
				   services, ctcount, ct) ;
		    }
		}
		clients[i].last_service = dtime () ;
		clients[i].active = TRUE ;
		cursvc->error = client_handler(cursvc, i) ;
	    }
	    cursvc->done = TRUE ;
	    base->svcreqs[cur].clientseg = NOCLIENT ;
	    if (cursvc->client_uid == base->server_uid)
		kill (cursvc->client_pid, SIGALRM) ;
	    if (did >= MAXPROC)
		break ;
	}
    }
    curtime = dtime () ;
    if ((curtime - lastsec) >= 1.0)
    {
	uppoll++ ;
	lastsec = curtime ;
	check_clients () ;
    }
    nanosleep (&rqtp, &rmtp) ;
    return retVal;
}
