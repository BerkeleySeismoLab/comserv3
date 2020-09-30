#ifndef COMSERV_VARS_H
#define COMSERV_VARS_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include <stdint.h>
#include <time.h>

#include "cfgutil.h"
#include "server.h"
#include "service.h"

#define MAXPROC 2 /* maximum number of service requests to process before checking serial port */
#define MAXWAIT 10 /* maximum number of seconds for clients to wait */
#define PRIVILEGED_WAIT 1000000 /* 1 second */
#define NON_PRIVILEGED_WAIT 100000 /* 0.1 second */
#define NON_PRIVILEGED_TO 60.0

/***********************************************************************
 * External comserv variables that must be defined by all servers.
 * This file can either define the variables (including initializing them)
 * or declare them.  To define them, one and only one source file per
 * program should define the variable DEFINE_COMSERV_VARS, eg:
 * 	#define DEFINE_COMSERV_GENERIC_VARS
 ***********************************************************************/

/***********************************************************************
 * External variables that require initialization.
 ***********************************************************************/

#ifdef DEFINE_COMSERV_VARS
#define EXTERN
EXTERN int32_t blockmask = 0 ;
EXTERN int32_t noackmask = 0 ;
EXTERN int32_t polltime = 50000 ;
EXTERN int32_t grpsize = 1 ;
EXTERN int32_t grptime = 5 ;
EXTERN tclients clients[MAXCLIENTS] ;
EXTERN short highclient = 0 ;
EXTERN short resclient = 0 ;
EXTERN short uids = 0 ;
EXTERN pchar dest = NULL ;
EXTERN int32_t start_time = 0 ;      /* For seconds in operation */
EXTERN boolean verbose = TRUE ;   /* normal, client on/off etc. */
EXTERN boolean rambling = FALSE ; /* Incoming packets display a line */
EXTERN boolean insane = FALSE ;   /* Client commands are shown */
EXTERN boolean override = FALSE ; /* Override station/component */
EXTERN boolean stop = FALSE ;
EXTERN short combusy = NOCLIENT ;           /* <>NOCLIENT if processing a command for a station */
EXTERN int32_t netto = 120 ; /* network timeout */
EXTERN int32_t netdly = 30 ; /* network reconnect delay */
EXTERN int32_t netto_cnt = 0 ; /* timeout counter (seconds) */
EXTERN int32_t netdly_cnt = 0 ; /* reconnect delay */
EXTERN byte cmd_seq = 1 ;
EXTERN linkstat_rec linkstat = { TRUE, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, "V2.3", 'A', CSF_QSL, "" } ;
EXTERN char station_desc[CFGWIDTH] = "" ;
EXTERN double curtime ;
#else
#define EXTERN extern
EXTERN int32_t blockmask ;
EXTERN int32_t noackmask ;
EXTERN int32_t polltime ;
EXTERN int32_t grpsize ;
EXTERN int32_t grptime ;
EXTERN tclients clients[MAXCLIENTS] ;
EXTERN short highclient ;
EXTERN short resclient ;
EXTERN short uids ;
EXTERN pchar dest ;
EXTERN int32_t start_time ;
EXTERN boolean verbose ;
EXTERN boolean rambling ;
EXTERN boolean insane ;
EXTERN boolean override ;
EXTERN boolean stop ;
EXTERN short combusy ;
EXTERN int32_t netto ;
EXTERN int32_t netdly ;
EXTERN int32_t netto_cnt ;
EXTERN int32_t netdly_cnt ;
EXTERN byte cmd_seq ;
EXTERN linkstat_rec linkstat ;
EXTERN char station_desc[CFGWIDTH] ;
EXTERN double curtime ;
#endif

/***********************************************************************
 * External variables that do NOT require initialization.
 ***********************************************************************/

EXTERN tring rings[NUMQ] ;                /* Access structure for rings */
EXTERN pserver_struc base ;
EXTERN config_struc cfg ;
EXTERN tuser_privilege user_privilege ;
EXTERN char str1[CFGWIDTH] ;
EXTERN char str2[CFGWIDTH] ;
EXTERN char stemp[CFGWIDTH] ;
EXTERN char dbuf[1024] ;

#endif
