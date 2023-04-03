



/*
    Quanterra Socket Setup Function copyright 2018, 2020 by
    Kinemetrics, Inc.
    Pasadena, CA 91107 USA.

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2018-07-15 rdr Adapted from tcp_forward.c.
    1 2018-07-16 rdr TCP_KEEPCNT are Linux specific, ignore for others.
    2 2018-07-28 jms added necessary header for linux tcp socket options
    3 2020-01-08 rdr Add set_nagle.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "platform.h"
#include "qsockets.h"
#if defined(__APPLE__)
#include <netinet/tcp.h>
#endif

/* Returns non-zero error code or zero for no error */
#ifdef X86_WIN32
int set_server_opts (SOCKET path)
#else
int set_server_opts (int path)
#endif
{
    int one = 1 ;
#ifdef linux
#include <netinet/tcp.h>
    int keepcnt = 6 ;
    int keepidle = 2 * 60 + 5 ; /* 2 Minutes plus 5 seconds */
    int keepintvl = 10 ;
#endif
    int intlth = sizeof(int) ;
    int err ;
#ifdef X86_WIN32
    U32 flag = 1 ;
#else
    int flag ;
#endif

    do { /* So can break out */
        err = setsockopt (path, SOL_SOCKET, SO_KEEPALIVE, (pvoid)&(one), intlth) ;

        if (err)
            break ;

#ifdef linux
        err = setsockopt (path, SOL_TCP, TCP_KEEPCNT, (pvoid)&(keepcnt), intlth) ;

        if (err)
            break ;

        err = setsockopt (path, SOL_TCP, TCP_KEEPIDLE, (pvoid)&(keepidle), intlth) ;

        if (err)
            break ;

        err = setsockopt (path, SOL_TCP, TCP_KEEPINTVL, (pvoid)&(keepintvl), intlth) ;

        if (err)
            break ;

#endif
#ifdef X86_WIN32
        err = ioctlsocket (path, FIONBIO, (pvoid)&(flag)) ;
#else
        flag = fcntl (path, F_GETFL, 0) ;

        if (flag < 0) {
            err = -1 ;
            break ;
        }

        err = fcntl (path, F_SETFL, flag | O_NONBLOCK) ;
#endif
    } while (! TRUE) ;

    if (err) {
#ifdef X86_WIN32
        err = WSAGetLastError() ;
#else
        err = errno ;
#endif
        return err ;
    } else
        return 0 ;
}

/* Returns non-zero error code or zero for no error */
#ifdef X86_WIN32
int set_nagle (SOCKET path, BOOLEAN enable)
#else
int set_nagle (int path, BOOLEAN enable)
#endif
{
#ifdef linux
#include <netinet/tcp.h>
#else
#endif
    int err ;
    int optlth ;
#ifdef X86_WIN32
    U32 flag ;
#else
    int flag ;
#endif

    flag = enable ;
    optlth = sizeof(flag) ;
    err = setsockopt (path, IPPROTO_TCP, TCP_NODELAY, (pvoid)&(flag), optlth) ;

    if (err) {
#ifdef X86_WIN32
        err = WSAGetLastError() ;
#else
        err = errno ;
#endif
        return err ;
    } else
        return 0 ;

}


