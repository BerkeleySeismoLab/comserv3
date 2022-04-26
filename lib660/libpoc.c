



/*   Lib660 POC Receiver
     Copyright 2006, 2013 by
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
    0 2006-09-28 rdr Created
    1 2006-10-29 rdr Remove "addr" function when passing thread address. Fix posix
                     thread function return type.
    2 2007-08-04 rdr Add conditionals for omitting network code.
    3 2010-01-04 rdr Use fcntl instead of ioctl to set socket non-blocking.
    4 2013-02-02 rdr Use actual socket number for select.
    5 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef OMIT_NETWORK

#include "libtypes.h"
#include "libstrucs.h"
#include "libpoc.h"
#include "libmsgs.h"
#include "libclient.h"
#include "libstrucs.h"

#define POC_SIZE (QDP_HDR_LTH + 16 + 4)

typedef struct
{
#ifdef X86_WIN32
    HANDLE threadhandle ;
    U32 threadid ;
#else
    pthread_t threadid ;
#endif
    BOOLEAN running ;
    tpoc_par poc_par ; /* creation parameters */
    tpoc_recvd poc_buf ; /* to build message for client */
#ifdef X86_WIN32
    SOCKET cpath ; /* commands socket */
#else
    int cpath ; /* commands socket */
    int high_socket ;
#endif
    struct sockaddr_in csock ; /* IPV4 address */
    struct sockaddr_in6 csock6 ; /* IPV6 address */
    tqdp recvhdr ;
    BOOLEAN sockopen ;
    BOOLEAN terminate ;
    U8 buf[POC_SIZE] ;
} tpocstr ;
typedef tpocstr *ppocstr ;

static void close_socket (ppocstr pocstr)
{

    pocstr->sockopen = FALSE ;

    if (pocstr->cpath != INVALID_SOCKET) {
#ifdef X86_WIN32
        closesocket (pocstr->cpath) ;
#else
        close (pocstr->cpath) ;
#endif
        pocstr->cpath = INVALID_SOCKET ;
    }

#ifndef X86_WIN32
    pocstr->high_socket = 0 ;
#endif
}

static void read_poc_socket (ppocstr pocstr)
{
    int lth ;
    PU8 p ;
    int err ;

    if (pocstr->cpath == INVALID_SOCKET)
        return ;

    if (pocstr->poc_par.ipv6) {
        lth = sizeof(struct sockaddr_in6) ;
        err = (int)recvfrom (pocstr->cpath, (pvoid)&(pocstr->buf), POC_SIZE, 0, (pvoid)&(pocstr->csock6), (pvoid)&(lth)) ;
    } else {
        lth = sizeof(struct sockaddr) ;
        err = (int)recvfrom (pocstr->cpath, (pvoid)&(pocstr->buf), POC_SIZE, 0, (pvoid)&(pocstr->csock), (pvoid)&(lth)) ;
    }

    if (err == SOCKET_ERROR) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif

        if (err != EWOULDBLOCK)
            if (err == ECONNRESET) {
                close_socket (pocstr) ;

                if (pocstr->poc_par.poc_callback)
                    pocstr->poc_par.poc_callback (PS_CONNRESET, &(pocstr->poc_buf)) ;
            }
    } else if (err > 0) {
        p = (PU8)&(pocstr->buf) ;
        lth = loadqdphdr (&(p), &(pocstr->recvhdr)) ;

        if (lth) {
            loadpoc (&(p), &(pocstr->poc_buf.msg)) ;
            pocstr->poc_buf.ipv6 = pocstr->poc_par.ipv6 ;

            if (pocstr->poc_buf.ipv6)
                memcpy (&(pocstr->poc_buf.ip6_address), &(pocstr->csock6.sin6_addr), sizeof(tip_v6)) ;
            else
                pocstr->poc_buf.ip_address = ntohl(pocstr->csock.sin_addr.s_addr) ;

            if (pocstr->poc_par.poc_callback)
                pocstr->poc_par.poc_callback (PS_NEWPOC, &(pocstr->poc_buf)) ;
        }
    }
}

static void open_socket (ppocstr pocstr)
{
    BOOLEAN isipv6 ;
    int err, j ;
    I32 flag ;
    struct sockaddr_in *psock ;
    struct sockaddr_in6 *psock6 ;
#ifdef X86_WIN32
    BOOL flag2 ;
#else
    int flag2 ;
#endif

    close_socket (pocstr) ;
    isipv6 = pocstr->poc_par.ipv6 ;

    if (isipv6)
        pocstr->cpath = socket (AF_INET6, SOCK_DGRAM, IPPROTO_UDP) ;
    else
        pocstr->cpath = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP) ;

    if (pocstr->cpath == INVALID_SOCKET)
        return ;

    flag = 1 ;
#ifdef X86_WIN32
    ioctlsocket (pocstr->cpath, FIONBIO, (pvoid)&(flag)) ;
#else
    flag = fcntl (pocstr->cpath, F_GETFL, 0) ;
    fcntl (pocstr->cpath, F_SETFL, flag | O_NONBLOCK) ;
#endif
    flag2 = 1 ;
#ifdef X86_WIN32
    j = sizeof(BOOL) ;
    err = setsockopt (pocstr->cpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)&(flag2), j) ;
#else
    j = sizeof(int) ;
    err = setsockopt (pocstr->cpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)&(flag2), j) ;

    if (pocstr->cpath > pocstr->high_socket)
        pocstr->high_socket = pocstr->cpath ;

#endif

    if (isipv6) {
        psock6 = (pointer)&(pocstr->csock6) ;
        memclr (psock6, sizeof(struct sockaddr_in6)) ;
        psock6->sin6_family = AF_INET6 ;
        psock6->sin6_port = htons(pocstr->poc_par.poc_port) ;
        memclr (&(psock6->sin6_addr), sizeof(tip_v6)) ;
        err = bind(pocstr->cpath, (pvoid)&(pocstr->csock6), sizeof(struct sockaddr_in6)) ;
    } else {
        psock = (pointer) &(pocstr->csock) ;
        memclr (psock, sizeof(struct sockaddr)) ;
        psock->sin_family = AF_INET ;
        psock->sin_port = htons(pocstr->poc_par.poc_port) ;
        psock->sin_addr.s_addr = INADDR_ANY ;
        err = bind(pocstr->cpath, (pvoid)&(pocstr->csock), sizeof(struct sockaddr)) ;
    }

    if (err) {
#ifdef X86_WIN32
        closesocket (pocstr->cpath) ;
#else
        close (pocstr->cpath) ;
#endif
        pocstr->cpath = INVALID_SOCKET ;
        return ;
    }

    pocstr->sockopen = TRUE ;
}

#ifdef X86_WIN32
unsigned long  __stdcall pocthread (pointer p)
#else
void *pocthread (pointer p)
#endif
{
    ppocstr pocstr ;
    fd_set readfds, writefds, exceptfds ;
    struct timeval timeout ;
    int res ;

#ifndef X86_WIN32
    pthread_detach (pthread_self ()) ;
#endif
    pocstr = p ;

    do {
        if (pocstr->sockopen) {
            /* wait for socket input or timeout */
            FD_ZERO (&(readfds)) ;
            FD_ZERO (&(writefds)) ;
            FD_ZERO (&(exceptfds)) ;
            FD_SET (pocstr->cpath, &(readfds)) ;
            timeout.tv_sec = 0 ;
            timeout.tv_usec = 25000 ; /* 25ms timeout */
#ifdef X86_WIN32
            res = select (0, &(readfds), &(writefds), &(exceptfds), &(timeout)) ;
#else
            res = select (pocstr->high_socket + 1, &(readfds), &(writefds), &(exceptfds), &(timeout)) ;
#endif

            if (res > 0)
                if (FD_ISSET (pocstr->cpath, &(readfds)))
                    read_poc_socket (pocstr) ;
        } else
            sleepms (25) ;
    } while (! pocstr->terminate) ;

    pocstr->running = FALSE ;
#ifdef X86_WIN32
    ExitThread (0) ;
    return 0 ;
#else
    pthread_exit (0) ;
#endif
}

pointer lib_poc_start (tpoc_par *pp)
{
    ppocstr pocstr ;
#ifndef X86_WIN32
    int err ;
    pthread_attr_t attr;
#endif
#endif

    pocstr = malloc(sizeof(tpocstr)) ;
    memclr (pocstr, sizeof(tpocstr)) ;
    memcpy (&(pocstr->poc_par), pp, sizeof(tpoc_par)) ;
    pocstr->cpath = INVALID_SOCKET ;
    open_socket (pocstr) ;

    if (pocstr->sockopen == FALSE) {
        free (pocstr) ;
        return NIL ;
    }

#ifdef X86_WIN32
    pocstr->threadhandle = CreateThread (NIL, 0, pocthread, pocstr, 0, (pvoid)&(pocstr->threadid)) ;

    if (pocstr->threadhandle == NIL)
#else
    err = pthread_attr_init (&(attr)) ;

    if (err == 0)
        err = pthread_attr_setdetachstate (&(attr), PTHREAD_CREATE_DETACHED) ;

    if (err == 0)
        err = pthread_create (&(pocstr->threadid), &(attr), pocthread, pocstr) ;

    if (err)
#endif

    {
        free (pocstr) ;
        return NIL ;
    } ;

    pocstr->running = TRUE ;

    return pocstr ; /* return running context */
}

void lib_poc_stop (pointer ct)
{
    ppocstr pocstr ;

    pocstr = ct ;
    pocstr->terminate = TRUE ;

    while (pocstr->running)
        sleepms (25) ;

    close_socket (pocstr) ;
}


