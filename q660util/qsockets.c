/*
    Quanterra Socket Setup Function copyright 2018 by
    Certified Software Corporation
    John Day, Oregon, USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2018-07-15 rdr Adapted from tcp_forward.c.
    1 2018-07-16 rdr TCP_KEEPCNT are Linux specific, ignore for others.
    2 2018-07-28 jms added necessary header for linux tcp socket options
*/
#include "pascal.h"
#include "qsockets.h"

/* Returns non-zero error code or zero for no error */
#ifdef X86_WIN32
integer set_server_opts (SOCKET path)
#else
integer set_server_opts (integer path)
#endif
begin
  integer one = 1 ;
#ifdef linux
#include <netinet/tcp.h>
  integer keepcnt = 6 ;
  integer keepidle = 2 * 60 + 5 ; /* 2 Minutes plus 5 seconds */
  integer keepintvl = 10 ;
#endif
  integer intlth = sizeof(integer) ;
  integer err ;
#ifdef X86_WIN32
  longword flag = 1 ;
#else
  integer flag ;
#endif

  repeat /* So can break out */
    err = setsockopt (path, SOL_SOCKET, SO_KEEPALIVE, (pvoid)addr(one), intlth) ;
    if (err)
      then
        break ;
#ifdef linux
    err = setsockopt (path, SOL_TCP, TCP_KEEPCNT, (pvoid)addr(keepcnt), intlth) ;
    if (err)
      then
        break ;
    err = setsockopt (path, SOL_TCP, TCP_KEEPIDLE, (pvoid)addr(keepidle), intlth) ;
    if (err)
      then
        break ;
    err = setsockopt (path, SOL_TCP, TCP_KEEPINTVL, (pvoid)addr(keepintvl), intlth) ;
    if (err)
      then
        break ;
#endif
#ifdef X86_WIN32
    err = ioctlsocket (path, FIONBIO, (pvoid)addr(flag)) ;
#else
    flag = fcntl (path, F_GETFL, 0) ;
    if (flag < 0)
      then
        begin
          err = -1 ;
          break ;
        end
    err = fcntl (path, F_SETFL, flag or O_NONBLOCK) ;
#endif
  until TRUE) ;
  if (err)
    then
      begin
#ifdef X86_WIN32
        err = WSAGetLastError() ;
#else
        err = errno ;
#endif
        return err ;
      end
    else
      return 0 ;
end

