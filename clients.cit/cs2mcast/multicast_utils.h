/***********************************************************

File Name :
	mcast.h

Programmer:
	Phil Maechling

Description:
	These are routines to initialize an output multicast socket,
	and to transmit data out over that sockets.

Limitations or Warnings:

Creation Date:
	16 Feb 1999

Modification History:
    2020-09-29 DSN Updated for comserv3.

Usage Notes:

**********************************************************/
#include <iostream>
#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h>


struct Multicast_Info {
 struct in_addr multicast_interface;
 struct in_addr multicast_address;
 int            multicast_port;
 int		socket_fd;
};


int init_multicast_socket(char   *mif,
	                  char   *maddr,
		          int    mport,
		          struct Multicast_Info& minfo);

int multicast_packet(struct Multicast_Info& minfo,
		     char*  packet,
		     int    nbytes);

void close_multicast_socket(int socket_fd);
