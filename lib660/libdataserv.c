



/*   Lib660 Data Server Routines
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
    0 2017-06-08 rdr Created
    1 2017-07-17 rdr Add reporting client IP address and port.
    2 2019-09-25 rdr Fix send_next_buffer so it doesn't keep sending the same
                     record over and over again.
    3 2019-09-26 rdr Various fixes to get low-latency callback to work.
    4 2021-12-11 jms prevent segfault when low_latency callback not defined.
    5 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libdataserv.h"
#include "libmsgs.h"
#include "libclient.h"
#include "libsupport.h"
#include "libstrucs.h"
#include "libcompress.h"

typedef struct
{
#ifdef X86_WIN32
    HANDLE mutex ;
    HANDLE threadhandle ;
    U32 threadid ;
#else
    pthread_mutex_t mutex ;
    pthread_t threadid ;
#endif
    BOOLEAN running ;
    BOOLEAN haveclient ; /* accepted a client */
    BOOLEAN sockopen ;
    BOOLEAN sockfull ; /* last send failed */
    BOOLEAN terminate ;
    tds_par ds_par ; /* creation parameters */
#ifdef X86_WIN32
    SOCKET dpath ; /* dataserv socket */
    struct sockaddr dsockin ; /* dataserv address descriptors */
    SOCKET sockpath ;
#else
    int dpath ; /* commands socket */
    struct sockaddr dsockin ; /* dataserv address descriptors */
    int sockpath ;
    int high_socket ;
#endif
    int dsq_in, dsq_out ;
    double last_sent ;
} tdsstr ;
typedef tdsstr *pdsstr ;

#ifdef X86_WIN32
static void create_mutex (pdsstr dsstr)
{

    dsstr->mutex = CreateMutex(NIL, FALSE, NIL) ;
}

static void destroy_mutex (pdsstr dsstr)
{

    CloseHandle (dsstr->mutex) ;
}

static void qlock (pdsstr dsstr)
{

    WaitForSingleObject (dsstr->mutex, INFINITE) ;
}

static void qunlock (pdsstr dsstr)
{

    ReleaseMutex (dsstr->mutex) ;
}

#else

static void create_mutex (pdsstr dsstr)
{

    pthread_mutex_init (&(dsstr->mutex), NULL) ;
}

static void destroy_mutex (pdsstr dsstr)
{

    pthread_mutex_destroy (&(dsstr->mutex)) ;
}

static void qlock (pdsstr dsstr)
{

    pthread_mutex_lock (&(dsstr->mutex)) ;
}

static void qunlock (pdsstr dsstr)
{

    pthread_mutex_unlock (&(dsstr->mutex)) ;
}

#endif

static void close_socket (pdsstr dsstr)
{

    dsstr->sockopen = FALSE ;

    if (dsstr->dpath != INVALID_SOCKET) {
#ifdef X86_WIN32
        closesocket (dsstr->dpath) ;
#else
        close (dsstr->dpath) ;
#endif
        dsstr->dpath = INVALID_SOCKET ;
    }

    if (dsstr->sockpath != INVALID_SOCKET) {
#ifdef X86_WIN32
        closesocket (dsstr->sockpath) ;
#else
        close (dsstr->sockpath) ;
#endif
        dsstr->sockpath = INVALID_SOCKET ;
    }

#ifndef X86_WIN32
    dsstr->high_socket = 0 ;
#endif
}

static void open_socket (pdsstr dsstr)
{
    int lth, j ;
    int err ;
    int flag ;
    U16 host_port ;
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

    if (dsstr->dpath == INVALID_SOCKET) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif
        sprintf(s, "%d on dataserv[%d] port", err, dsstr->ds_par.server_number) ;
        lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_SOCKETERR, 0, s) ;
        return ;
    }

#ifndef X86_WIN32

    if (dsstr->dpath > dsstr->high_socket)
        dsstr->high_socket = dsstr->dpath ;

#endif
    psock = (pointer) &(dsstr->dsockin) ;
    memset(psock, 0, sizeof(struct sockaddr)) ;
    psock->sin_family = AF_INET ;

    if (dsstr->ds_par.ds_port == PORT_OS)
        psock->sin_port = 0 ;
    else
        psock->sin_port = htons(dsstr->ds_par.ds_port) ;

    psock->sin_addr.s_addr = INADDR_ANY ;
    flag2 = 1 ;
#ifdef X86_WIN32
    j = sizeof(BOOL) ;
    setsockopt (dsstr->dpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)&(flag2), j) ;
#else
    j = sizeof(int) ;
    setsockopt (dsstr->dpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)&(flag2), j) ;
#endif
    flag = sizeof(struct linger) ;
#ifdef X86_WIN32
    getsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)&(lingeropt), (pvoid)&(flag)) ;
#else
    getsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)&(lingeropt), (pvoid)&(flag)) ;
#endif

    if (lingeropt.l_onoff) {
        lingeropt.l_onoff = 0 ;
        lingeropt.l_linger = 0 ;
        flag = sizeof(struct linger) ;
#ifdef X86_WIN32
        setsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)&(lingeropt), flag) ;
#else
        setsockopt (dsstr->dpath, SOL_SOCKET, SO_LINGER, (pvoid)&(lingeropt), flag) ;
#endif
    }

#ifdef X86_WIN32
    err = bind(dsstr->dpath, &(dsstr->dsockin), sizeof(struct sockaddr)) ;

    if (err)
#else
    err = bind(dsstr->dpath, &(dsstr->dsockin), sizeof(struct sockaddr)) ;

    if (err)
#endif

    {
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
    }

    flag = 1 ;
#ifdef X86_WIN32
    ioctlsocket (dsstr->dpath, FIONBIO, (pvoid)&(flag)) ;
    err = listen (dsstr->dpath, 1) ;
#else
    flag = fcntl (dsstr->dpath, F_GETFL, 0) ;
    fcntl (dsstr->dpath, F_SETFL, flag | O_NONBLOCK) ;
    err = listen (dsstr->dpath, 1) ;
#endif

    if (err) {
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
    }

    lth = sizeof(struct sockaddr) ;
    getsockname (dsstr->dpath, (pvoid)&(xyz), (pvoid)&(lth)) ;
    psock = (pointer) &(xyz) ;
    host_port = ntohs(psock->sin_port) ;
    sprintf(s, "on dataserv[%d] port %d", dsstr->ds_par.server_number, host_port) ;
    lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_SOCKETOPEN, 0, s) ;
    dsstr->sockopen = TRUE ;
}

static void accept_ds_socket (pdsstr dsstr)
{
    int lth, err, err2 ;
    int flag ;
    int bufsize ;
    U16 client_port ;
    string63 hostname ;
    string63 s ;
    struct sockaddr_storage tmpsock ;
    struct sockaddr_in *psock ;
#ifndef CBUILDERX
    struct sockaddr_in6 *psock6 ;
    U32 lw6[4] ; /* IPV6 represented as 4 32 bit entries */
#endif

    lth = sizeof(struct sockaddr_storage) ;

    if (dsstr->dpath == INVALID_SOCKET)
        return ;

#ifdef X86_WIN32
    dsstr->sockpath = accept (dsstr->dpath, (pvoid)&(tmpsock), (pvoid)&(lth)) ;
#else
    dsstr->sockpath = accept (dsstr->dpath, (pvoid)&(tmpsock), (pvoid)&(lth)) ;
#endif

    if (dsstr->sockpath == INVALID_SOCKET) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif

        if ((err != EWOULDBLOCK) && (err != EINPROGRESS)) {
#ifdef X86_WIN32
            err2 = closesocket (dsstr->dpath) ;
#else
            err2 = close (dsstr->dpath) ;
#endif
            dsstr->dpath = INVALID_SOCKET ;
            sprintf(s, "%d on dataserv[%d] port", err2, dsstr->ds_par.server_number) ;
            lib_msg_add(dsstr->ds_par.stnctx, AUXMSG_ACCERR, 0, s) ;
        }
    } else {
#ifdef X86_WIN32
        flag = 1 ;
        ioctlsocket (dsstr->sockpath, FIONBIO, (pvoid)&(flag)) ;
#else

        if (dsstr->sockpath > dsstr->high_socket)
            dsstr->high_socket = dsstr->sockpath ;

        flag = fcntl (dsstr->sockpath, F_GETFL, 0) ;
        fcntl (dsstr->sockpath, F_SETFL, flag | O_NONBLOCK) ;
#endif

        if (tmpsock.ss_family == AF_INET) {
            psock = (pointer) &(tmpsock) ;
            showdot (ntohl(psock->sin_addr.s_addr), hostname) ;
            client_port = ntohs(psock->sin_port) ;
        }

#ifndef CBUILDERX
        else {
            psock6 = (pointer) &(tmpsock) ;
            memcpy (&(lw6), &(psock6->sin6_addr), IN6ADDRSZ) ;

            if ((lw6[0] == 0) && (lw6[1] == 0) && (lw6[2] == 0xFFFF0000))
                /* Is actually an IPV4 address */
                showdot (ntohl(lw6[3]), hostname) ;
            else
                strcpy (hostname, inet_ntop6 ((pvoid)&(psock6->sin6_addr))) ;

            client_port = ntohs(psock6->sin6_port) ;
        }

#endif
        sprintf(s, "\"%s:%d\" to dataserv[%d] port", hostname, client_port, dsstr->ds_par.server_number) ;
        lib_msg_add (dsstr->ds_par.stnctx, AUXMSG_CONN, 0, s) ;
        lth = sizeof(int) ;
#ifdef X86_WIN32
        err = getsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)&(bufsize), (pvoid)&(lth)) ;
#else
        err = getsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)&(bufsize), (pvoid)&(lth)) ;
#endif

        if ((err == 0) && (bufsize < 30000)) {
            bufsize = 30000 ;
#ifdef X86_WIN32
            setsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)&(bufsize), lth) ;
#else
            setsockopt (dsstr->sockpath, SOL_SOCKET, SO_SNDBUF, (pvoid)&(bufsize), lth) ;
#endif
        }

#ifndef X86_WIN32
        flag = 1 ;
        lth = sizeof(int) ;
#if defined(linux) || defined(solaris)
        signal (SIGPIPE, SIG_IGN) ;
#else
        setsockopt (dsstr->sockpath, SOL_SOCKET, SO_NOSIGPIPE, (pvoid)&(flag), lth) ;
#endif
#endif
        dsstr->haveclient = TRUE ;
        dsstr->last_sent = now () ;
        dsstr->sockfull = FALSE ;
    }
}

static void read_from_client (pdsstr dsstr)
{
#define RBUFSIZE 100
    int err ;
    U8 buf[RBUFSIZE] ;
    string63 s ;

    do {
        if (! dsstr->haveclient)
            return ;

        err = (int)recv(dsstr->sockpath, (pvoid)&(buf), RBUFSIZE, 0) ;

        if (err == SOCKET_ERROR) {
            err =
#ifdef X86_WIN32
                WSAGetLastError() ;
#else
                errno ;
#endif

            if ((err == ECONNRESET) || (err == ECONNABORTED)) {
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
            }
        }
    } while (! (err == 0)) ; /* nothing left in buffer */
}

static int wrap_buffer (int max, int i)
{
    int j ;

    j = i + 1 ;

    if (j >= max)
        j = 0 ;

    return j ;
}

static void send_next_buffer (pdsstr dsstr)
{
    int err, lth ;
    PI32 pl ;
    string95 s ;

    if (! dsstr->haveclient)
        return ;

    pl = (PI32)&((*(dsstr->ds_par.dsbuf))[dsstr->dsq_out]) ;
    lth = *pl ;
#if defined(linux)
    err = (int)send(dsstr->sockpath, (pvoid)pl, lth, MSG_NOSIGNAL) ;
#else
    err = (int)send(dsstr->sockpath, (pvoid)pl, lth, 0) ;
#endif

    if (err == SOCKET_ERROR) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif

        if (err == EWOULDBLOCK) {
            dsstr->sockfull = TRUE ;
            return ;
        }

#ifdef X86_WIN32
        else if ((err == ECONNRESET) || (err == ECONNABORTED))
#else
        else if (err == EPIPE)
#endif

        {
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
        }
    } else {
        dsstr->sockfull = FALSE ;
        dsstr->dsq_out = wrap_buffer(dsstr->ds_par.record_count, dsstr->dsq_out) ;
    }

    dsstr->last_sent = now () ;
}

void lib_ds_send (pointer ct, plowlat_call pbuf)
{
    pdsstr dsstr ;
    int nq ;
    BOOLEAN wasempty ;

    dsstr = ct ;
    qlock (dsstr) ;
    wasempty = (dsstr->dsq_in == dsstr->dsq_out) ;
    nq = wrap_buffer (dsstr->ds_par.record_count, dsstr->dsq_in) ; /* next pointer after we insert new record */

    if (nq == dsstr->dsq_out)
        dsstr->dsq_out = wrap_buffer(dsstr->ds_par.record_count, dsstr->dsq_out) ; /* throw away oldest */

    memcpy(&((*(dsstr->ds_par.dsbuf))[dsstr->dsq_in]), pbuf, LIB_REC_SIZE) ;
    dsstr->dsq_in = nq ;

    if (wasempty)
        send_next_buffer (dsstr) ;

    qunlock (dsstr) ;
}

#ifdef X86_WIN32
unsigned long  __stdcall dsthread (pointer p)
#else
void *dsthread (pointer p)
#endif
{
    pdsstr dsstr ;
    fd_set readfds, writefds, exceptfds ;
    struct timeval timeout ;
    int res ;
    BOOLEAN sent ;

#ifndef X86_WIN32
    pthread_detach (pthread_self ()) ;
#endif
    dsstr = p ;

    do {
        if (dsstr->sockopen) {
            /* wait for socket input or timeout */
            FD_ZERO (&(readfds)) ;
            FD_ZERO (&(writefds)) ;
            FD_ZERO (&(exceptfds)) ;

            if (! dsstr->haveclient)
                FD_SET (dsstr->dpath, &(readfds)) ; /* waiting for accept */
            else {
                FD_SET (dsstr->sockpath, &(readfds)) ; /* client might try to send me something */

                if (dsstr->sockfull)
                    FD_SET (dsstr->sockpath, &(writefds)) ; /* buffer was full */
            }

            timeout.tv_sec = 0 ;
            timeout.tv_usec = 25000 ; /* 25ms timeout */
#ifdef X86_WIN32
            res = select (0, &(readfds), &(writefds), &(exceptfds), &(timeout)) ;
#else
            res = select (dsstr->high_socket + 1, &(readfds), &(writefds), &(exceptfds), &(timeout)) ;
#endif

            if (res > 0) {
                if (FD_ISSET (dsstr->dpath, &(readfds)))
                    accept_ds_socket (dsstr) ;
                else if (dsstr->haveclient) {
                    if (FD_ISSET (dsstr->sockpath, &(readfds)))
                        read_from_client (dsstr) ;

                    if ((dsstr->sockfull) && (FD_ISSET (dsstr->sockpath, &(writefds))))
                        dsstr->sockfull = FALSE ;
                }
            }

            if (dsstr->haveclient) {
                sent = FALSE ;

                if (dsstr->sockfull)
                    sleepms (10) ;
                else {
                    qlock (dsstr) ;

                    if (dsstr->dsq_in != dsstr->dsq_out) {
                        sent = TRUE ;
                        send_next_buffer (dsstr) ;
                    }

                    qunlock (dsstr) ;

                    if (! sent)
                        sleepms (10) ;
                }
            }
        } else
            sleepms (25) ;
    } while (! dsstr->terminate) ;

    dsstr->running = FALSE ;
#ifdef X86_WIN32
    ExitThread (0) ;
    return 0 ;
#else
    pthread_exit (0) ;
#endif
}

pointer lib_ds_start (tds_par *dspar)
{
    pdsstr dsstr ;
#ifndef X86_WIN32
    int err ;
    pthread_attr_t attr;
#endif

    dsstr = malloc (sizeof(tdsstr)) ;
    memset (dsstr, 0, sizeof(tdsstr)) ;
    memcpy (&(dsstr->ds_par), dspar, sizeof(tds_par)) ;
    create_mutex (dsstr) ;
    dsstr->dpath = INVALID_SOCKET ;
    dsstr->sockpath = INVALID_SOCKET ;
    open_socket (dsstr) ;

    if (! dsstr->sockopen) {
        free (dsstr) ;
        return NIL ;
    }

#ifdef X86_WIN32
    dsstr->threadhandle = CreateThread (NIL, 0, dsthread, dsstr, 0, (pvoid)&(dsstr->threadid)) ;

    if (dsstr->threadhandle == NIL)
#else
    err = pthread_attr_init (&(attr)) ;

    if (err == 0)
        err = pthread_attr_setdetachstate (&(attr), PTHREAD_CREATE_DETACHED) ;

    if (err == 0)
        err = pthread_create(&(dsstr->threadid), &(attr), dsthread, dsstr) ;

    if (err)
#endif

    {
        free (dsstr) ;
        return NIL ;
    }

    dsstr->running = TRUE ;
    return dsstr ; /* return running context */
}

void lib_ds_stop (pointer ct)
{
    pdsstr dsstr ;

    dsstr = ct ;
    dsstr->terminate = TRUE ;

    do {
        sleepms (25) ;
    } while (! (! dsstr->running)) ;

    close_socket (dsstr) ;
    destroy_mutex (dsstr) ;
}

void process_ll (pq660 q660, PU8 p, int chan_offset,
                 U32 sec)
{
    PU8 psave ;
    tdatahdr datahdr ;
    PU8 pd ;
    plcq q ;
    int size, dsize, offset ;
    U8 freq ;

    psave = (PU8)p ;
    loaddatahdr (&(p), &(datahdr)) ;
    q = q660->mdispatch[(datahdr.chan >> 4) + chan_offset][(datahdr.chan & 0xF)] ;

    if (q == NIL)
        return ; /* Can't process at this point, needs BE change */

    if (!q660->par_create.call_lowlatency)
        return ;

    strcpy (q660->lowlat_call.location, q->slocation) ;
    strcpy (q660->lowlat_call.channel, q->sseedname) ;
    q660->ll_lcq.gen_src = datahdr.gds ;
    q660->lowlat_call.src_gen = q660->ll_lcq.gen_src ;
    q660->lowlat_call.src_subchan = datahdr.chan ;
    freq = datahdr.chan & 0xF ;
    q660->ll_lcq.rate = FREQTAB[freq] ; /* Convert to actual rate */
    size = ((U16)datahdr.blk_size + 1) << 2 ; /* Total blockette size */
    offset = (datahdr.offset_flag & DHOF_MASK) << 2 ; /* Offset to start of data */
    dsize = size - offset ; /* Number of data bytes */
    pd = (PU8)((PNTRINT)psave + (PNTRINT)offset) ; /* Start of data blocks */
    q660->ll_lcq.precomp.prev_sample = datahdr.prev_offset ;
    q660->ll_lcq.precomp.prev_value = 0 ; /* Can't be used due to dropped packets */
    q660->ll_lcq.precomp.pmap = (pvoid)p ;
    q660->ll_lcq.precomp.pdata = pd ;
    q660->ll_lcq.databuf = &(q660->lowlat_call.samples) ;
    q660->ll_lcq.precomp.mapidx = 0 ;
    q660->ll_lcq.precomp.blocks = dsize >> 2 ; /* Number of 32 bit blocks */
    q660->lowlat_call.sample_count = decompress_blockette (q660, &(q660->ll_lcq)) ;
    q660->lowlat_call.rate = q660->ll_lcq.rate ;
    q660->lowlat_call.reserved = 0 ;
    q660->lowlat_call.timestamp = sec + q660->ll_offset + ((double)datahdr.sampoff / q660->ll_lcq.rate) - q->delay ;
    q660->lowlat_call.qual_perc = q660->ll_data_qual ;
    q660->lowlat_call.context = q660 ;
    memcpy (&(q660->lowlat_call.station_name), &(q660->station_ident), sizeof(string9)) ;
    q660->lowlat_call.activity_flags = 0 ;

    if (q660->lowlat_call.qual_perc >= Q_OFF)
        q660->lowlat_call.io_flags = SIF_LOCKED ;
    else
        q660->lowlat_call.io_flags = 0 ;

    if (q660->lowlat_call.qual_perc < Q_LOW)
        q660->lowlat_call.data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
    else
        q660->lowlat_call.data_quality_flags = 0 ;

    q660->lowlat_call.total_size = sizeof(tlowlat_call) - ((MAX_RATE - q660->lowlat_call.sample_count) * sizeof(I32)) ;
    /*  printf ("Samps=%d, Sec=%d, SOff=%d, Time=%10.6f\r\n", (integer)q660->lowlat_call.sample_count, */
    /*          (integer)sec, (integer)datahdr.sampoff, q660->lowlat_call.timestamp) ; */
    /*  printf ("Samps=%d, Sec=%d, SOff=%d, LLOff=%d\r\n", (integer)q660->lowlat_call.sample_count, */
    /*          (integer)sec, (integer)datahdr.sampoff, (integer)q660->ll_offset) ; */
    q660->par_create.call_lowlatency (&(q660->lowlat_call)) ;
}



