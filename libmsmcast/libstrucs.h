/*  Libmsmcast internal data structures

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017-2019 Certified Software Corporation
    Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#ifndef LIBSTRUCS_H
#define LIBSTRUCS_H
#define VER_LIBSTRUCS 2

#include "memutil.h"
#include "libsampglob.h"
#include "libclient.h"
#include "platform.h"
#include "libtypes.h"

/* Make the buffer the 2x size of the maximum expected MSEED packet size. */
#define MAX_MSEED_BLKSIZE 512
#define TCPBUFSZ MAX_MSEED_BLKSIZE * 2
#define DEFAULT_PIU_RETRY 5 * 60 /* Port in Use */
#define DEFAULT_DATA_TIMEOUT 5 * 60 /* Data timeout */
#define DEFAULT_DATA_TIMEOUT_RETRY 5 * 60 ;
#define DEFAULT_STATUS_TIMEOUT 5 * 60 /* Status timeout */
#define DEFAULT_STATUS_TIMEOUT_RETRY 5 * 60 ;
#define MIN_MSG_QUEUE_SIZE 10 /* Minimum number of client message buffers */

typedef longint taccminutes[60] ;
typedef struct { /* for building one dp statistic */
    longint accum ;
    longint accum_ds ; /* for datastream use */
    pointer ds_lcq ; /* lcq associated with this statistic, if any */
    taccminutes minutes ;
    longint hours[24] ;
} taccmstat ;
typedef taccmstat taccmstats[AC_LAST + 1] ;

typedef struct { /* shared variables with client. Require mutex protection */
    enum tliberr liberr ; /* last error condition */
    enum tlibstate target_state ; /* state the client wants */
    integer stat_minutes ;
    integer stat_hours ;
    longint total_minutes ;
    taccmstats accmstats ;
    topstat opstat ; /* operation status */
    word first_share_clear ; /* start of shared fields cleared after de-registration */
    word last_share_clear ; /* last address cleared after de-registration */
} tshare ;

typedef struct { /* this is the actual context which is hidden from clients */
    pthread_mutex_t mutex ;
    pthread_mutex_t msgmutex ;
    pthread_t threadid ;
    enum tlibstate libstate ; /* current state of this station */
    boolean terminate ; /* set TRUE to terminate thread */
//::    tseed_net network ;
//::    tseed_stn station ;
    tpar_create par_create ; /* parameters to create context */
    tpar_register par_register ; /* registration parameters */
    tmeminst thrmem ; /* Station oriented memory */
    tshare share ; /* variables shared with client */
    longword msg_count ; /* message count */
    integer reg_tries ; /* number of times we have tried to register */
    integer minute_counter ; /* 6 * 10 seconds = 60 seconds */
    integer data_timeout ; /* threshold seconds for data_timer */
    integer data_timeout_retry ; /* seconds to wait before retrying after data timeout */
    integer status_timeout ; /* threshold seconds for status_timer */
    integer status_timeout_retry ; /* seconds to wait before retrying after status timeout */
    integer piu_retry ; /* seconds to wait before retrying after port in use error */
    integer retry_index ; /* To get number of seconds from RETRY_TAB */
    longint last_ten_sec ; /* last ten second value */
    double last_100ms ; /* last time ran 100ms timer routine */
    double saved_data_timetag ; /* for latency calculations */
    longword dpstat_timestamp ; /* for dp statistics */
    word cur_verbosity ; /* current verbosity */
    integer cpath ; /* socket */
    integer high_socket ; /* Highest socket number */
    struct sockaddr msock ; /* For IPV4 */
    struct sockaddr_in6 msock6 ; /* For IPV6 */
    tstate_call state_call ; /* buffer for building state callbacks */
    tmsg_call msg_call ; /* buffer for building message callbacks */
    tminiseed_call miniseed_call ; /* buffer for building miniseed callbacks */
    pmsgqueue msgqueue, msgq_in, msgq_out ;
    double ref2016 ; /* for converting between system time and since 2016 time */
    boolean nested_log ; /* we are actually writing a log record */
    integer log_timer ; /* count-down since last added message line */
    plcq msg_lcq ;
    string31 station_ident ; /* network-station */
    /* following are cleared after de-registering */
    word first_clear ; /* first byte to clear */
    boolean ipv6 ; /* TRUE if using IPV6 */
    boolean registered ; /* registered with q660 */
    char tcpbuf[TCPBUFSZ] ;
    integer tcpidx ;
    integer lastidx ;
    double data_timetag ;
    double ll_data_timetag ;
    word last_clear ; /* end of clear */
} tmsmcast ;
typedef tmsmcast *pmsmcast ;

extern void lock (pmsmcast msmcast) ;
extern void unlock (pmsmcast msmcast) ;
extern void sleepms (integer ms) ;
extern void lib_create_660 (tcontext *ct, tpar_create *cfg) ;
extern enum tliberr lib_destroy_660 (tcontext *ct) ;
extern enum tliberr lib_register_660 (pmsmcast msmcast, tpar_register *rpar) ;
extern void new_state (pmsmcast msmcast, enum tlibstate newstate) ;
extern void state_callback (pmsmcast msmcast, enum tstate_type stype, longword val) ;
extern longword make_bitmap (longword bit) ;
extern void set_liberr (pmsmcast msmcast, enum tliberr newerr) ;
#endif
