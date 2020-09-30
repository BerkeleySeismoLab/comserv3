/*$Id: os9inet.h,v 1.1.1.1 2004/06/15 19:08:01 isti Exp $*/
/*   Berkeley sockets definitions
     Copyright 1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0  4 Jun 96 WHO Created.
    1  7 Dec 96 WHO sendto and recv added.
*/

#ifndef OS9INET_H
#define OS9INET_H

#include <inet/socket.h>
#include <inet/in.h>

/* no socket definition file is provided, so define function
   prototypes here */
  
int socket (int family, int type, int protocol) ;
int bind (int sockfd, struct sockaddr *myaddr, int addrlen) ;
int connect (int sockfd, struct sockaddr *servaddr, int addrlen) ;
int listen (int sockfd, int backlog) ;
int accept (int sockfd, struct sockaddr *peer, int *addrlen) ;
int getsockopt (int sockfd, int level, int optname, char *optval, int *optlen) ;
int setsockopt (int sockfd, int level, int optname, char *optval, int optlen) ;
int shutdown (int sockfd, int howto) ;
int sendto (int s, char *msg, int len, int flags, struct sockaddr *to,
            int tolen) ;
int recv (int s, char *buf, int len, int flags) ;

#endif
