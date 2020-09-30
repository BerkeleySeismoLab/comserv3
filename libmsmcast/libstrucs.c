/*  Libmsmcast internal core routines

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

#include "libclient.h"
#include "libsampglob.h"
#include "libstrucs.h"
#include "libmsgs.h"
#include "libtypes.h"
#include "platform.h"
#include "libsupport.h"

#define MS100 (0.1)

#ifdef DODBPRINT
#define DBPRINT(A) A
#else
#define DBPRINT(A) 
#endif

static void create_mutex (pmsmcast msmcast)
{
    pthread_mutex_init (addr(msmcast->mutex), NULL) ;
    pthread_mutex_init (addr(msmcast->msgmutex), NULL) ;
}

static void destroy_mutex (pmsmcast msmcast)
{
    pthread_mutex_destroy (addr(msmcast->mutex)) ;
    pthread_mutex_destroy (addr(msmcast->msgmutex)) ;
}

void lock (pmsmcast msmcast)
{
    pthread_mutex_lock (addr(msmcast->mutex)) ;
}

void unlock (pmsmcast msmcast)
{
    pthread_mutex_unlock (addr(msmcast->mutex)) ;
}

void msglock (pmsmcast msmcast)
{
    pthread_mutex_lock (addr(msmcast->msgmutex)) ;
}

void msgunlock (pmsmcast msmcast)
{
    pthread_mutex_unlock (addr(msmcast->msgmutex)) ;
}

void sleepms (integer ms)
{
    struct timespec dly ;

    dly.tv_sec = 0 ;
    dly.tv_nsec = ms * 1000000 ; /* convert to nanoseconds */
    nanosleep (addr(dly), NULL) ;
}


void state_callback (pmsmcast msmcast, enum tstate_type stype, longword val)
{
    if (msmcast->par_create.call_state)
    {
	msmcast->state_call.context = msmcast ;
	msmcast->state_call.state_type = stype ;
	memcpy(addr(msmcast->state_call.station_name), addr(msmcast->station_ident), sizeof(msmcast->station_ident)) ;
	msmcast->state_call.info = val ;
	msmcast->state_call.subtype = 0 ;
	msmcast->par_create.call_state (addr(msmcast->state_call)) ;
    }
}

void new_state (pmsmcast msmcast, enum tlibstate newstate)
{
    msmcast->libstate = newstate ;
    state_callback (msmcast, ST_STATE, (longword)newstate) ;
}

longword make_bitmap (longword bit)
{
    return (longword)1 shl bit ;
}

void *libthread (pointer p)
{
    pmsmcast msmcast ;
    fd_set readfds, writefds, exceptfds ;
    struct timeval timeout ;
    integer res ;
    double now_, diff ;
    longint new_ten_sec ;
    integer err ;

    pthread_detach (pthread_self ()) ;
    msmcast = p ;
    do {
        switch (msmcast->libstate) {
	case LIBSTATE_CONN :
	case LIBSTATE_REQ :
	case LIBSTATE_RESP :
	case LIBSTATE_XML :
	case LIBSTATE_CFG :
	case LIBSTATE_RUNWAIT :
	case LIBSTATE_RUN :
	    FD_ZERO (addr(readfds)) ;
	    FD_ZERO (addr(writefds)) ;
	    FD_ZERO (addr(exceptfds)) ;
	    if (msmcast->cpath != INVALID_SOCKET)
            {
		if (msmcast->libstate == LIBSTATE_CONN)
		{
		    FD_SET (msmcast->cpath, addr(exceptfds)) ;
		}
		else 
		{
		    FD_SET (msmcast->cpath, addr(readfds)) ;
		}
		FD_SET (msmcast->cpath, addr(readfds)) ;
		timeout.tv_sec = 0 ;
		timeout.tv_usec = 25000 ; /* 25ms timeout */
		res = select (msmcast->high_socket + 1, addr(readfds), addr(writefds), addr(exceptfds), addr(timeout)) ;
		DBPRINT(printf ("DEBUG:: select: cpath=%d high_socket=%d rc=%d libstate=%d\n", msmcast->cpath, msmcast->high_socket, res, msmcast->libstate);)
		if ((msmcast->libstate != LIBSTATE_IDLE) && (res > 0))
		{
                    if (msmcast->libstate == LIBSTATE_CONN)
		    {
			if (FD_ISSET (msmcast->cpath, addr(exceptfds)))
			    tcp_error (msmcast, "Connection Refused") ;
			else if ((msmcast->cpath != INVALID_SOCKET) && (FD_ISSET (msmcast->cpath, addr(writefds))))
			{
			    msmcast->tcpidx = 0 ;
			    libmsgadd(msmcast, LIBMSG_CONN, "") ;
			    lib_continue_registration (msmcast) ;
			}
		    }
		    else
		    {
			if ((msmcast->cpath != INVALID_SOCKET) && (FD_ISSET (msmcast->cpath, addr(readfds))))
			    DBPRINT(printf ("DEBUG:: Calling read_mcast_socket\n");)
			    read_mcast_socket (msmcast) ;
		    }
		}
		else if (res < 0)
		{
                    err = errno ; /* For debugging */
		}
	    }
	    break ;
	case LIBSTATE_TERM :
	    break ; /* nothing to */
	case LIBSTATE_IDLE :
	    DBPRINT(printf ("DEBUG:: libthread LIBSTATE_IDLE sleep: 25 ms\n");)
	    sleepms (25) ;
	    break ;
	default :
	    DBPRINT(printf ("DEBUG:: libthread default sleep: 25 ms\n");)
	    sleepms (25) ;
	}

	if (! msmcast->terminate)
	{
	    now_ = now() ;
	    diff = now_ - msmcast->last_100ms ;
	    if (fabs(diff) > (MS100 * 20)) /* greater than 2 second spread */
		msmcast->last_100ms = now_ + MS100 ; /* clock changed, reset interval */
	    else if (diff >= MS100)
	    {
                msmcast->last_100ms = msmcast->last_100ms + MS100 ;
		lib_timer (msmcast) ;
	    }
	    if (! msmcast->terminate) /* lib_timer may set terminate */
	    {
                new_ten_sec = lib_round(now_) ; /* rounded second */
		new_ten_sec = new_ten_sec / 10 ; /* integer 10 second value */
		if ((new_ten_sec > msmcast->last_ten_sec) ||
		    (new_ten_sec <= (msmcast->last_ten_sec - 12)))
		{
		    msmcast->last_ten_sec = new_ten_sec ;
		    msmcast->dpstat_timestamp = new_ten_sec * 10 ; /* into seconds since 2000 */
		    lib_stats_timer (msmcast) ;
		}
	    }
	}
    } while (! msmcast->terminate) ;
    new_state (msmcast, LIBSTATE_TERM) ;
    pthread_exit (0) ;
}

void lib_create_msmcast (tcontext *ct, tpar_create *cfg)
{
    pmsmcast msmcast ;
    integer n, msgcnt ;
    pmsgqueue msgq ;
    integer err ;
    pthread_attr_t attr;

    *ct = malloc(sizeof(tmsmcast)) ;
    msmcast = *ct ;
    memset (msmcast, 0, sizeof(tmsmcast)) ;
    create_mutex (msmcast) ;
    msmcast->libstate = LIBSTATE_IDLE ;
    msmcast->share.target_state = LIBSTATE_IDLE ;
    memcpy (addr(msmcast->par_create), cfg, sizeof(tpar_create)) ;
    msmcast->share.opstat.status_latency = INVALID_LATENCY ;
    msmcast->share.opstat.data_latency = INVALID_LATENCY ;    msmcast->last_100ms = now () ;
    initialize_memory_utils (addr(msmcast->thrmem)) ;
    msmcast->last_ten_sec = (msmcast->last_100ms + 0.5) / 10 ; /* integer 10 second value */
    msmcast->data_timeout = DEFAULT_DATA_TIMEOUT ;
    msmcast->data_timeout_retry = DEFAULT_DATA_TIMEOUT_RETRY ;
    msmcast->status_timeout = DEFAULT_STATUS_TIMEOUT ;
    msmcast->status_timeout_retry = DEFAULT_STATUS_TIMEOUT_RETRY ;
    msmcast->piu_retry = DEFAULT_PIU_RETRY ;
    getbuf (addr(msmcast->thrmem), (pvoid)addr(msgq), sizeof(tmsgqueue)) ;
    msmcast->msgqueue = msgq ;
    msmcast->msgq_in = msgq ;
    msmcast->msgq_out = msgq ;
    msgcnt = msmcast->par_create.opt_client_msgs ;
    if (msgcnt < MIN_MSG_QUEUE_SIZE)
	msgcnt = MIN_MSG_QUEUE_SIZE ;
    for (n = 2 ; n <= msgcnt ; n++)
    {
	getbuf (addr(msmcast->thrmem), (pvoid)addr(msgq->link), sizeof(tmsgqueue)) ;
	msgq = msgq->link ;
    }
    msgq->link = msmcast->msgqueue ;
    msmcast->cur_verbosity = msmcast->par_create.opt_verbose ; /* until register anyway */
    msmcast->cpath = INVALID_SOCKET ;
    err = pthread_attr_init (addr(attr)) ;
    if (err == 0)
	err = pthread_attr_setdetachstate (addr(attr), PTHREAD_CREATE_DETACHED) ;
    if (err == 0)
	err = pthread_create (addr(msmcast->threadid), addr(attr), libthread, msmcast) ;
    if (err)
    {
        cfg->resp_err = LIBERR_THREADERR ;
        free (*ct) ; /* no context */
        *ct = NIL ;
    }
}

enum tliberr lib_destroy_msmcast (tcontext *ct)
{
    pmsmcast msmcast ;

    msmcast = *ct ;
    *ct = NIL ;
    destroy_mutex (msmcast) ;
    free (msmcast) ;
    return LIBERR_NOERR ;
}

enum tliberr lib_register_msmcast (pmsmcast msmcast, tpar_register *rpar)
{
    memclr (addr(msmcast->first_clear), (integer)((pntrint)addr(msmcast->last_clear) - (pntrint)addr(msmcast->first_clear))) ;
    memclr (addr(msmcast->share.first_share_clear),
	    (integer)((pntrint)addr(msmcast->share.last_share_clear) - (pntrint)addr(msmcast->share.first_share_clear))) ;
    memcpy (addr(msmcast->par_register), rpar, sizeof(tpar_register)) ;
    memcpy (addr(msmcast->station_ident), addr(msmcast->par_create.msmcastid_station), sizeof(msmcast->station_ident)) ; /* initial station name */
    msmcast->share.target_state = LIBSTATE_RUNWAIT ;
    return LIBERR_NOERR ;
}

void set_liberr (pmsmcast msmcast, enum tliberr newerr)
{
    lock (msmcast) ;
    msmcast->share.liberr = newerr ;
    unlock (msmcast) ;
}
