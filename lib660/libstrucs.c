



/*   Lib660 internal core routines
     Copyright 2017-2021 by
     Kinemetrics, Inc.
     Pasadena, CA 91107 USA.

    This file is part of Lib660

    Lib660 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Lib660 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2017-06-05 rdr Created
    1 2018-08-28 rdr If get an error when connecting to Q330 the report it and wait.
    2 2019-06-26 rdr Add dispatch support for GPS data streams.
    3 2021-12-11 jms various temporary debugging prints.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/

#undef LINUXDEBUGPRINT

#include "libstrucs.h"
#include "libmsgs.h"
#include "libsupport.h"
#include "libcmds.h"
#include "libstats.h"
#include "libsampcfg.h"
#include "libcont.h"
#include "libfilters.h"

#define MS100 (0.1)

#ifdef X86_WIN32
static void create_mutex (pq660 q660)
{

    q660->mutex = CreateMutex(NIL, FALSE, NIL) ;
    q660->msgmutex = CreateMutex(NIL, FALSE, NIL) ;
}

static void destroy_mutex (pq660 q660)
{

    CloseHandle (q660->mutex) ;
    CloseHandle (q660->msgmutex) ;
}

void lock (pq660 q660)
{

    WaitForSingleObject (q660->mutex, INFINITE) ;
}

void unlock (pq660 q660)
{

    ReleaseMutex (q660->mutex) ;
}

void msglock (pq660 q660)
{

    WaitForSingleObject (q660->msgmutex, INFINITE) ;
}

void msgunlock (pq660 q660)
{

    ReleaseMutex (q660->msgmutex) ;
}

void sleepms (int ms)
{

    Sleep (ms) ;
}

#else

static void create_mutex (pq660 q660)
{

    pthread_mutex_init (&(q660->mutex), NULL) ;
    pthread_mutex_init (&(q660->msgmutex), NULL) ;
}

static void destroy_mutex (pq660 q660)
{

    pthread_mutex_destroy (&(q660->mutex)) ;
    pthread_mutex_destroy (&(q660->msgmutex)) ;
}

void lock (pq660 q660)
{

    pthread_mutex_lock (&(q660->mutex)) ;
}

void unlock (pq660 q660)
{

    pthread_mutex_unlock (&(q660->mutex)) ;
}

void msglock (pq660 q660)
{

    pthread_mutex_lock (&(q660->msgmutex)) ;
}

void msgunlock (pq660 q660)
{

    pthread_mutex_unlock (&(q660->msgmutex)) ;
}

void sleepms (int ms)
{
    struct timespec dly ;

    dly.tv_sec = 0 ;
    dly.tv_nsec = ms * 1000000 ; /* convert to nanoseconds */
    nanosleep (&(dly), NULL) ;
}
#endif

void state_callback (pq660 q660, enum tstate_type stype, U32 val)
{

    if (q660->par_create.call_state) {
        q660->state_call.context = q660 ;
        q660->state_call.state_type = stype ;
        memcpy(&(q660->state_call.station_name), &(q660->station_ident), sizeof(string9)) ;
        q660->state_call.info = val ;
        q660->state_call.subtype = 0 ;
        q660->par_create.call_state (&(q660->state_call)) ;
    }
}

void new_state (pq660 q660, enum tlibstate newstate)
{

    q660->libstate = newstate ;
    state_callback (q660, ST_STATE, (U32)newstate) ;
}

U32 make_bitmap (U32 bit)
{

    return (U32)1 << bit ;
}

void new_cfg (pq660 q660, U32 newbitmap)
{

    lock (q660) ;

    if (q660->share.have_config != (q660->share.have_config | newbitmap)) {
        /* something we didn't have before */
        q660->share.have_config = q660->share.have_config | newbitmap ;
        unlock (q660) ;
        state_callback (q660, ST_CFG, q660->share.have_config) ;
    } else
        unlock (q660) ;
}

void new_status (pq660 q660, U32 newbitmap)
{

    lock (q660) ;
    q660->share.have_status = q660->share.have_status | newbitmap ;
    unlock (q660) ;
    state_callback (q660, ST_STATUS, newbitmap) ;
}

#ifdef X86_WIN32
unsigned long  __stdcall libthread (pointer p)
#else
void *libthread (pointer p)
#endif
{
    pq660 q660 ;
    fd_set readfds, writefds, exceptfds ;
    struct timeval timeout ;
    int res ;
    double now_, diff ;
    I32 new_ten_sec ;
#ifndef X86_WIN32
    int err ;
#endif

#ifndef X86_WIN32
    pthread_detach (pthread_self ()) ;
#endif
    q660 = p ;

    do {
        switch (q660->libstate) {
        case LIBSTATE_CONN :
        case LIBSTATE_REQ :
        case LIBSTATE_RESP :
        case LIBSTATE_XML :
        case LIBSTATE_CFG :
        case LIBSTATE_RUNWAIT :
        case LIBSTATE_RUN :
            FD_ZERO (&(readfds)) ;
            FD_ZERO (&(writefds)) ;
            FD_ZERO (&(exceptfds)) ;

            if (q660->cpath != INVALID_SOCKET) {
                if (q660->libstate == LIBSTATE_CONN) {
                    /* waiting for connection */
                    FD_SET (q660->cpath, &(writefds)) ;
                    FD_SET (q660->cpath, &(exceptfds)) ;
                } else if (q660->share.freeze_timer <= 0)
                    FD_SET (q660->cpath, &(readfds)) ; /* Read data if not frozen */

                timeout.tv_sec = 0 ;
                timeout.tv_usec = 25000 ; /* 25ms timeout */
#ifdef X86_WIN32
                res = select (0, &(readfds), &(writefds), &(exceptfds), &(timeout)) ;
#else
                res = select (q660->high_socket + 1, &(readfds), &(writefds), &(exceptfds), &(timeout)) ;
#endif
#ifdef LINUXDEBUGPRINT
                {
                    static int S=0;
                    fprintf(stderr,"S%d-%d\n",S++,res);
                    fflush(stderr);
                }
#endif

                if ((q660->libstate != LIBSTATE_IDLE) && (res > 0)) {
                    if (q660->libstate == LIBSTATE_CONN) {
                        if (FD_ISSET (q660->cpath, &(exceptfds)))
                            tcp_error (q660, "Connection Refused") ;
                        else if ((q660->cpath != INVALID_SOCKET) && (FD_ISSET (q660->cpath, &(writefds)))) {
                            q660->tcpidx = 0 ;
                            libmsgadd(q660, LIBMSG_CONN, "") ;
                            lib_continue_registration (q660) ;
                        }
                    } else {
                        if ((q660->cpath != INVALID_SOCKET) && (FD_ISSET (q660->cpath, &(readfds))))
#ifdef LINUXDEBUGPRINT
                        {
                            static int C=0;
                            fprintf(stderr,"C%d\n",C++);
                            fflush(stderr);
#endif
                            read_cmd_socket (q660) ;
#ifdef LINUXDEBUGPRINT
                        }

#endif
                    }
                }

#ifndef X86_WIN32
                else if (res < 0) {
                    err = errno ; /* For debugging */
                }

#endif
            }

            break ;

        case LIBSTATE_TERM :
            break ; /* nothing to */

        case LIBSTATE_IDLE :
            if (q660->needtosayhello) {
                q660->needtosayhello = FALSE ;
                libmsgadd (q660, LIBMSG_CREATED, "") ;
            } else
                sleepms (25) ;

            break ;

        default :
            sleepms (25) ;
        }

        if (! q660->terminate) {
#ifdef LINUXDEBUGPRINT
            {
                static int T=0;
                fprintf(stderr,"T%d\n",T++);
                fflush(stderr);
            }
#endif
            now_ = now() ;
            diff = now_ - q660->last_100ms ;

            if (fabs(diff) > (MS100 * 20)) /* greater than 2 second spread */
                q660->last_100ms = now_ + MS100 ; /* clock changed, reset interval */
            else if (diff >= MS100) {
                q660->last_100ms = q660->last_100ms + MS100 ;
                lib_timer (q660) ;
#ifdef LINUXDEBUGPRINT
                {
                    static int I=0;
                    fprintf(stderr,"I%d\n",I++);
                    fflush(stderr);
                }
#endif
            }

            if (! q660->terminate) { /* lib_timer may set terminate */
                new_ten_sec = lib_round(now_) ; /* rounded second */
                new_ten_sec = new_ten_sec / 10 ; /* integer 10 second value */

                if ((new_ten_sec > q660->last_ten_sec) ||
                        (new_ten_sec <= (q660->last_ten_sec - 12)))

                {
                    q660->last_ten_sec = new_ten_sec ;
                    q660->dpstat_timestamp = new_ten_sec * 10 ; /* into seconds since 2000 */
                    lib_stats_timer (q660) ;
                }
            }
        }
    } while (! q660->terminate) ;

    new_state (q660, LIBSTATE_TERM) ;
#ifdef X86_WIN32
    ExitThread (0) ;
    return 0 ;
#else
    pthread_exit (0) ;
#endif
}

void lib_create_660 (tcontext *ct, tpar_create *cfg)
{
    pq660 q660 ;
    int n, msgcnt ;
    pmsgqueue msgq ;
#ifndef X86_WIN32
    int err ;
    pthread_attr_t attr;
#endif

    *ct = malloc(sizeof(tq660)) ;
    q660 = *ct ;
    memset (q660, 0, sizeof(tq660)) ;
    create_mutex (q660) ;
    q660->libstate = LIBSTATE_IDLE ;
    q660->share.target_state = LIBSTATE_IDLE ;
    memcpy (&(q660->par_create), cfg, sizeof(tpar_create)) ;
    q660->share.opstat.status_latency = INVALID_LATENCY ;
    q660->share.opstat.data_latency = INVALID_LATENCY ;
    q660->share.opstat.gps_age = -1 ;
    initialize_memory_utils (&(q660->connmem)) ;
    initialize_memory_utils (&(q660->thrmem)) ;
    q660->last_100ms = now () ;
    q660->last_ten_sec = (q660->last_100ms + 0.5) / 10 ; /* integer 10 second value */
    q660->data_timeout = DEFAULT_DATA_TIMEOUT ;
    q660->data_timeout_retry = DEFAULT_DATA_TIMEOUT_RETRY ;
    q660->status_timeout = DEFAULT_STATUS_TIMEOUT ;
    q660->status_timeout_retry = DEFAULT_STATUS_TIMEOUT_RETRY ;
    q660->piu_retry = DEFAULT_PIU_RETRY ;
    getbuf (&(q660->thrmem), (pvoid)&(q660->cbuf), sizeof(tcbuf)) ;
    getbuf (&(q660->thrmem), (pvoid)&(msgq), sizeof(tmsgqueue)) ;
    q660->msgqueue = msgq ;
    q660->msgq_in = msgq ;
    q660->msgq_out = msgq ;
    msgcnt = q660->par_create.opt_client_msgs ;

    if (msgcnt < MIN_MSG_QUEUE_SIZE)
        msgcnt = MIN_MSG_QUEUE_SIZE ;

    for (n = 2 ; n <= msgcnt ; n++) {
        getbuf (&(q660->thrmem), (pvoid)&(msgq->link), sizeof(tmsgqueue)) ;
        msgq = msgq->link ;
    }

    msgq->link = q660->msgqueue ;
    n = q660->par_create.amini_exponent ;

    if (n >= 9) {
        q660->arc_size = 512 ;

        while (n > 9) {
            q660->arc_size = q660->arc_size << 1 ;
            (n)-- ;
        }
    } else
        q660->arc_size = 0 ; /* defeat */

    q660->arc_frames = q660->arc_size / FRAME_SIZE ;
    q660->cur_verbosity = q660->par_create.opt_verbose ; /* until register anyway */
    restore_thread_continuity (q660, TRUE, q660->contmsg) ;
    load_firfilters (q660) ; /* build standards */
    append_firfilters (q660, q660->par_create.mini_firchain) ; /* add client defined */
    q660->cpath = INVALID_SOCKET ;
    q660->rsum = lib_round (now ()) ;
#ifdef X86_WIN32
    q660->threadhandle = CreateThread (NIL, 0, libthread, q660, 0, (pvoid)&(q660->threadid)) ;

    if (q660->threadhandle == NIL)
#else
    err = pthread_attr_init (&(attr)) ;

    if (err == 0)
        err = pthread_attr_setdetachstate (&(attr), PTHREAD_CREATE_DETACHED) ;

    if (err == 0)
        err = pthread_create (&(q660->threadid), &(attr), libthread, q660) ;

    if (err)
#endif

    {
        cfg->resp_err = LIBERR_THREADERR ;
        free (*ct) ; /* no context */
        *ct = NIL ;
    } else
        q660->needtosayhello = TRUE ;
}

enum tliberr lib_destroy_660 (tcontext *ct)
{
    pq660 q660 ;
    pmem_manager pm, pmn ;

    q660 = *ct ;
    *ct = NIL ;
    destroy_mutex (q660) ;
    pm = q660->connmem.memory_head ;

    while (pm) {
        pmn = pm->next ;
        free (pm->base) ;
        free (pm) ;
        pm = pmn ;
    }

    pm = q660->thrmem.memory_head ;

    while (pm) {
        pmn = pm->next ;
        free (pm->base) ;
        free (pm) ;
        pm = pmn ;
    }

    free (q660) ;
    return LIBERR_NOERR ;
}

enum tliberr lib_register_660 (pq660 q660, tpar_register *rpar)
{

    memclr (&(q660->first_clear), (int)((PNTRINT)&(q660->last_clear) - (PNTRINT)&(q660->first_clear))) ;
    memclr (&(q660->share.first_share_clear),
            (int)((PNTRINT)&(q660->share.last_share_clear) - (PNTRINT)&(q660->share.first_share_clear))) ;
    initialize_memory_utils (&(q660->connmem)) ;
    memcpy (&(q660->par_register), rpar, sizeof(tpar_register)) ;
    q660->share.opstat.gps_age = -1 ;
    memcpy (&(q660->station_ident), &(q660->par_create.q660id_station), sizeof(string9)) ; /* initial station name */
    q660->share.status_interval = 10 ;
    q660->reg_tries = 0 ;
    q660->dynip_age = 0 ;

    if ((q660->par_register.opt_dynamic_ip) && ((q660->par_register.q660id_address)[0] == 0))
        q660->share.target_state = LIBSTATE_WAIT ;
    else
        q660->share.target_state = LIBSTATE_RUNWAIT ;

    return LIBERR_NOERR ;
}

void set_liberr (pq660 q660, enum tliberr newerr)
{

    lock (q660) ;
    q660->share.liberr = newerr ;
    unlock (q660) ;
}
