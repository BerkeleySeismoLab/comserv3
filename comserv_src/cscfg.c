/*   Server Config file parser
     Copyright 1994-1998 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 23 Mar 94 WHO Pulled out of server.c
    1 27 Mar 94 WHO Merged blockette ring changed to separate rings.
    2 16 Apr 94 WHO Setup fake ultra structure for Shear stations.
    3  6 Jun 94 WHO Add setting user privilege structure.
    4  9 Jun 94 WHO Cleanup to avoid warnings.
    5 11 Aug 94 WHO Add setting of ipport.
    6 20 Jun 95 WHO allow setting of netto, netdly, grpsize, and
                    grptime parameters.
    7  2 Oct 95 WHO Allow setting of sync parameter.
    8  8 Oct 95 WHO Allow setting of flow parameter.
    9 17 Nov 95 WHO sync is now called notify in config file.
   10 28 Jan 96 DSN Add LOG_SEED and TIMING_SEED directives.
   11 29 May 96 WHO Start of conversion to run on OS9.
   12 13 Jun 96 WHO Add setting of "SEEDIN" flag.
   13  3 Aug 96 WHO Add setting of "ANYSTATION" and "NOULTRA" flags.
   14  3 Dec 96 WHO Add setting of "BLKBUFS".  
   15  7 Dec 96 WHO Add setting of "UDPADDR".
   16 16 Oct 97 WHO Add "MSHEAR" as duplicate of "SEEDIN" flag. Add
                    VER_CSCFG
   17 22 Oct 97 WHO Add setting of verbosity flags based on vopttable.
   18 29 Nov 97 DSN Add optional LOCKFILE directive again.
   19  8 Jan 98 WHO Lockfile not for OS9 version.
   20 23 Dec 98 WHO Add setting of "LINKRETRY".
   21  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
		       comserv_split is used to avoid the conflict with the other functions
		       with similar names.
   22 21 Apr 04 DSN Added myipaddr config directive.
   23 29 Sep 2020 DSN	Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#ifndef _OSK
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
#else
#include <stdlib.h>
#include <sgstat.h>
#include <module.h>
#include <types.h>
#include <sg_codes.h>
#include <modes.h>
#include "os9inet.h"
#endif
#include "quanstrc.h"
#include "stuff.h"
#include "service.h"
#include "cfgutil.h"
#include "server.h"
#include "timeutil.h"
#ifdef _OSK
#include "os9stuff.h"
#endif

short VER_CSCFG = 23 ;

extern char log_channel_id[4] ;
extern char log_location_id[3] ;
extern char clock_channel_id[4] ;
extern char clock_location_id[3] ;
#ifdef COMSERV2
extern complong station ;
#else
extern string15 station ;
#endif
extern config_struc cfg ;
extern tuser_privilege user_privilege ;
extern char str1[CFGWIDTH] ;
extern char str2[CFGWIDTH] ;
extern char stemp[CFGWIDTH] ;
extern char port[SECWIDTH] ;
extern char ipport[SECWIDTH] ;
extern char ipaddr[SECWIDTH] ;
#ifdef	MYIPADDR
extern char myipaddr[SECWIDTH] ;
#endif
#ifndef _OSK
extern char lockfile[CFGWIDTH] ;
#endif
extern int32_t baud ;
extern char parity ; 
extern int32_t polltime ;
extern int32_t reconfig_on_err ;
extern int32_t grpsize ;
extern int32_t grptime ;
extern int32_t link_retry ;
extern tring rings[NUMQ] ;
extern int32_t blockmask ;
extern int32_t netto ;
extern int32_t netdly ;
extern int segkey ;
extern unsigned char buf[BLOB] ;
extern tclients clients[MAXCLIENTS] ;
extern string3 seed_names[20][7] ;
extern seed_net_type network ;
extern linkstat_rec linkstat ;
extern boolean verbose ;
extern boolean rambling ;
extern boolean insane ;
extern boolean override ;
extern boolean notify ;
extern boolean flow ;
extern boolean udplink ;
extern boolean seedin ;
extern boolean anystation ;
extern boolean noultra ;
extern short sequence_mod ;
extern short highclient ;
extern short uids ;
extern ultra_type *pultra ;
extern location_type seed_locs[20][7] ;
#ifdef _OSK
extern voptentry *pvopt ;
#endif
      
static char s1[CFGWIDTH], s2[CFGWIDTH], s3[20] ;

double dtime (void) ; 
void terminate (pchar s) ;

/* Sets "seed" to the nth element in the passed string if there is one */
void parsechan (pchar s, short n, seed_name_type *seed, location_type *loc, short *phys)
{
    short j ;
      
    strcpy(s1, s) ;
    upshift (s1) ;
    (*seed)[0] = '\0' ;
    *phys = 0 ;
    for (j = 0 ; j <= n ; j++)
    {
	comserv_split (s1, s2, ',') ;
	if ((s1[0] != '\0') && (j == n)) /* there is a replacement string for requested index */
	{
	    comserv_split (s1, s3, '#') ;
	    if (s3[0] != '\0')
		*phys = atoi(s3) ;
	    comserv_split (s1, s3, '-') ;
	    if (s3[0] == '\0')
	    {
		strcpy (s3, s1) ; /* no location, move to channel */
		strcpy (s1, "  ") ;
	    }
	    while (strlen(s1) < 2)
		strcat(s1, " ") ;
	    while (strlen(s3) < 3)
		strcat(s3, " ") ;
	    memcpy((pchar) seed, s3, 3) ;
	    memcpy((pchar) loc, s1, 2) ;
	    return ;
	}
	strcpy (s1, s2) ;
	if (s1[0] == '\0')
	    return ;
    }
}

void set_verb (int i)
{
    verbose = i > 0 ;
    rambling = i > 1 ;
    insane = i > 2 ;
}

void readcfg (void)
{                           /* Read parameters for this station */
    pchar tmp ;
    short i, j, k, l ;
    widestring streams[7] = { "", "", "", "", "", "", "" } ;
    short rates[7] = { 20, 80, 80, 10, 1, -10, -100 } ; /* believable rates */
    seed_name_type seed ;
    location_type loc ;
    int ultra_size ;
    boolean supercal = FALSE ;
    boolean qapcal = FALSE ;
    chan_record *pcr ;
    cal_record *pcal ;
    eachcal *pec ;
 
    do
    {
	read_cfg(&cfg, str1, str2) ;
	if (str1[0] == '\0')
	    break ;
#ifndef _OSK
	if (strcmp(str1, "LOCKFILE") == 0)
	    strcpy(lockfile, str2) ;
#endif
	if (strcmp(str1, "PORT") == 0)
	    strcpy(port, str2) ;
	if (strcmp(str1, "IPPORT") == 0)
	    strcpy(ipport, str2) ;
#ifdef  MYIPADDR
	if (strcmp(str1, "MYIPADDR") == 0)
	    strcpy(myipaddr, str2) ;
#endif
	if (strcmp(str1, "UDPADDR") == 0)
	{
	    strcpy(ipaddr, str2) ;
	    udplink = TRUE ;
	}
	else if (strcmp(str1, "BAUD") == 0)
	    baud = atol(str2) ;
	else if (strcmp(str1, "PARITY") == 0)
	    parity = toupper(str2[0]) ;
	else if (strcmp(str1, "VERBOSITY") == 0)
	{
#ifdef _OSK
	    pvopt->verbosity = atoi((pchar)&str2) ;
	    set_verb (longhex(pvopt->verbosity)) ;
#else
	    set_verb (atoi((pchar)&str2)) ;
#endif
	}
	else if (strcmp(str1, "OVERRIDE") == 0)
	    override = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "ANYSTATION") == 0)
	    anystation = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "NOULTRA") == 0)
	    noultra = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "NOTIFY") == 0)
	    notify = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "FLOW") == 0)
	    flow = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "SEEDIN") == 0)
	    seedin = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "MSHEAR") == 0)
	    seedin = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	else if (strcmp(str1, "STATION") == 0)
	{
	    str2[SERVER_NAME_SIZE-1] = '\0';
	    copy_sname_cs_str(station,str2);
	}
	else if (strcmp(str1, "LOG_SEED") == 0)
	{
	    upshift(str2) ;
	    tmp = strchr(str2, '-') ;
	    if (tmp)
	    {
		*tmp++ = '\0' ;
		strncpy(log_location_id, str2, 2);
	    }
	    else
		tmp = str2;
	    tmp[3] = '\0';
	    strncpy(log_channel_id, tmp, 3);
	}
	else if (strcmp(str1, "TIMING_SEED") == 0)
	{
	    upshift(str2) ;
	    tmp = strchr(str2, '-') ;
	    if (tmp)
	    {
		*tmp++ = '\0' ;
		strncpy(clock_location_id, str2, 2);
	    }
	    else
		tmp = str2;
	    tmp[3] = '\0';
	    strncpy(clock_channel_id, tmp, 3);
	}
	else if (strcmp(str1, "SEGID") == 0)
	    segkey = atoi((pchar)&str2) ;
	else if (strcmp(str1, "POLLUSECS") == 0)
	    polltime = atol((pchar)&str2) ;
	else if (strcmp(str1, "DATABUFS") == 0)
	    rings[DATAQ].count = atoi((pchar)&str2) ;
	else if (strcmp(str1, "DETBUFS") == 0)
	    rings[DETQ].count = atoi((pchar)&str2) ;
	else if (strcmp(str1, "CALBUFS") == 0)
	    rings[CALQ].count = atoi((pchar)&str2) ;
	else if (strcmp(str1, "TIMBUFS") == 0)
	    rings[TIMQ].count = atoi((pchar)&str2) ;
	else if (strcmp(str1, "MSGBUFS") == 0)
	    rings[MSGQ].count = atoi((pchar)&str2) ;
	else if (strcmp(str1, "BLKBUFS") == 0)
	    rings[BLKQ].count = atoi((pchar)&str2) ;
	else if (strcmp(str1, "RECONFIG") == 0)
	    reconfig_on_err = atoi((pchar)&str2) ;
	else if (strcmp(str1, "NETTO") == 0)
	    netto = atol((pchar)&str2) ;
	else if (strcmp(str1, "NETDLY") == 0)
	    netdly = atol((pchar)&str2) ;
	else if (strcmp(str1, "GRPSIZE") == 0)
	{
	    grpsize = atol((pchar)&str2) ;
	    if (grpsize < 1)
		grpsize = 1 ;
	    if (grpsize > 63)
		grpsize = 63 ;
	}
	else if (strcmp(str1, "GRPTIME") == 0)
	    grptime = atol((pchar)&str2) ;
	else if (strcmp(str1, "LINKRETRY") == 0)
	    link_retry = atol((pchar)&str2) ;
	else
	{  /* look for client[xx]=name[,timeout] */
	    strcpy(stemp, str1) ;
	    stemp[6] = '\0' ; /* strip down to "client" only */
	    if (strcmp(stemp, "CLIENT") == 0)
	    {
		i = highclient++ ;
		/* get client name */
		upshift(str2) ;
		comserv_split (str2, stemp, ',') ;
		copy_cname_cs_str(clients[i].client_name,str2);
		/* get timeout, if any */
		if (stemp[0] != '\0')
		{
		    clients[i].timeout = atol((pchar)&stemp) ;
		    if (clients[i].timeout)
		    {
			set_bit (&blockmask, i) ;
			clients[i].last_service = dtime () ;
			clients[i].active = TRUE ;
			clients[i].blocking = TRUE ;
		    }
		}
	    }
	    else
	    { /* look for uidxxx=nnn */
		strcpy(stemp, str1) ;
		stemp[3] = '\0' ; /* strip down to "uid" only */
		if (strcmp(stemp, "UID") == 0)
		{
		    i = uids++ ;
		    /* get user id */
		    str_right (stemp, &str1[2]) ;
		    user_privilege[i].user_id = atoi((pchar)&stemp) ;
		    /* get privilege mask */
		    user_privilege[i].user_mask = atoi((pchar)&str2) ;
		}
	    }
	}
    }
    while (1) ;

/* Process the [shear] section, if there is one */
    if (! skipto (&cfg, "shear"))
    {
	linkstat.data_format = CSF_Q512 ;
	linkstat.ultraon = FALSE ;
	do
	{
	    read_cfg (&cfg, str1, str2) ;
	    if (str1[0] == '\0')
		break ;
	    if (strcmp(str1, "NETWORK") == 0)
	    {
		upshift(str2) ;
		for (i = 0 ; i < 2 ; i++)
		    if (i < strlen(str2))
			network[i] = str2[i] ;
		    else
			network[i] = ' ' ;
	    }
	    else if (strcmp(str1, "SEQMOD") == 0)
		sequence_mod = atoi(str2) ;
	    else if (strcmp(str1, "CALIB") == 0)
	    {
		upshift(str2) ;
		if (strcmp(str2, "SUPERCAL") == 0)
		    supercal = TRUE ;
		else if (strcmp(str2, "QAPCAL") == 0)
		    qapcal = TRUE ;
		else
		    terminate ("Invalid calibrator\n") ;
	    }
	    else if (strcmp(str1, "VBB") == 0)
		strcpy (streams[0], str2) ;
	    else if (strcmp(str1, "VSP") == 0)
		strcpy (streams[1], str2) ;
	    else if (strcmp(str1, "LG") == 0)
		strcpy (streams[2], str2) ;
	    else if (strcmp(str1, "MP") == 0)
		strcpy (streams[3], str2) ;
	    else if (strcmp(str1, "LP") == 0)
		strcpy (streams[4], str2) ;
	    else if (strcmp(str1, "VLP") == 0)
		strcpy (streams[5], str2) ;
	    else if (strcmp(str1, "ULP") == 0)
		strcpy (streams[6], str2) ;
	}
	while (1) ;
	k = 0 ;
	for (i = 0 ; i <= 6 ; i++)
	    for (j = 0 ; j <= 19 ; j++)
	    {
		memset((pchar) &(seed_locs[j][i]), ' ', 2) ; /* Initialize to spaces */
		parsechan(streams[i], j, &seed, &loc, &l) ;
		if (seed[0] != '\0')
		    k++ ;
	    }
	ultra_size = k * sizeof(chan_record) + sizeof(ultra_type) + 16 ;
	if (supercal || qapcal)
	    ultra_size = ultra_size + (sizeof(cal_record) - (MAXCAL - 2) * sizeof(eachcal)) ;
	pultra = (pvoid) malloc(ultra_size) ;
	/* Clear the whole bugger out, this saves alot of assignments */
	memset ((pchar) pultra, '\0', ultra_size) ;
	pultra->vcovalue = -1 ;
	pultra->umass_ok = supercal || qapcal ;
	pultra->usedcount = k ;
	pultra->usedoffset = sizeof(ultra_type) ;
	pcr = (pvoid) ((uintptr_t) pultra + pultra->usedoffset) ;
	for (i = 0 ; i <= 6 ; i++)
	    for (j = 0 ; j <= 19 ; j++)
	    {
		parsechan(streams[i], j, &seed, &loc, &l) ;
		if (seed[0] != '\0')
		{
		    memcpy(pcr->seedname, seed, 3) ;
		    memcpy(pcr->seedloc, loc, 2) ;
		    pcr->stream = i ;
		    pcr->physical = l ;
		    pcr->available = 3 ;
		    pcr->enabled = 3 ;
		    pcr->rate = rates[i] ;
		    memcpy((seed_names[j][i]), seed, 3) ;
		    memcpy((seed_locs[j][i]), loc, 2) ;
		    pcr++ ;
		}
	    }
	if (qapcal || supercal)
	{
	    pultra->calcount = 2 ;
	    pultra->caloffset = ((uintptr_t) pcr - (uintptr_t) pultra + 7) & 0xFFFFFFF8 ;
	    pcal = (pvoid) ((uintptr_t) pultra + pultra->caloffset) ;
	    pcal->number = 2 ;
	    pcal->mass_ok = TRUE ;
	    for (i = 0 ; i <= 1 ; i++)
	    {
		pec = &(pcal->acal[i]) ;
		pec->board = i + 1 ;
		pec->map = 7 << (3 * i) ;
		if (qapcal)
		{
		    strpas(pec->name, "QAPCAL") ;
		    pec->coupling_option = FALSE ;
		    pec->polarity_option = TRUE ;
		    pec->min_mass_dur = 500 ;
		    pec->max_mass_dur = 500 ;
		    pec->inc_mass_dur = 10 ;
		    pec->def_mass_dur = 500 ;
		    pec->waveforms = 1 << STEP ;
		    pec->min_amp = -6 ;
		    pec->max_amp = -6 ;
		    pec->amp_step = 1 ;
		    pec->durations[STEP].min_dur = 60 ;
		    pec->durations[STEP].max_dur = 14400 ;
		    pec->durations[STEP].inc_dur = 60 ;
		}
		else
		{
		    strpas(pec->name, "SUPERCAL") ;
		    pec->coupling_option = TRUE ;
		    pec->polarity_option = TRUE ;
		    pec->min_mass_dur = 500 ;
		    pec->max_mass_dur = 500 ;
		    pec->inc_mass_dur = 10 ;
		    pec->def_mass_dur = 500 ;
		    for (j = SINE ; j <= WRAND ; j++)
			set_bit((int32_t *)&pec->waveforms, j) ;
		    pec->min_amp = -96 ;
		    pec->amp_step = 6 ;
		    pec->durations[STEP].min_dur = 1 ;
		    pec->durations[STEP].max_dur = 14400 ;
		    pec->durations[STEP].inc_dur = 1 ;
		    pec->durations[SINE].min_dur = 60 ;
		    pec->durations[SINE].max_dur = 14400 ;
		    pec->durations[SINE].inc_dur = 60 ;
		    pec->durations[RAND].min_dur = 60 ;
		    pec->durations[RAND].max_dur = 14400 ;
		    pec->durations[RAND].inc_dur = 60 ;
		    pec->durations[WRAND].min_dur = 60 ;
		    pec->durations[WRAND].max_dur = 14400 ;
		    pec->durations[WRAND].inc_dur = 60 ;
		    pec->rand_min_period = 1 ;
		    pec->rand_max_period = 16 ;
		    for (j = Hz8_0000 ; j <= Hz0_0005 ; j++)
			set_bit((int32_t *)&pec->sine_freqs, j) ;
		}
	    }
	};
	linkstat.ultrarecv = TRUE ;
    }
    close_cfg(&cfg) ;
}
