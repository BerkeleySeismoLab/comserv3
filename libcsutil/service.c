/*$Id: service.c,v 1.2 2005/03/30 22:27:56 isti Exp $*/
/*   Client Server Access Library Routines
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 14 Mar 94 WHO Derived from client.c test program.
    1 31 Mar 94 WHO Client data & blockettes merged.
    2  7 Apr 94 WHO Interface to servers tightened up.
    3  8 Apr 94 WHO Initialize command output buffers.
    4  9 Apr 94 WHO cs_all renamed to cs_setup and station name added.
    5 18 Apr 94 WHO "stuff.h" added since dtime was moved there from service.h
    6 30 May 94 WHO In cs_svc, set curclient->status to CSCR_DIED if that is
                    the status to be returned (DSN).
    7  9 Jun 94 WHO Cleanup to avoid warnings.
    8  9 Aug 94 WHO Put protection against improbable duplicate detach (DSN).
    9 14 oct 94 jms use "nanosleep" if NANOSLEEP
   10  3 Nov 94 WHO Use SOLARIS2 definition to use nanosleep/sleep instead of usleep.
   11 27 Feb 95 WHO Start of conversion to run on OS9.
   12 29 May 96 WHO Somebody slipped in a change from "getuid" to
                    "geteuid" without updating change list.
   13  7 Jun 96 WHO Don't assume a good return from "kill" is zero,
                    check against -1.
   14  4 Dec 96 WHO Fix sels[CHAN] not being initialized.
   15 11 Jun 97 WHO Fix Solaris2/OSK conditionals for sleeping.e
   16 18 Feb 07 DSN Fix client detach from server shared memory.
   17 24 Aug 07 DSN Change from SIG_IGN to signal handler for SIGALRM.
   18 13 Sep 07 PAF moved DSN's signal handler to beginning of file
   19 22 Jan 2020 DSN	Parameterized CS_CHECK_INTERVAL in service.h.  
   			Originally it was the constant 10.
   20 29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>

#include "dpstruc.h"
#include "service.h"
#include "cfgutil.h"
#include "stuff.h"

#ifdef SOLARIS2
#include <time.h>
#endif

void cs_sig_alrm (int signo)
{
    /* nothing to do, just returning wakes up nanosleep. */
}

void cs_detach (pclient_struc client, short station_number);

/* Try to put the structure pointed to by "client" into service queue for
   server pointed to by "srvr". Returns 0 if no error, -1 if cannot
*/
short cs_svc (pclient_struc client, short station_number)
{
    short found, i ;
    int32_t sofar, sleeptime ;
    struct sembuf busy = { 0, -1, 0 } ;
    struct sembuf notbusy = { 0, 1, 0 } ;
    pclient_station curclient ;
    pserver_struc srvr ;
#ifdef SOLARIS2
    timespec_t rqtp, rmtp ;
#endif

    found = FALSE ;
    client->done = FALSE ;
    client->error = 0 ;
    client->curstation = station_number ;
    curclient = (pclient_station) ((uintptr_t) client + client->offsets[station_number]) ;
    curclient->last_attempt = dtime () ;
    srvr = curclient->base ;
    if (srvr == (pserver_struc) NOCLIENT)
    {
	curclient->status = CSCR_DIED ;
	return CSCR_DIED ;
    }
    if (srvr->init != 'I')
    {
	usleep (srvr->client_wait) ;
	if (srvr->init != 'I')
	    return CSCR_INIT ;
    }
    client->client_uid = geteuid () ;

/* Try to find free service request buffer, logic is stolen from digitizer server */
    for (i = 0 ; i <= MAXCLIENTS ; i++)
        if ((srvr->svcreqs[i].clientseg == NOCLIENT) &&
	    ((empty_string(srvr->svcreqs[i].clientname)) ||
	     (client_match(srvr->svcreqs[i].clientname,client->myname))))
	{
	    semop (srvr->server_semid, &busy, 1) ;
	    if (srvr->svcreqs[i].clientseg == NOCLIENT)
	    {
		srvr->svcreqs[i].clientseg = client->client_shm ;
		semop (srvr->server_semid, &notbusy, 1) ; /* now in queue */
		found = TRUE ;
		break ;
	    }
	    else
		semop (srvr->server_semid, &notbusy, 1) ; /* Somebody filled my slot, try another */
	}
    if (! found)
	return CSCR_ENQUEUE ;
    if (srvr->server_uid == client->client_uid)
    {
	if (kill (srvr->server_pid, SIGALRM) == ERROR) /* get its attention */
	{
	    srvr->svcreqs[i].clientseg = NOCLIENT ;
	    cs_detach (client, station_number);	/*:: DSN added */
	    curclient->base = (pserver_struc) NOCLIENT ;
	    curclient->status = CSCR_DIED ;
	    return CSCR_DIED ;
	}
	sleeptime = srvr->privusec ;
    }
    else
    {
	/*if (kill (srvr->server_pid, 0))
	  {
	  srvr->svcreqs[i].clientseg = NOCLIENT ;
	  curclient->base = NOCLIENT ;
	  curclient->status = CSCR_DIED ;
	  return CSCR_DIED ;
	  } */
	sleeptime = srvr->nonusec ;
    }
    sofar = 0 ;
    while ((! client->done) && (sofar <= srvr->client_wait))
    {
#ifdef SOLARIS2
	if (sleeptime >= 1000000)
	    sleep (sleeptime / 1000000) ;
	else
	{
	    rqtp.tv_sec = 0 ;
	    rqtp.tv_nsec = 1000 * sleeptime ;
	    err = nanosleep (&rqtp, &rmtp) ;
	}
#else
	usleep (sleeptime) ;
#endif
	sofar = sofar + sleeptime ;
    }
    if (! client->done)
    {
	semop (srvr->server_semid, &busy, 1) ;
	if (srvr->svcreqs[i].clientseg == client->client_shm) /* still valid */
	    srvr->svcreqs[i].clientseg = NOCLIENT ;
	semop (srvr->server_semid, &notbusy, 1) ;
	return CSCR_TIMEOUT ;
    }
    curclient->last_good = dtime () ;
    return client->error ;
}          

void cs_setup (pstations_struc stations, pchar name, pchar sname, boolean shared, 
	       boolean blocking, short databufs, short sels, short mask, int32_t comsize)
{
    short j ;
    boolean any ;
    config_struc cfg ;
    char str1[CFGWIDTH] ;
    char str2[CFGWIDTH] ;
    char stemp[CFGWIDTH] ;
    char source[SECWIDTH] ;
    char filename[CFGWIDTH] ;

    if (comsize < 100)
	comsize = 100 ;
    any = sname[0] == '*' ;
/* Initialize the header */ 
    copy_cname_cs_str(stations->myname,name);
    stations->shared = shared ;
    stations->station_count = 0 ;
    stations->data_buffers = databufs ;

/* open the stations list and look for any station */
    strcpy (filename, "/etc/stations.ini") ;
    memset(&cfg,0,sizeof(cfg));
    if (open_cfg(&cfg, filename, sname))
    {
	close_cfg (&cfg) ; /* no stations */
	return ;
    }

/* outer loop looks for stations, just stores directory for now */
    do
    {
	strcpy(stemp, &cfg.lastread[1]) ;
	stemp[strlen(stemp)-1] = '\0' ; /* remove [   ] */
	upshift(stemp) ;
	source[0] = '\0' ;

/* Try to find the station directory, source, and description */
	do
	{
	    read_cfg(&cfg, str1, str2) ;
	    if (str1[0] == '\0')
		break ;
	    if (strcmp(str1, "DIR") == 0)
		strcpy(filename, str2) ;
	    else if (strcmp(str1, "SOURCE") == 0)
		strcpy(source, str2) ;
	}
	while (1) ;
//	if (strcasecmp(source, "comlink") == 0)
//      Accept any source.
	if (strlen(source) > 0)
	{
	    j = stations->station_count++ ; /* new station */
	    copy_sname_cs_str(stations->station_list[j].stationname,stemp);
	    strcpy(stations->station_list[j].directory, filename) ;
	    stations->station_list[j].comoutsize = comsize ;
	    stations->station_list[j].selectors = sels ;
	    stations->station_list[j].mask = mask ;
	    stations->station_list[j].segkey = NOCLIENT ;
	    stations->station_list[j].blocking = blocking ;
	}
    }
    while (any && (! skipto (&cfg, sname))) ;
    close_cfg(&cfg) ;
      
    for (j = 0 ; j < stations->station_count ; j++)
    {
/* Try to open the station.ini file in this station's directory */
	strcpy (filename, stations->station_list[j].directory) ;
	addslash (filename) ;
	strcat (filename, "station.ini") ;
//	if (! open_cfg(&cfg, filename, "comlink"))
	if ((strlen(source) > 0) && ! open_cfg(&cfg, filename, source))
	{
	    do
	    {
		read_cfg(&cfg, str1, str2) ;
		if (str1[0] == '\0')
		    break ;
		else if (strcmp(str1, "SEGID") == 0)
		    stations->station_list[j].segkey = atoi((pchar) &str2) ;
	    }
	    while (1) ;
	    close_cfg(&cfg) ;
	}
    }
}

void cs_remove (pstations_struc stations, short num)
{
    short i ;
      
    for (i = num ; i < stations->station_count - 1 ; i++)
        stations->station_list[i] = stations->station_list[i + 1] ;
    stations->station_count-- ;
}

/* try to link to server's shared memory segment */
void cs_link (pclient_struc client, short station_number, boolean first)
{
    int shmid ;
    pclient_station curclient ;
    pserver_struc srvr ;
      
    curclient = (pclient_station) ((uintptr_t) client + client->offsets[station_number]) ;
    shmid = shmget(curclient->seg_key, sizeof(tserver_struc), PERM) ;
    if (shmid == ERROR)
    {
	cs_detach (client, station_number);	/*:: DSN added */
	curclient->base = (pserver_struc) NOCLIENT ;
	curclient->status = CSCR_DIED ;
    }
    else
    {
	curclient->base = (pserver_struc) shmat(shmid, NULL, 0) ;
	if (curclient->base == (pserver_struc) ERROR)
	    curclient->status = CSCR_DIED ;
	else
	{
	    srvr = curclient->base ;
	    if ((client->client_uid == srvr->server_uid) 
		&& (kill(srvr->server_pid, 0) == ERROR))
	    {
		curclient->status = CSCR_DIED ;
		shmdt ((pchar)srvr) ;
		curclient->base = (pserver_struc) NOCLIENT ;
	    }
	    else
	    {
		if (first)
		    curclient->servcode = srvr->servcode ;
		curclient->status = CSCR_GOOD ;
	    }
	}
    }
}

/* Try to do CSCM_ATTACH command */
void cs_attach (pclient_struc client, short station_number)
{
    pclient_station curclient ;
      
    curclient = (pclient_station) ((uintptr_t) client + client->offsets[station_number]) ;
    if (curclient->base == (pserver_struc) ERROR)
	curclient->status = CSCR_DIED ;
    else
    {
	curclient->command = CSCM_ATTACH ;
	curclient->status = cs_svc (client, station_number) ;
    }
}

pclient_struc cs_gen (pstations_struc stations)
{
    pclient_struc me ;
    pclient_station this ;
    int myshm ;
    int j, k ;
    int32_t total, dsize, curoff, inoff, outoff, doff ;
    seltype *psel ;
    comstat_rec *pcr ;

    struct sigaction action;
    /* Set up a permanently installed signal handler for SIG_ALRM. */
    action.sa_handler = cs_sig_alrm;
    action.sa_flags = 0;
    sigemptyset (&(action.sa_mask));
    sigaction (SIGALRM, &action, NULL);

/* Figure out how large it needs to be for all stations, all values are double word aligned */  
    total = (sizeof(tclient_struc) + 7) & 0xFFFFFFF8 ;
    dsize = (sizeof(tdata_user) + 7) & 0xFFFFFFF8 ;
    total = total + (dsize * stations->data_buffers) ;
    for (j = 0 ; j < stations->station_count ; j++)
    {
	total = total + ((sizeof(tclient_station) + 7) & 0xFFFFFFF8) ;
	if ((j == 0) || ((! stations->shared) && (stations->station_list[j].comoutsize)))
	    total = (total + ((stations->station_list[j].comoutsize) + 115)) & 0xFFFFFFF8 ;
	total = (total + ((sizeof(seltype) * stations->station_list[j].selectors) + 7)) & 0xFFFFFFF8 ;
    }
 
/* Try to create the shared memory segment */
    myshm = shmget(IPC_PRIVATE, total, IPC_CREAT | PERM) ;
    if (myshm == ERROR)
	return (pclient_struc) CSCR_PRIVATE ;
    me = (pclient_struc) shmat(myshm, NULL, 0) ;
    if (me == (pclient_struc) ERROR)
	return (pclient_struc) CSCR_PRIVATE ;

/* Start filling global fields */
    copy_cname_cs_cs(me->myname,stations->myname);
    me->client_pid = getpid () ;
    me->client_shm = myshm ;
    me->spare = 0 ;
    me->done = FALSE ;
    me->error = 0 ;
    me->maxstation = stations->station_count ;
    me->curstation = 0 ;
    for (j = 0 ; j < MAXSTATIONS ; j++)
        me->offsets[j] = 0 ;
    curoff = (sizeof(tclient_struc) + 7) & 0xFFFFFFF8 ;
 
/* If shared command output buffer, allocate it at the beginning */
    if (stations->shared)
    {
	inoff = curoff ;
	curoff = curoff + 104 ;
	outoff = curoff ;
	curoff = curoff + ((stations->station_list[0].comoutsize + 7) & 0xFFFFFFF8) ;
    }

/* Data buffers are always shared */
    doff = curoff ;
    curoff = curoff + (dsize * stations->data_buffers) ;
     
/* For each station, build entry */
    for (j = 0 ; j < stations->station_count ; j++)
    {
	this = (pclient_station) ((uintptr_t) me + curoff) ; 
	curoff = curoff + ((sizeof(tclient_station) + 7) & 0xFFFFFFF8) ;
	me->offsets[j] = (uintptr_t) this - (intptr_t) me ;
	copy_sname_cs_cs(this->name,stations->station_list[j].stationname);
	this->seg_key = stations->station_list[j].segkey ;
	this->command = CSCM_ATTACH ;
	this->blocking = stations->station_list[j].blocking ;
	this->status = OK ;
	this->next_data = 0 ;
	this->last_attempt = 0.0 ;
	this->last_good = 0.0 ;
	this->servcode = 0.0 ;
	this->base = (pserver_struc) NOCLIENT ;
	if (! stations->shared)
	{
	    inoff = curoff ;
	    curoff = curoff + 104 ;
	    outoff = curoff ;
	    curoff = curoff + ((stations->station_list[j].comoutsize + 7) & 0xFFFFFFF8) ;
	}
	pcr = (pvoid) ((uintptr_t) me + outoff) ;
	pcr->command_tag = 0 ;
	pcr->completion_status = CSCS_IDLE ;
	this->cominoffset = inoff ;
	this->comoutoffset = outoff ;
	this->comoutsize = stations->station_list[j].comoutsize ;
	this->dbufoffset = doff ;
	this->dbufsize = dsize ;
	this->maxdbuf = stations->data_buffers ;
	this->reqdbuf = this->maxdbuf ;
	this->valdbuf = 0 ;
	this->seqdbuf = CSQ_FIRST ;
	this->startdbuf = 0.0 ;
	this->seloffset = curoff ;
	psel = (pvoid) ((uintptr_t) me + curoff) ;
	curoff = (curoff + ((sizeof(seltype) * stations->station_list[j].selectors) + 7)) & 0xFFFFFFF8 ;
	this->maxsel = stations->station_list[j].selectors ;
	for (k = DATAQ ; k <= CHAN ; k++)
	{
	    this->sels[k].first = 0 ;
	    this->sels[k].last = 0 ;
	}
	this->datamask = stations->station_list[j].mask ;
	strcpy(psel[0], "?????") ;

/* now attach to the server's shared memory segment */
	cs_link (me, j, TRUE) ;
	if (this->base != (pserver_struc) NOCLIENT)
	    cs_attach (me, j) ;
    }
    return me ;
}

byte cs_check (pclient_struc client, short station_number, double now)
{
    pclient_station curclient ;
    pserver_struc srvr ;
      
    curclient = (pclient_station) ((uintptr_t) client + client->offsets[station_number]) ;
    if (curclient->status != CSCR_GOOD)
    {
	if ((curclient->last_attempt + CS_CHECK_INTERVAL) < now)
	{
	    curclient->last_attempt = now ;
	    if (curclient->base == (pserver_struc) NOCLIENT)
		cs_link (client, station_number, FALSE) ;
	    if (curclient->base != (pserver_struc) NOCLIENT)
		cs_attach (client, station_number) ;
	}
    }
    if ((curclient->status == CSCR_GOOD) && (curclient->base != (pserver_struc) NOCLIENT))
    {
	srvr = curclient->base ;
	if (curclient->servcode != srvr->servcode)
	{
	    curclient->servcode = srvr->servcode ;
	    curclient->next_data = 0 ;
	    curclient->seqdbuf = CSQ_FIRST ;
	    cs_attach (client, station_number) ;
	    curclient->status = CSCR_CHANGE ;
	}
	else
	{
	    if (curclient->next_data < srvr->next_data)
		return CSCR_GOOD ;
	    if ((curclient->last_attempt + CS_CHECK_INTERVAL) < now)
	    {
		curclient->last_attempt = now ;
		if ((client->client_uid == srvr->server_uid) 
		    && (kill(srvr->server_pid, 0) == ERROR))
		{
		    curclient->status = CSCR_DIED ;
		    shmdt ((pchar)srvr) ;
		    curclient->base = (pserver_struc) NOCLIENT ;
		}
		else
		    return CSCR_GOOD ;
	    }
	    else
		return CSCR_NODATA ;
	}
    }
    return curclient->status ;
}

void cs_detach (pclient_struc client, short station_number)
{
    pclient_station this ;
      
    this = (pclient_station) ((uintptr_t) client + client->offsets[station_number]) ;
/* Detach from server segment */
    if (! (this->base == (pserver_struc) NOCLIENT))
	shmdt((pchar)this->base) ;
    this->status = CSCR_DIED ;
    this->base = (pserver_struc) NOCLIENT ;
}

void cs_off (pclient_struc client)
{
    short j ;
    int seg ;

/* Detach from all servers */      
    for (j = 0 ; j < client->maxstation ; j++)
        cs_detach (client, j) ;

/* Detach from my segment */
    seg = client->client_shm ;
    shmdt((pchar)client) ;

/* Delete my segment */
    shmctl(seg, IPC_RMID, NULL) ;
}

short cs_scan (pclient_struc client, boolean *alert)
{
    byte old_status, result ;
    short old_station ;
    pclient_station this ;
    double curtime ;
      
    old_station = client->curstation ;
    curtime = dtime () ;
    *alert = FALSE ;
    do
    {
	if (++client->curstation >= client->maxstation)
	    client->curstation = 0 ;
	this = (pclient_station) ((uintptr_t) client + client->offsets[client->curstation]) ;
	old_status = this->status ;
	result = cs_check (client, client->curstation, curtime) ;
	*alert = (this->status != old_status) ;
	this->valdbuf = 0 ;
	if (result == CSCR_GOOD)
	{
	    this->command = CSCM_DATA_BLK ;
	    old_status = this->status ;
	    this->status = cs_svc (client, client->curstation) ;
	    *alert = *alert || (this->status != old_status) ;
	    if (*alert || (this->valdbuf != 0))
		return client->curstation ;
	}
	if (*alert)
	    return client->curstation ;
    }
    while (client->curstation != old_station) ;
    return NOCLIENT ;      
}

