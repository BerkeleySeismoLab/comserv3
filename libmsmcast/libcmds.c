/*  Libmsmcast Command Processing

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017-2018 Certified Software Corporation
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
    along with Libmsmcast; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#include "libcmds.h"
#include "libclient.h"
#include "libmsgs.h"

#define RECV_BUF_NUM_MSEED_PACKETS 100 /* the maximum number of mseed packets to buffer in recv() */

#define CHUNK_SIZE 10000
#define RETRY_ENTRIES 7
#define CALSTAT_CLEAR_TIMEOUT 200 /* 20 second backup cal status clearing */

#define DT_DATA 0           /* Normal Data */

#undef FAKEERROR
#undef DODBPRINT
#ifdef BSL_DEBUG
/* #define DODBPRINT */
#endif

#ifdef DODBPRINT
#define DBPRINT(A) A
#else
#define DBPRINT(A) 
#endif

void close_socket (pmsmcast msmcast)
{
    if (msmcast->cpath != INVALID_SOCKET)
    {
        close (msmcast->cpath) ;
        msmcast->cpath = INVALID_SOCKET ;
    }
}

void start_deallocation (pmsmcast msmcast)
{

    libmsgadd (msmcast, LIBMSG_DEALLOC, "") ;
    new_state (msmcast, LIBSTATE_DEALLOC) ;
    new_state (msmcast, LIBSTATE_IDLE) ;
    close_socket (msmcast) ;
}

void tcp_error (pmsmcast msmcast, pchar msgsuf)
{
    string95 s ;

    DBPRINT (printf("TCP error: %s\n", (char *) msgsuf));
    lib_change_state (msmcast, LIBSTATE_WAIT, LIBERR_NOTR) ;
    close_socket (msmcast) ;
    sprintf (s, "%s, Waiting %d seconds", msgsuf, 0) ;
    libmsgadd (msmcast, LIBMSG_PKTERR, s) ;
}

void tcp_write_socket (pmsmcast msmcast, pchar s)
{
      /*:: No writing performed on mulitcast read socket.
      integer err ;
      string31 s1 ;

      if (q660->cpath != INVALID_SOCKET)
      {
      err = (integer)send(q660->cpath, s, strlen(s), 0) ;
      if (err == SOCKET_ERROR)
      { 
      err = errno ;
      sprintf (s1, "%d", err) ;
      tcp_error (q660, s1) ;
      }
      else
      add_status (q660, LOGF_SENTBPS, (integer)strlen(s)) ;
      }
      ::*/
}

#define BUFSOCKETSIZE 4000

// points to start of buffer of size size
void init_bufsocket (char *wb, int size)
{
    memset (wb, 0, size);
}

// points to start of buffer 
void push_bufsocket (char *wb, pmsmcast msmcast)
{
    tcp_write_socket (msmcast, wb);
}

// points to address of current buffer pointer. pointer is updated by strlen(s)+1
void write_socket (char **wbp, pchar s)
{
    /*:: No writing performed on mulitcast read socket.
      integer err ;
      string31 s1 ;

      if (msmcast->cpath != INVALID_SOCKET)
      {
      err = (integer)send(msmcast->cpath, s, strlen(s), 0) ;
      if (err == SOCKET_ERROR)
      {
      err = errno ;
      sprintf (s1, "%d", err) ;
      tcp_error (msmcast, s1) ;
      }
      else
      add_status (msmcast, LOGF_SENTBPS, (integer)strlen(s)) ;
      }
      ::*/
}


// points to address of current buffer pointer. pointer is updated by strlen(s)+1
void writeln_socket (char **wbp, pchar s)
{
    /*:: No writing performed on mulitcast read socket.
      write_socket (wbp, s) ;
      write_socket (wbp, "\r\n") ;
      ::*/
}

void change_status_request (pmsmcast msmcast)
{
    /*:: Function not needed. */
}

void lib_continue_registration (pmsmcast msmcast)
{
    word ctrlport ;
    integer lth ;
    struct sockaddr_in xyz ;
    struct sockaddr_in6 xyz6 ;
    string63 s ;
  
    char bufso[BUFSOCKETSIZE];
    char *wb;
  
  
    wb = bufso;
    init_bufsocket (bufso, BUFSOCKETSIZE);

    if (msmcast->ipv6)
    {
        lth = sizeof(struct sockaddr_in6) ;
        getsockname (msmcast->cpath, (pvoid)addr(xyz6), (pvoid)addr(lth)) ;
        ctrlport = ntohs(xyz6.sin6_port) ;
//::        memcpy (addr(msmcast->share.opstat.current_ipv6), addr(xyz6.sin6_addr), sizeof(tip_v6)) ;
    }
    else
    {
        lth = sizeof(struct sockaddr) ;
        getsockname (msmcast->cpath, (pvoid)addr(xyz), (pvoid)addr(lth)) ;
        ctrlport = ntohs(xyz.sin_port) ;
//::        msmcast->share.opstat.current_ip4 = ntohl(xyz.sin_addr.s_addr) ;
    }
//::  msmcast->share.opstat.ipv6 = msmcast->ipv6 ;
    sprintf (s, "on port %d", (integer)ctrlport) ;
    libmsgadd(msmcast, LIBMSG_SOCKETOPEN, s) ;
//::  msmcast->share.opstat.current_port = ctrlport ;
    new_state (msmcast, LIBSTATE_REQ) ;
//::  msmcast->registered = FALSE ;
}

void enable_packet_streaming (pmsmcast msmcast)
{
    /*:: Function not needed. */
}

void send_user_message (pmsmcast msmcast, pchar msg)
{
    /*:: Function not needed. */
}

void send_logger_stat (pmsmcast msmcast, integer val)
{
    /*:: Function not needed. */
}

void send_chan_mask (pmsmcast msmcast)
{
    /*:: Function not needed. */
}

static boolean open_socket (pmsmcast msmcast)
{
    integer flag, j, err  ;
    longword ip_v4 ;
    longword if_v4;
    word port ;
    boolean is_ipv6 ;
    boolean internal_dl ;
    struct sockaddr_in *psock ;
    struct sockaddr_in6 *psock6 ;
    struct in_addr ip_addr;
    struct in_addr if_addr;
    struct in6_addr if6_addr;
    tip_v6 if_v6;
    tip_v6 ip_v6 ;
    string s ;
    pchar pc ;
    integer flag2 ;
    int recv_buf_bytes ;
    struct ip_mreq mreq;
    struct ipv6_mreq mreq6 ;
    close_socket (msmcast) ;
    is_ipv6 = FALSE ;

    /* Open a UDP socket. */
    strcpy (s, msmcast->par_register.msmcastif_address) ;
    if_v4 = get_ip_address (s, addr(if_v6), addr(is_ipv6), msmcast->par_register.prefer_ipv4) ;
    if ((! is_ipv6) && (ip_v4 == INADDR_NONE))
    {
        libmsgadd(msmcast, LIBMSG_ADDRERR, msmcast->par_register.msmcastif_address) ;
        return TRUE ;
    }
    if (is_ipv6)
	msmcast->cpath = socket (AF_INET6, SOCK_DGRAM, 0) ;
    else
	msmcast->cpath = socket (AF_INET, SOCK_DGRAM, 0) ;
    if (msmcast->cpath == INVALID_SOCKET)
    {
        err = errno ;
        sprintf (s, "%d opening socket on Multicast Address %s",
		 err, msmcast->par_register.msmcastif_address) ;

        libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
        return TRUE ;
    }

    /* Set the REUSEADDR option on the socket. */
    flag2 = 1 ;
    j = sizeof(integer) ;
    err = setsockopt (msmcast->cpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)addr(flag2), j) ;
    if ( err )
    {
	err = errno ;
	sprintf (s, "%d setsockopt SO_REUSEADDR on Multicast Address %s", 
		 err, msmcast->par_register.msmcastif_address) ;
	libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
	close_socket(msmcast) ;
	return TRUE ;
    }

    /* Add multicast interface option. This specifies the ip address of the */
    /* network port we are to listen on. This is important if the machine */
    /* has multiple network interfaces. */
    if (is_ipv6)
    {
	memcpy (addr(if6_addr.s6_addr), addr(if_v6), sizeof(if_v6)) ;
	err = setsockopt(msmcast->cpath,IPPROTO_IP,IPV6_MULTICAST_IF,
			 (char *)&if6_addr,sizeof(if6_addr));
    }
    else {
	if_addr.s_addr = if_v4; /* always big-endian */
	err = setsockopt(msmcast->cpath,IPPROTO_IP,IP_MULTICAST_IF,
			 (char *)&if_addr,sizeof(if_addr));
    }
    if ( err )
    {
	err = errno ;
	sprintf (s, "%d setsockopt IP_MULTICAST_IF for socket on Multicast Address %s",
		 err, msmcast->par_register.msmcastif_address) ;
	libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
	close_socket(msmcast) ;
	return TRUE ;
    }

    /* Bind the specified (multicast) address to the UDP socket. */
    strcpy (s, msmcast->par_register.msmcastid_address) ;
    ip_v4 = get_ip_address (s, addr(ip_v6), addr(is_ipv6), msmcast->par_register.prefer_ipv4) ;
    port = msmcast->par_register.msmcastid_udpport ;
    if (is_ipv6)
    {
        psock6 = (pointer) addr(msmcast->msock6) ;
        memset(psock6, 0, sizeof(struct sockaddr_in6)) ;
        psock6->sin6_family = AF_INET6 ;
        psock6->sin6_port = htons(port) ;
        memcpy (addr(psock6->sin6_addr), addr(ip_v6), sizeof(tip_v6)) ;
	err = bind(msmcast->cpath, (pvoid)addr(msmcast->msock6), sizeof(struct sockaddr_in6)) ;
    }
    else
    {
        psock = (pointer) addr(msmcast->msock) ;
        memset(psock, 0, sizeof(struct sockaddr)) ;
        psock->sin_family = AF_INET ;
        psock->sin_port = htons(port) ;
        psock->sin_addr.s_addr = ip_v4 ; /* always big-endian */
	err = bind(msmcast->cpath, (pvoid)addr(msmcast->msock), sizeof(struct sockaddr)) ;
    }
    if ( err )
    {
	err = errno ;
	sprintf (s, "%d bind port %hd for socket on Multicast Address %s",
		 err, htons(port), msmcast->par_register.msmcastif_address) ;
	libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
	close_socket(msmcast) ;
	return TRUE ;
    }

    /* Increase the recv() buffer to handle a slew of mseed packets */
    /* while the program is processing clients. */
    recv_buf_bytes = MAX_MSEED_BLKSIZE * RECV_BUF_NUM_MSEED_PACKETS;
    err = setsockopt(msmcast->cpath,SOL_SOCKET,SO_RCVBUF,
		     &recv_buf_bytes,sizeof(recv_buf_bytes));
    if ( err )
    {
	err = errno ;
	sprintf (s, "%d setsockopt SO_RCVBUF %d on Multicast Address %s", 
		 err, recv_buf_bytes, msmcast->par_register.msmcastif_address) ;
	libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
	close_socket(msmcast) ;
	return TRUE ;
    }

    /* Join the multicast group for the specified address. */
    /* This should be in the multicast address range 224.xxxxx to xxxxxx */
    if(is_ipv6) {
	memcpy(&mreq6.ipv6mr_multiaddr,if_v6,sizeof(if_v6));
	mreq6.ipv6mr_interface = 0;
	err = setsockopt(msmcast->cpath,IPPROTO_IP,IPV6_ADD_MEMBERSHIP,
			 (char *)&mreq6,sizeof(mreq6));
    }
    else
    {
	mreq.imr_multiaddr.s_addr=ip_v4;
	mreq.imr_interface.s_addr=if_v4 ;
	err = setsockopt(msmcast->cpath,IPPROTO_IP,IP_ADD_MEMBERSHIP,
			 (char *)&mreq,sizeof(mreq));
    }
    if ( err )
    {
	err = errno ;
	sprintf (s, "%d setsockopt IP_ADD_MEMBERSHIPF for Multicast Address %s",
		 err, msmcast->par_register.msmcastif_address) ;
	libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
	close_socket(msmcast) ;
	return TRUE ;
    }

    /* Set the socket to be nonblocking. */
    flag = 1 ;
    if (msmcast->cpath > msmcast->high_socket)
	msmcast->high_socket = msmcast->cpath ;
    flag = fcntl (msmcast->cpath, F_GETFL, 0) ;
    fcntl (msmcast->cpath, F_SETFL, flag | O_NONBLOCK) ;
    if (err)
    {
        err = errno ;
        if ((err != EWOULDBLOCK) && (err != EINPROGRESS))
	{
	    close_socket (msmcast) ;
	    sprintf (s, "%d on Multicast Address %s",
		 err, msmcast->par_register.msmcastif_address) ;
	    libmsgadd(msmcast, LIBMSG_SOCKETERR, s) ;
	    return TRUE ;
	}
    }

    msmcast->ipv6 = is_ipv6 ;
    return FALSE ;
}

void lib_start_registration (pmsmcast msmcast)
{

    if (open_socket (msmcast))
    {
        lib_change_state (msmcast, LIBSTATE_WAIT, LIBERR_NETFAIL) ;
        libmsgadd (msmcast, LIBMSG_SNR, "0 Minutes") ;
        return ;
    } ;
    /* Since this library is used for connectionless data input, */
    /* we NEVER want to be in stat LIBSTAT_CONN.  Go to LIBSTAT_RUNWAIT */
    //::   new_state (msmcast, LIBSTATE_CONN) ;
    new_state (msmcast, LIBSTATE_RUNWAIT);
      
}

static void process_packet (pmsmcast msmcast, integer lth)
{
    /*:: Function not needed. */
}

static void extract_value (pchar src, pchar dest)
{
    /*:: Function not needed. */
}

static void check_for_disconnect (pmsmcast msmcast, integer err)
{
    /*:: Function not needed. */
}


void read_mcast_socket (pmsmcast msmcast)
{
    integer err ;
    integer recsize ;
    integer good ;
    string s ;

    switch (msmcast->libstate) {
    case LIBSTATE_RUN : /* Streaming Binary Packets */
	if (msmcast->cpath == INVALID_SOCKET)
	{
	    DBPRINT(printf ("DEBUG:: Invalid socket in read_mcast_socket"));
	    return ;
	}
	err = (integer)recv (msmcast->cpath, (pvoid)addr(msmcast->tcpbuf[msmcast->tcpidx]),
			     TCPBUFSZ - msmcast->tcpidx, 0) ;
	DBPRINT(printf ("DEBUG:: recv err=%d\n", err));
	if (err == SOCKET_ERROR)
	{
	    err = errno ;
	    if (err == EPIPE)
		check_for_disconnect (msmcast, err) ;
	    else if (err != EWOULDBLOCK)
	    {
		sprintf (s, "%d", err) ;
		tcp_error (msmcast, s) ;
	    }
	    return ;
	}
	else if (err == 0)
	{
	    DBPRINT (printf("state=%d recv err 0, read size=%d\n",msmcast->libstate, TCPBUFSZ - msmcast->tcpidx));
	    check_for_disconnect (msmcast, ECONNRESET) ;
	    return ;
	}
	else if (err < 0)
	{
	    return ;
	}
	/* Increment status info for the data packet read. */
	add_status (msmcast, LOGF_RECVBPS, err) ;
	add_status (msmcast, LOGF_PACKRECV, 1) ;

	/* Process and distribute MiniSEED data packet. */
	int recsize = err;
	DBPRINT(printf ("DEBUG:: calling process_mseed\n"));
	good = process_mseed (msmcast, (pvoid)addr(msmcast->tcpbuf[msmcast->tcpidx]), recsize);
	DBPRINT(printf ("DEBUG:: process_mseed rc=%d\n", good));
	if (! good)
	{
	    tcp_error (msmcast, "Invalid Data Packet") ;
	}
	
	/* Since this library read multicast UDP MiniSEED packets, */
	/* by definition there is only one MiniSEED record per packet. */
//::       	msmcast->tcpidx = msmcast->tcpidx + recsize ;
	msmcast->tcpidx = 0;
	break ;
    default :
	new_state (msmcast, LIBSTATE_RUN) ;
	break ;
    }
}

void lib_timer (pmsmcast msmcast)
{
    string s ;

    if (msmcast->libstate != msmcast->share.target_state)
    {
        switch (msmcast->share.target_state) {
	case LIBSTATE_WAIT :
	case LIBSTATE_IDLE :
            switch (msmcast->libstate) {
	    case LIBSTATE_IDLE :
                new_state (msmcast, msmcast->share.target_state) ;
		DBPRINT (printf("settarg #7 = %d\n",msmcast->share.target_state));
		break ;
	    case LIBSTATE_CONN :
		DBPRINT (printf("state=%d targy=%d close socket\n",msmcast->libstate,msmcast->share.target_state));
		close_socket (msmcast) ;
		new_state (msmcast, msmcast->share.target_state) ;
		DBPRINT (printf("settarg #6 = %d\n",msmcast->share.target_state));
		break ;
	    case LIBSTATE_RUNWAIT :
	    case LIBSTATE_RUN :
		DBPRINT (printf("doing deall #1\n"));
		start_deallocation(msmcast) ;
                break ;
	    case LIBSTATE_WAIT :
                new_state (msmcast, msmcast->share.target_state) ;
		DBPRINT (printf("settarg #5 = %d\n",msmcast->share.target_state));
		break ;
	    default :
                break ;
            }
            break ;
	case LIBSTATE_RUNWAIT :
            switch (msmcast->libstate) {
	    case LIBSTATE_RUN :
		DBPRINT (printf("doing deall #2\n"));
		start_deallocation(msmcast) ;
                break ;
	    case LIBSTATE_IDLE :
                lib_start_registration (msmcast) ;
                break ;
	    case LIBSTATE_WAIT :
                lib_start_registration (msmcast) ;
                break ;
	    default :
                break ;
            }
            break ;
	case LIBSTATE_RUN :
            switch (msmcast->libstate) {
	    case LIBSTATE_RUNWAIT :
                new_state (msmcast, LIBSTATE_RUN) ;
                break ;
	    default :
                break ;
            }
            break ;
	case LIBSTATE_TERM :
            if (! msmcast->terminate)
	    {
		msmcast->terminate = TRUE ;
	    }
            break ;
	default :
            break ;
        }
    }
    switch (msmcast->libstate) {
    case LIBSTATE_IDLE :
//::	inc(msmcast->timercnt) ;
//::	if (msmcast->timercnt >= 10)
//::	{ /* about 1 second */
//::            msmcast->timercnt = 0 ;
//::            msmcast->share.opstat.runtime = msmcast->share.opstat.runtime - 1 ;
//::            continuity_timer (msmcast) ;
//::	}
	return ;
    case LIBSTATE_TERM :
	return ;
    case LIBSTATE_WAIT :
	lib_change_state (msmcast, LIBSTATE_RUNWAIT, LIBERR_NOERR) ;
	return ;
    case LIBSTATE_RUN :
	break ;
    default :
	break ;
    }
}

