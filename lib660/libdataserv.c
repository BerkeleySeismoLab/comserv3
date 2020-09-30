/*   Lib660 Data Server Routines
     Copyright 2017 Certified Software Corporation

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
    0 2017-06-08 rdr Created
    1 2017-07-17 rdr Add reporting client IP address and port.
    2 2019-09-25 rdr Fix send_next_buffer so it doesn't keep sending the same
                     record over and over again.
    3 2019-09-26 rdr Various fixes to get low-latency callback to work.
*/
#include "libdataserv.h"
#include "libmsgs.h"
#include "libclient.h"
#include "libsupport.h"
#include "libstrucs.h"
#include "libcompress.h"

typedef struct {
#ifdef X86_WIN32
  HANDLE mutex ;
  HANDLE threadhandle ;
  longword threadid ;
#else
  pthread_mutex_t mutex ;
  pthread_t threadid ;
#endif
  boolean running ;
  boolean haveclient ; /* accepted a client */
  boolean sockopen ;
  boolean sockfull ; /* last send failed */
  boolean terminate ;
  tds_par ds_par ; /* creation parameters */
#ifdef X86_WIN32
  SOCKET dpath ; /* dataserv socket */
  struct sockaddr dsockin ; /* dataserv address descriptors */
  SOCKET sockpath ;
#else
  integer dpath ; /* commands socket */
  struct sockaddr dsockin ; /* dataserv address descriptors */
  integer sockpath ;
  integer high_socket ;
#endif
  integer dsq_in, dsq_out ;
  double last_sent ;
} tdsstr ;
typedef tdsstr *pdsstr ;

#ifdef X86_WIN32
static void create_mutex (pdsstr dsstr)
begin

  dsstr->mutex = CreateMutex(NIL, FALSE, NIL) ;
end

static void destroy_mutex (pdsstr dsstr)
begin

  CloseHandle (dsstr->mutex) ;
end

static void qlock (pdsstr dsstr)
begin

  WaitForSingleObject (dsstr->mutex, INFINITE) ;
end

static void qunlock (pdsstr dsstr)
begin

  ReleaseMutex (dsstr->mutex) ;
end

#else

static void create_mutex (pdsstr dsstr)
begin

  pthread_mutex_init (addr(dsstr->mutex), NULL) ;
end

static void destroy_mutex (pdsstr dsstr)
begin

  pthread_mutex_destroy (addr(dsstr->mutex)) ;
end

static void qlock (pdsstr dsstr)
begin

  pthread_mutex_lock (addr(dsstr->mutex)) ;
end

static void qunlock (pdsstr dsstr)
begin

  pthread_mutex_unlock (addr(dsstr->mutex)) ;
end

#endif

static void close_socket (pdsstr dsstr)
begin

  dsstr->sockopen = FALSE ;
  if (dsstr->dpath != INVALID_SOCKET)
    then
      begin
#ifdef X86_WIN32
        closesocket (dsstr->dpath) ;
#else
        close (dsstr->dpath) ;
#endif
        dsstr->dpath = INVALID_SOCKET ;
      end
  if (dsstr->sockpath != INVALID_SOCKET)
    then
      begin
#ifdef X86_WIN32
        closesocket (dsstr->sockpath) ;
#else
        close (dsstr->sockpath) ;
#endif
        dsstr->sockpath = INVALID_SOCKET ;
      end
#ifndef X86_WIN32
  dsstr->high_socket = 0 ;
#endif
end

static void open_socket (pdsstr dsstr)
begin
  integer lth, j ;
  integer err ;
  integer flag ;
  word host_port ;
  struct sockaddr xyz ;
  struct sockaddr_in *psock ;
  string63 s ;
#ifdef X86_WIN32
  BOOL flag2 ;
#else
  int flag2 ;
#endif
  struct linger lingeropt ;

  close_socket (dsstr) ;
  dsstr->dpath = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
  if (dsstr->dpath == INVALID_SOCKET)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        sprintf(s, "%d on dataserv[%d] port", err, dsstr->ds_par.server_number) ;
        lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_SOCKETERR, 0, s) ;
        return ;
      end
#ifndef X86_WIN32
  if (dsstr->dpath > dsstr->high_socket)
    then
      dsstr->high_socket = dsstr->dpath ;
#endif
  psock = (pointer) addr(dsstr->dsockin) ;
  memset(psock, 0, sizeof(struct sockaddr)) ;
  psock->sin_family = AF_INET ;
  if (dsstr->ds_par.ds_port == PORT_OS)
    then
      psock->sin_port = 0 ;
    else
      psock->sin_port = htons(dsstr->ds_par.ds_port) ;
  psock->sin_addr.s_addr = INADDR_ANY ;
  flag2 = 1 ;
#ifdef X86_WIN32
  j = sizeof(BOOL) ;
  setsockopt (dsstr->dpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)addr(flag2), j) ;
#else
  j = sizeof(int) ;
  setsockopt (dsstr->dpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)addr(flag2), j) ;
#endif
  flag = sizeof(struct linger) ;
#ifdef X86_WIN32
  getsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)addr(lingeropt), (pvoid)addr(flag)) ;
#else
  getsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)addr(lingeropt), (pvoid)addr(flag)) ;
#endif
  if (lingeropt.l_onoff)
    then
      begin
        lingeropt.l_onoff = 0 ;
        lingeropt.l_linger = 0 ;
        flag = sizeof(struct linger) ;
#ifdef X86_WIN32
        setsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)addr(lingeropt), flag) ;
#else
        setsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)addr(lingeropt), flag) ;
#endif
      end
#ifdef X86_WIN32
  err = bind(dsstr->dpath, addr(dsstr->dsockin), sizeof(struct sockaddr)) ;
  if (err)
#else
  err = bind(dsstr->dpath, addr(dsstr->dsockin), sizeof(struct sockaddr)) ;
  if (err)
#endif
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
        closesocket (dsstr->dpath) ;
#else
               errno ;
        close (dsstr->dpath) ;
#endif
        dsstr->dpath = INVALID_SOCKET ;
        sprintf(s, "%d on dataserv[%d] port", err, dsstr->ds_par.server_number) ;
        lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_BINDERR, 0, s) ;
        return ;
      end
  flag = 1 ;
#ifdef X86_WIN32
  ioctlsocket (dsstr->dpath, FIONBIO, (pvoid)addr(flag)) ;
  err = listen (dsstr->dpath, 1) ;
#else
  flag = fcntl (dsstr->dpath, F_GETFL, 0) ;
  fcntl (dsstr->dpath, F_SETFL, flag or O_NONBLOCK) ;
  err = listen (dsstr->dpath, 1) ;
#endif
  if (err)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
        closesocket (dsstr->dpath) ;
#else
               errno ;
        close (dsstr->dpath) ;
#endif
        dsstr->dpath = INVALID_SOCKET ;
        sprintf(s, "%d on dataserv[%d] port", err, dsstr->ds_par.server_number) ;
        lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_LISTENERR, 0, s) ;
        return ;
      end
  lth = sizeof(struct sockaddr) ;
  getsockname (dsstr->dpath, (pvoid)addr(xyz), (pvoid)addr(lth)) ;
  psock = (pointer) addr(xyz) ;
  host_port = ntohs(psock->sin_port) ;
  sprintf(s, "on dataserv[%d] port %d", dsstr->ds_par.server_number, host_port) ;
  lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_SOCKETOPEN, 0, s) ;
  dsstr->sockopen = TRUE ;
end

static void accept_ds_socket (pdsstr dsstr)
begin
  integer lth, err, err2 ;
  integer flag ;
  integer bufsize ;
  word client_port ;
  string63 hostname ;
  string63 s ;
  struct sockaddr_storage tmpsock ;
  struct sockaddr_in *psock ;
#ifndef CBUILDERX
  struct sockaddr_in6 *psock6 ;
  longword lw6[4] ; /* IPV6 represented as 4 32 bit entries */
#endif

  lth = sizeof(struct sockaddr_storage) ;
  if (dsstr->dpath == INVALID_SOCKET)
    then
      return ;
#ifdef X86_WIN32
  dsstr->sockpath = accept (dsstr->dpath, (pvoid)addr(tmpsock), (pvoid)addr(lth)) ;
#else
  dsstr->sockpath = accept (dsstr->dpath, (pvoid)addr(tmpsock), (pvoid)addr(lth)) ;
#endif
  if (dsstr->sockpath == INVALID_SOCKET)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        if ((err != EWOULDBLOCK) land (err != EINPROGRESS))
          then
            begin
#ifdef X86_WIN32
              err2 = closesocket (dsstr->dpath) ;
#else
              err2 = close (dsstr->dpath) ;
#endif
              dsstr->dpath = INVALID_SOCKET ;
              sprintf(s, "%d on dataserv[%d] port", err2, dsstr->ds_par.server_number) ;
              lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_ACCERR, 0, s) ;
            end
      end
    else
      begin
#ifdef X86_WIN32
        flag = 1 ;
        ioctlsocket (dsstr->sockpath, FIONBIO, (pvoid)addr(flag)) ;
#else
        if (dsstr->sockpath > dsstr->high_socket)
          then
            dsstr->high_socket = dsstr->sockpath ;
        flag = fcntl (dsstr->sockpath, F_GETFL, 0) ;
        fcntl (dsstr->sockpath, F_SETFL, flag or O_NONBLOCK) ;
#endif
        if (tmpsock.ss_family == AF_INET)
          then
            begin
              psock = (pointer) addr(tmpsock) ;
              showdot (ntohl(psock->sin_addr.s_addr), hostname) ;
              client_port = ntohs(psock->sin_port) ;
            end
#ifndef CBUILDERX
          else
            begin
              psock6 = (pointer) addr(tmpsock) ;
              memcpy (addr(lw6), addr(psock6->sin6_addr), IN6ADDRSZ) ;
              if ((lw6[0] == 0) land (lw6[1] == 0) land (lw6[2] == 0xFFFF0000))
                then /* Is actually an IPV4 address */
                  showdot (ntohl(lw6[3]), hostname) ;
                else
                  strcpy (hostname, inet_ntop6 ((pvoid)addr(psock6->sin6_addr))) ;
              client_port = ntohs(psock6->sin6_port) ;
            end
#endif
        sprintf(s, "\"%s:%d\" to dataserv[%d] port", hostname, client_port, dsstr->ds_par.server_number) ;
        lib_msg_add (dsstr->ds_par.stnctx, AUXMSG_CONN, 0, s) ;
        lth = sizeof(integer) ;
#ifdef X86_WIN32
        err = getsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)addr(bufsize), (pvoid)addr(lth)) ;
#else
        err = getsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)addr(bufsize), (pvoid)addr(lth)) ;
#endif
        if ((err == 0) land (bufsize < 30000))
          then
            begin
              bufsize = 30000 ;
#ifdef X86_WIN32
              setsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)addr(bufsize), lth) ;
#else
              setsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)addr(bufsize), lth) ;
#endif
            end
#ifndef X86_WIN32
        flag = 1 ;
        lth = sizeof(integer) ;
#if defined(linux) || defined(solaris)
        signal (SIGPIPE, SIG_IGN) ;
#else
        setsockopt (dsstr->sockpath, SOL_SOCKET, SO_NOSIGPIPE, (pvoid)addr(flag), lth) ;
#endif
#endif
        dsstr->haveclient = TRUE ;
        dsstr->last_sent = now () ;
        dsstr->sockfull = FALSE ;
      end
end

static void read_from_client (pdsstr dsstr)
begin
#define RBUFSIZE 100
  integer err ;
  byte buf[RBUFSIZE] ;
  string63 s ;

  repeat
    if (lnot dsstr->haveclient)
      then
        return ;
    err = (integer)recv(dsstr->sockpath, (pvoid)addr(buf), RBUFSIZE, 0) ;
    if (err == SOCKET_ERROR)
      then
        begin
          err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
          if ((err == ECONNRESET) lor (err == ECONNABORTED))
            then
              begin
#ifdef X86_WIN32
                closesocket (dsstr->sockpath) ;
#else
                close (dsstr->sockpath) ;
#endif
                sprintf(s, "dataserv[%d] port", dsstr->ds_par.server_number) ;
                lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_DISCON, 0, s) ;
                dsstr->haveclient = FALSE ;
                dsstr->sockfull = FALSE ;
                return ;
              end
        end
  until (err == 0)) ; /* nothing left in buffer */
end

static integer wrap_buffer (integer max, integer i)
begin
  integer j ;

  j = i + 1 ;
  if (j >= max)
    then
      j = 0 ;
  return j ;
end

static void send_next_buffer (pdsstr dsstr)
begin
  integer err, lth ;
  plong pl ;
  string95 s ;

  if (lnot dsstr->haveclient)
    then
      return ;
  pl = (plong)addr((*(dsstr->ds_par.dsbuf))[dsstr->dsq_out]) ;
  lth = *pl ;
#if defined(linux)
  err = (integer)send(dsstr->sockpath, (pvoid)pl, lth, MSG_NOSIGNAL) ;
#else
  err = (integer)send(dsstr->sockpath, (pvoid)pl, lth, 0) ;
#endif
  if (err == SOCKET_ERROR)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        if (err == EWOULDBLOCK)
          then
            begin
              dsstr->sockfull = TRUE ;
              return ;
            end
#ifdef X86_WIN32
        else if ((err == ECONNRESET) lor (err == ECONNABORTED))
#else
        else if (err == EPIPE)
#endif
          then
            begin
#ifdef X86_WIN32
              closesocket (dsstr->sockpath) ;
#else
              close (dsstr->sockpath) ;
#endif
              sprintf(s, "dataserv[%d] port", dsstr->ds_par.server_number) ;
              lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_DISCON, 0, s) ;
              dsstr->haveclient = FALSE ;
              dsstr->sockfull = FALSE ;
              return ;
            end
      end
    else
      begin
        dsstr->sockfull = FALSE ;
        dsstr->dsq_out = wrap_buffer(dsstr->ds_par.record_count, dsstr->dsq_out) ;
      end
  dsstr->last_sent = now () ;
end

void lib_ds_send (pointer ct, plowlat_call pbuf)
begin
  pdsstr dsstr ;
  integer nq ;
  boolean wasempty ;

  dsstr = ct ;
  qlock (dsstr) ;
  wasempty = (dsstr->dsq_in == dsstr->dsq_out) ;
  nq = wrap_buffer (dsstr->ds_par.record_count, dsstr->dsq_in) ; /* next pointer after we insert new record */
  if (nq == dsstr->dsq_out)
    then
      dsstr->dsq_out = wrap_buffer(dsstr->ds_par.record_count, dsstr->dsq_out) ; /* throw away oldest */
  memcpy(addr((*(dsstr->ds_par.dsbuf))[dsstr->dsq_in]), pbuf, LIB_REC_SIZE) ;
  dsstr->dsq_in = nq ;
  if (wasempty)
    then
      send_next_buffer (dsstr) ;
  qunlock (dsstr) ;
end

#ifdef X86_WIN32
unsigned long  __stdcall dsthread (pointer p)
#else
void *dsthread (pointer p)
#endif
begin
  pdsstr dsstr ;
  fd_set readfds, writefds, exceptfds ;
  struct timeval timeout ;
  integer res ;
  boolean sent ;

#ifndef X86_WIN32
  pthread_detach (pthread_self ()) ;
#endif
  dsstr = p ;
  repeat
    if (dsstr->sockopen)
      then
        begin /* wait for socket input or timeout */
          FD_ZERO (addr(readfds)) ;
          FD_ZERO (addr(writefds)) ;
          FD_ZERO (addr(exceptfds)) ;
          if (lnot dsstr->haveclient)
            then
              FD_SET (dsstr->dpath, addr(readfds)) ; /* waiting for accept */
            else
              begin
                FD_SET (dsstr->sockpath, addr(readfds)) ; /* client might try to send me something */
                if (dsstr->sockfull)
                  then
                    FD_SET (dsstr->sockpath, addr(writefds)) ; /* buffer was full */
              end
          timeout.tv_sec = 0 ;
          timeout.tv_usec = 25000 ; /* 25ms timeout */
#ifdef X86_WIN32
          res = select (0, addr(readfds), addr(writefds), addr(exceptfds), addr(timeout)) ;
#else
          res = select (dsstr->high_socket + 1, addr(readfds), addr(writefds), addr(exceptfds), addr(timeout)) ;
#endif
          if (res > 0)
            then
              begin
                if (FD_ISSET (dsstr->dpath, addr(readfds)))
                  then
                    accept_ds_socket (dsstr) ;
                else if (dsstr->haveclient)
                  then
                    begin
                      if (FD_ISSET (dsstr->sockpath, addr(readfds)))
                        then
                          read_from_client (dsstr) ;
                      if ((dsstr->sockfull) land (FD_ISSET (dsstr->sockpath, addr(writefds))))
                        then
                          dsstr->sockfull = FALSE ;
                    end
              end
          if (dsstr->haveclient)
            then
              begin
                sent = FALSE ;
                if (dsstr->sockfull)
                  then
                    sleepms (10) ;
                  else
                    begin
                      qlock (dsstr) ;
                      if (dsstr->dsq_in != dsstr->dsq_out)
                        then
                          begin
                            sent = TRUE ;
                            send_next_buffer (dsstr) ;
                          end
                      qunlock (dsstr) ;
                      if (lnot sent)
                        then
                          sleepms (10) ;
                    end
              end
        end
      else
        sleepms (25) ;
  until dsstr->terminate) ;
  dsstr->running = FALSE ;
#ifdef X86_WIN32
  ExitThread (0) ;
  return 0 ;
#else
  pthread_exit (0) ;
#endif
end

pointer lib_ds_start (tds_par *dspar)
begin
  pdsstr dsstr ;
#ifndef X86_WIN32
  integer err ;
  pthread_attr_t attr;
#endif

  dsstr = malloc (sizeof(tdsstr)) ;
  memset (dsstr, 0, sizeof(tdsstr)) ;
  memcpy (addr(dsstr->ds_par), dspar, sizeof(tds_par)) ;
  create_mutex (dsstr) ;
  dsstr->dpath = INVALID_SOCKET ;
  dsstr->sockpath = INVALID_SOCKET ;
  open_socket (dsstr) ;
  if (lnot dsstr->sockopen)
    then
      begin
        free (dsstr) ;
        return NIL ;
      end
#ifdef X86_WIN32
  dsstr->threadhandle = CreateThread (NIL, 0, dsthread, dsstr, 0, (pvoid)addr(dsstr->threadid)) ;
  if (dsstr->threadhandle == NIL)
#else
  err = pthread_attr_init (addr(attr)) ;
  if (err == 0)
    then
      err = pthread_attr_setdetachstate (addr(attr), PTHREAD_CREATE_DETACHED) ;
  if (err == 0)
    then
      err = pthread_create(addr(dsstr->threadid), addr(attr), dsthread, dsstr) ;
  if (err)
#endif
    then
      begin
        free (dsstr) ;
        return NIL ;
      end
  dsstr->running = TRUE ;
  return dsstr ; /* return running context */
end

void lib_ds_stop (pointer ct)
begin
  pdsstr dsstr ;

  dsstr = ct ;
  dsstr->terminate = TRUE ;
  repeat
    sleepms (25) ;
  until (lnot dsstr->running)) ;
  close_socket (dsstr) ;
  destroy_mutex (dsstr) ;
end

void process_ll (pq660 q660, pbyte p, integer chan_offset,
                      longword sec)
begin
  pbyte psave ;
  tdatahdr datahdr ;
  pbyte pd ;
  plcq q ;
  integer size, dsize, offset ;
  byte freq ;

  psave = (pbyte)p ;
  loaddatahdr (addr(p), addr(datahdr)) ;
  q = q660->mdispatch[(datahdr.chan shr 4) + chan_offset][(datahdr.chan and 0xF)] ;
  if (q == NIL)
    then
      return ; /* Can't process at this point, needs BE change */
  strcpy (q660->lowlat_call.location, q->slocation) ;
  strcpy (q660->lowlat_call.channel, q->sseedname) ;
  q660->ll_lcq.gen_src = datahdr.gds ;
  q660->lowlat_call.src_gen = q660->ll_lcq.gen_src ;
  q660->lowlat_call.src_subchan = datahdr.chan ;
  freq = datahdr.chan and 0xF ;
  q660->ll_lcq.rate = FREQTAB[freq] ; /* Convert to actual rate */
  size = ((word)datahdr.blk_size + 1) shl 2 ; /* Total blockette size */
  offset = (datahdr.offset_flag and DHOF_MASK) shl 2 ; /* Offset to start of data */
  dsize = size - offset ; /* Number of data bytes */
  pd = (pbyte)((pntrint)psave + (pntrint)offset) ; /* Start of data blocks */
  q660->ll_lcq.precomp.prev_sample = datahdr.prev_offset ;
  q660->ll_lcq.precomp.prev_value = 0 ; /* Can't be used due to dropped packets */
  q660->ll_lcq.precomp.pmap = (pvoid)p ;
  q660->ll_lcq.precomp.pdata = pd ;
  q660->ll_lcq.databuf = addr(q660->lowlat_call.samples) ;
  q660->ll_lcq.precomp.mapidx = 0 ;
  q660->ll_lcq.precomp.blocks = dsize shr 2 ; /* Number of 32 bit blocks */
  q660->lowlat_call.sample_count = decompress_blockette (q660, addr(q660->ll_lcq)) ;
  q660->lowlat_call.rate = q660->ll_lcq.rate ;
  q660->lowlat_call.reserved = 0 ;
  q660->lowlat_call.timestamp = sec + q660->ll_offset + ((double)datahdr.sampoff / q660->ll_lcq.rate) - q->delay ;
  q660->lowlat_call.qual_perc = q660->ll_data_qual ;
  q660->lowlat_call.context = q660 ;
  memcpy (addr(q660->lowlat_call.station_name), addr(q660->station_ident), sizeof(string9)) ;
  q660->lowlat_call.activity_flags = 0 ;
  if (q660->lowlat_call.qual_perc >= Q_OFF)
   then
     q660->lowlat_call.io_flags = SIF_LOCKED ;
   else
     q660->lowlat_call.io_flags = 0 ;
  if (q660->lowlat_call.qual_perc < Q_LOW)
    then
      q660->lowlat_call.data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
    else
      q660->lowlat_call.data_quality_flags = 0 ;
  q660->lowlat_call.total_size = sizeof(tlowlat_call) - ((MAX_RATE - q660->lowlat_call.sample_count) * sizeof(longint)) ;
//  printf ("Samps=%d, Sec=%d, SOff=%d, Time=%10.6f\r\n", (integer)q660->lowlat_call.sample_count,
//          (integer)sec, (integer)datahdr.sampoff, q660->lowlat_call.timestamp) ;
//  printf ("Samps=%d, Sec=%d, SOff=%d, LLOff=%d\r\n", (integer)q660->lowlat_call.sample_count,
//          (integer)sec, (integer)datahdr.sampoff, (integer)q660->ll_offset) ;
  q660->par_create.call_lowlatency (addr(q660->lowlat_call)) ;
end



