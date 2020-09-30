/*   Lib660 internal core routines
     Copyright 2017-2019 Certified Software Corporation

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
*/
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
begin

  q660->mutex = CreateMutex(NIL, FALSE, NIL) ;
  q660->msgmutex = CreateMutex(NIL, FALSE, NIL) ;
end

static void destroy_mutex (pq660 q660)
begin

  CloseHandle (q660->mutex) ;
  CloseHandle (q660->msgmutex) ;
end

void lock (pq660 q660)
begin

  WaitForSingleObject (q660->mutex, INFINITE) ;
end

void unlock (pq660 q660)
begin

  ReleaseMutex (q660->mutex) ;
end

void msglock (pq660 q660)
begin

  WaitForSingleObject (q660->msgmutex, INFINITE) ;
end

void msgunlock (pq660 q660)
begin

  ReleaseMutex (q660->msgmutex) ;
end

void sleepms (integer ms)
begin

  Sleep (ms) ;
end

#else

static void create_mutex (pq660 q660)
begin

  pthread_mutex_init (addr(q660->mutex), NULL) ;
  pthread_mutex_init (addr(q660->msgmutex), NULL) ;
end

static void destroy_mutex (pq660 q660)
begin

  pthread_mutex_destroy (addr(q660->mutex)) ;
  pthread_mutex_destroy (addr(q660->msgmutex)) ;
end

void lock (pq660 q660)
begin

  pthread_mutex_lock (addr(q660->mutex)) ;
end

void unlock (pq660 q660)
begin

  pthread_mutex_unlock (addr(q660->mutex)) ;
end

void msglock (pq660 q660)
begin

  pthread_mutex_lock (addr(q660->msgmutex)) ;
end

void msgunlock (pq660 q660)
begin

  pthread_mutex_unlock (addr(q660->msgmutex)) ;
end

void sleepms (integer ms)
begin
  struct timespec dly ;

  dly.tv_sec = 0 ;
  dly.tv_nsec = ms * 1000000 ; /* convert to nanoseconds */
  nanosleep (addr(dly), NULL) ;
end
#endif

void state_callback (pq660 q660, enum tstate_type stype, longword val)
begin

  if (q660->par_create.call_state)
    then
      begin
        q660->state_call.context = q660 ;
        q660->state_call.state_type = stype ;
        memcpy(addr(q660->state_call.station_name), addr(q660->station_ident), sizeof(string9)) ;
        q660->state_call.info = val ;
        q660->state_call.subtype = 0 ;
        q660->par_create.call_state (addr(q660->state_call)) ;
      end
end

void new_state (pq660 q660, enum tlibstate newstate)
begin

  q660->libstate = newstate ;
  state_callback (q660, ST_STATE, (longword)newstate) ;
end

longword make_bitmap (longword bit)
begin

  return (longword)1 shl bit ;
end

void new_cfg (pq660 q660, longword newbitmap)
begin

  lock (q660) ;
  if (q660->share.have_config != (q660->share.have_config or newbitmap))
    then
      begin /* something we didn't have before */
        q660->share.have_config = q660->share.have_config or newbitmap ;
        unlock (q660) ;
        state_callback (q660, ST_CFG, q660->share.have_config) ;
      end
    else
      unlock (q660) ;
end

void new_status (pq660 q660, longword newbitmap)
begin

  lock (q660) ;
  q660->share.have_status = q660->share.have_status or newbitmap ;
  unlock (q660) ;
  state_callback (q660, ST_STATUS, newbitmap) ;
end

#ifdef X86_WIN32
unsigned long  __stdcall libthread (pointer p)
#else
void *libthread (pointer p)
#endif
begin
  pq660 q660 ;
  fd_set readfds, writefds, exceptfds ;
  struct timeval timeout ;
  integer res ;
  double now_, diff ;
  longint new_ten_sec ;
#ifndef X86_WIN32
  integer err ;
#endif

#ifndef X86_WIN32
  pthread_detach (pthread_self ()) ;
#endif
  q660 = p ;
  repeat
    switch (q660->libstate) begin
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
        if (q660->cpath != INVALID_SOCKET)
          then
            begin
              if (q660->libstate == LIBSTATE_CONN)
                then
                  begin /* waiting for connection */
                    FD_SET (q660->cpath, addr(writefds)) ;
                    FD_SET (q660->cpath, addr(exceptfds)) ;
                  end
              else if (q660->share.freeze_timer <= 0)
                then
                  FD_SET (q660->cpath, addr(readfds)) ; /* Read data if not frozen */
              timeout.tv_sec = 0 ;
              timeout.tv_usec = 25000 ; /* 25ms timeout */
#ifdef X86_WIN32
              res = select (0, addr(readfds), addr(writefds), addr(exceptfds), addr(timeout)) ;
#else
              res = select (q660->high_socket + 1, addr(readfds), addr(writefds), addr(exceptfds), addr(timeout)) ;
#endif
              if ((q660->libstate != LIBSTATE_IDLE) land (res > 0))
                then
                  begin
                    if (q660->libstate == LIBSTATE_CONN)
                      then
                        begin
                          if (FD_ISSET (q660->cpath, addr(exceptfds)))
                              then
                                tcp_error (q660, "Connection Refused") ;
                          else if ((q660->cpath != INVALID_SOCKET) land (FD_ISSET (q660->cpath, addr(writefds))))
                            then
                              begin
                                q660->tcpidx = 0 ;
                                libmsgadd(q660, LIBMSG_CONN, "") ;
                                lib_continue_registration (q660) ;
                              end
                        end
                      else
                        begin
                          if ((q660->cpath != INVALID_SOCKET) land (FD_ISSET (q660->cpath, addr(readfds))))
                            then
                              read_cmd_socket (q660) ;
                        end
                  end
#ifndef X86_WIN32
              else if (res < 0)
                then
                  begin
                    err = errno ; /* For debugging */
                  end
#endif
            end
        break ;
      case LIBSTATE_TERM :
        break ; /* nothing to */
      case LIBSTATE_IDLE :
        if (q660->needtosayhello)
          then
            begin
              q660->needtosayhello = FALSE ;
              libmsgadd (q660, LIBMSG_CREATED, "") ;
            end
          else
            sleepms (25) ;
        break ;
      default :
        sleepms (25) ;
    end
    if (lnot q660->terminate)
      then
        begin
          now_ = now() ;
          diff = now_ - q660->last_100ms ;
          if (fabs(diff) > (MS100 * 20)) /* greater than 2 second spread */
            then
              q660->last_100ms = now_ + MS100 ; /* clock changed, reset interval */
          else if (diff >= MS100)
            then
              begin
                q660->last_100ms = q660->last_100ms + MS100 ;
                lib_timer (q660) ;
              end
          if (lnot q660->terminate) /* lib_timer may set terminate */
            then
              begin
                new_ten_sec = lib_round(now_) ; /* rounded second */
                new_ten_sec = new_ten_sec div 10 ; /* integer 10 second value */
                if ((new_ten_sec > q660->last_ten_sec) lor
                    (new_ten_sec <= (q660->last_ten_sec - 12)))
                  then
                    begin
                      q660->last_ten_sec = new_ten_sec ;
                      q660->dpstat_timestamp = new_ten_sec * 10 ; /* into seconds since 2000 */
                      lib_stats_timer (q660) ;
                    end
              end
        end
  until q660->terminate) ;
  new_state (q660, LIBSTATE_TERM) ;
#ifdef X86_WIN32
  ExitThread (0) ;
  return 0 ;
#else
  pthread_exit (0) ;
#endif
end

void lib_create_660 (tcontext *ct, tpar_create *cfg)
begin
  pq660 q660 ;
  integer n, msgcnt ;
  pmsgqueue msgq ;
#ifndef X86_WIN32
  integer err ;
  pthread_attr_t attr;
#endif

  *ct = malloc(sizeof(tq660)) ;
  q660 = *ct ;
  memset (q660, 0, sizeof(tq660)) ;
  create_mutex (q660) ;
  q660->libstate = LIBSTATE_IDLE ;
  q660->share.target_state = LIBSTATE_IDLE ;
  memcpy (addr(q660->par_create), cfg, sizeof(tpar_create)) ;
  q660->share.opstat.status_latency = INVALID_LATENCY ;
  q660->share.opstat.data_latency = INVALID_LATENCY ;
  q660->share.opstat.gps_age = -1 ;
  initialize_memory_utils (addr(q660->connmem)) ;
  initialize_memory_utils (addr(q660->thrmem)) ;
  q660->last_100ms = now () ;
  q660->last_ten_sec = (q660->last_100ms + 0.5) div 10 ; /* integer 10 second value */
  q660->data_timeout = DEFAULT_DATA_TIMEOUT ;
  q660->data_timeout_retry = DEFAULT_DATA_TIMEOUT_RETRY ;
  q660->status_timeout = DEFAULT_STATUS_TIMEOUT ;
  q660->status_timeout_retry = DEFAULT_STATUS_TIMEOUT_RETRY ;
  q660->piu_retry = DEFAULT_PIU_RETRY ;
  getbuf (addr(q660->thrmem), (pvoid)addr(q660->cbuf), sizeof(tcbuf)) ;
  getbuf (addr(q660->thrmem), (pvoid)addr(msgq), sizeof(tmsgqueue)) ;
  q660->msgqueue = msgq ;
  q660->msgq_in = msgq ;
  q660->msgq_out = msgq ;
  msgcnt = q660->par_create.opt_client_msgs ;
  if (msgcnt < MIN_MSG_QUEUE_SIZE)
    then
      msgcnt = MIN_MSG_QUEUE_SIZE ;
  for (n = 2 ; n <= msgcnt ; n++)
    begin
      getbuf (addr(q660->thrmem), (pvoid)addr(msgq->link), sizeof(tmsgqueue)) ;
      msgq = msgq->link ;
    end
  msgq->link = q660->msgqueue ;
  n = q660->par_create.amini_exponent ;
  if (n >= 9)
    then
      begin
        q660->arc_size = 512 ;
        while (n > 9)
          begin
            q660->arc_size = q660->arc_size shl 1 ;
            dec(n) ;
          end
      end
    else
      q660->arc_size = 0 ; /* defeat */
  q660->arc_frames = q660->arc_size div FRAME_SIZE ;
  q660->cur_verbosity = q660->par_create.opt_verbose ; /* until register anyway */
  restore_thread_continuity (q660, TRUE, q660->contmsg) ;
  load_firfilters (q660) ; /* build standards */
  append_firfilters (q660, q660->par_create.mini_firchain) ; /* add client defined */
  q660->cpath = INVALID_SOCKET ;
  q660->rsum = lib_round (now ()) ;
#ifdef X86_WIN32
  q660->threadhandle = CreateThread (NIL, 0, libthread, q660, 0, (pvoid)addr(q660->threadid)) ;
  if (q660->threadhandle == NIL)
#else
  err = pthread_attr_init (addr(attr)) ;
  if (err == 0)
    then
      err = pthread_attr_setdetachstate (addr(attr), PTHREAD_CREATE_DETACHED) ;
  if (err == 0)
    then
      err = pthread_create (addr(q660->threadid), addr(attr), libthread, q660) ;
  if (err)
#endif
    then
      begin
        cfg->resp_err = LIBERR_THREADERR ;
        free (*ct) ; /* no context */
        *ct = NIL ;
      end
    else
      q660->needtosayhello = TRUE ;
end

enum tliberr lib_destroy_660 (tcontext *ct)
begin
  pq660 q660 ;
  pmem_manager pm, pmn ;

  q660 = *ct ;
  *ct = NIL ;
  destroy_mutex (q660) ;
  pm = q660->connmem.memory_head ;
  while (pm)
    begin
      pmn = pm->next ;
      free (pm->base) ;
      free (pm) ;
      pm = pmn ;
    end
  pm = q660->thrmem.memory_head ;
  while (pm)
    begin
      pmn = pm->next ;
      free (pm->base) ;
      free (pm) ;
      pm = pmn ;
    end
  free (q660) ;
  return LIBERR_NOERR ;
end

enum tliberr lib_register_660 (pq660 q660, tpar_register *rpar)
begin

  memclr (addr(q660->first_clear), (integer)((pntrint)addr(q660->last_clear) - (pntrint)addr(q660->first_clear))) ;
  memclr (addr(q660->share.first_share_clear),
       (integer)((pntrint)addr(q660->share.last_share_clear) - (pntrint)addr(q660->share.first_share_clear))) ;
  initialize_memory_utils (addr(q660->connmem)) ;
  memcpy (addr(q660->par_register), rpar, sizeof(tpar_register)) ;
  q660->share.opstat.gps_age = -1 ;
  memcpy (addr(q660->station_ident), addr(q660->par_create.q660id_station), sizeof(string9)) ; /* initial station name */
  q660->share.status_interval = 10 ;
  q660->reg_tries = 0 ;
  q660->dynip_age = 0 ;
  if ((q660->par_register.opt_dynamic_ip) land ((q660->par_register.q660id_address)[0] == 0))
    then
      q660->share.target_state = LIBSTATE_WAIT ;
    else
      q660->share.target_state = LIBSTATE_RUNWAIT ;
  return LIBERR_NOERR ;
end

void set_liberr (pq660 q660, enum tliberr newerr)
begin

  lock (q660) ;
  q660->share.liberr = newerr ;
  unlock (q660) ;
end
