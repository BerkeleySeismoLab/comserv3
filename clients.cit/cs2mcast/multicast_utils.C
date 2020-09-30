/***********************************************************

File Name :
	mcast.C

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
#include <stdio.h>
#include <errno.h>
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
#include <arpa/inet.h>
#include <unistd.h>
#include "multicast_utils.h"
#include "RetCodes.h"

int init_multicast_socket(char    *mif,
	                  char    *maddr,
		          int      mport,
		          struct   Multicast_Info& minfo)
{

  minfo.multicast_interface.s_addr = inet_addr(mif);
  minfo.multicast_address.s_addr   = inet_addr(maddr);
  minfo.multicast_port         =  mport;

  //
  // Establish the socket as a Datagram socket
  //

  minfo.soket = socket(AF_INET,SOCK_DGRAM,0);

  if(minfo.soket == -1)
  {
    perror("Error opening socket");
    return(TN_FAILURE);
  }
   

  //
  // Now establish the multicast output interface
  //

  if  (setsockopt(minfo.soket,
		  IPPROTO_IP,
                  IP_MULTICAST_IF, 
                  (char *)&minfo.multicast_interface.s_addr,
                  sizeof(minfo.multicast_interface.s_addr) ) == -1)
  {
    perror("Set Socket Opt IP_MULTICAST_IF error");
    close_multicast_socket(minfo.soket);
    return(TN_FAILURE);
  }


  return(TN_SUCCESS);

}



int multicast_packet(struct Multicast_Info& minfo,
		     char*  packet,
		     int    nbytes)

{

  int lensent;
  static struct sockaddr_in name;
  
  memset(&name,0,sizeof(name));  /* Fill with zeros */
  name.sin_family  = AF_INET;
  name.sin_port    = htons( (unsigned short) minfo.multicast_port);
  name.sin_addr.s_addr = minfo.multicast_address.s_addr;

  lensent = sendto(minfo.soket,
                   (char *) packet,
                   nbytes,
                   0,
                   (struct sockaddr *) &name,
                   sizeof(name));

  if(lensent == -1)
  {
    perror ("Multicast send error");
    return(TN_FAILURE);
  }
  else
  {
    return(TN_SUCCESS);
  }
}


void close_multicast_socket(int soko)
{
  close(soko);
  return;
}
