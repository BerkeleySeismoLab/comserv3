/*
 * File     :
 *  comserv_queue.c
 *
 * Purpose  :
 *  This modification of the original comlink.c is for use with servers
 *  that receive MiniSEED data from logger-spedific libraries.
 *  The code inserts the packet into comserv's internal queues so that
 *  it can be delivered to comserv clients.
 *
 * Author   :
 *  Phil Maechling
 *
 *
 * Mod Date :
 *  6 April 2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 */
/*$Id: clink.c,v 1.3 2005/06/17 19:29:53 isti Exp $*/
/*   Server comlink protocol file
     Copyright 1994-1998 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 23 Mar 94 WHO First created.
    1  9 Apr 94 WHO Command Echo processing added.
    2 15 Apr 94 WHO Commands done.
    3 30 May 94 WHO If rev 1 or higher ultra record received, set comm_mask.
    4  9 Jun 94 WHO Consider a "busy" client still active if it is a
                    foreign client, without trying to do a Kill (0).
                    Cleanup to avoid warnings.
    5 11 Jun 94 WHO Martin-Marietta support added.
    6  9 Aug 94 WHO Set last_good and last_bad fields in linkstat.
    7 11 Aug 94 WHO Add support for network.
    8 25 Sep 94 WHO Allow receiving RECORD_HEADER_3.
    9  3 Nov 94 WHO Use SOLARIS2 definition to alias socket parameter.
   10 13 Dec 94 WHO Remove record size in seedheader function, COMSERV always
                    uses 512 byte blocks.
   11 16 Jun 95 WHO Don't try to open closed network connection until
                    netdly_cnt reaches netdly. Clear netto_cnt timeout
                    counter when a packet is received.
   12 20 Jun 95 WHO Updates due to link adj and link record packets. Transmitted
                    packet circular buffer added to accomodate grouping.
   13 15 Aug 95 WHO Set frame_count for Q512 and MM256 data packets.
   14  2 Oct 95 WHO Implement new link_pkt/ultra_req handshaking protocol.
   15 28 Jan 96 DSN Update check_input to handle unexpected characters better.
                    Correctly assign state when unexpected character is found.
   16  3 Jun 96 WHO Start of conversion to OS9
   17  4 Jun 96 WHO Comparison for dbuf.seq being positive removed, seq
                    is unsigned. cli_addr, network, and station made external.
   18  7 Jun 96 WHO Check result of "kill" with ERROR, not zero.
   19 13 Jun 96 WHO Adjust seedname and location fields for COMMENTS.
   20  3 Aug 96 WHO If anystation flag is on, don't check station, and show what
                    station came from. If noultra is on, don't poll for ultra
                    packets.
   21  7 Dec 96 WHO Add support for Blockettes and UDP.
   22 11 Jun 97 WHO Clear out remainder of packets that are received with
                    less than 512 bytes. Convert equivalent of header_flag
                    in blockette packet to seed sequence number.
   23 27 Jul 97 WHO Handle FLOOD_PKT.
   24 17 Oct 97 WHO Add VER_COMLINK
   25 23 Dec 98 WHO Use link_retry instead of a fixed 10 seconds for
                    polling for link packets (VSAT).
   26  4 Nov 99 WHO Change "=" to "==" in cmd_echo processing.
   27 18 May 00 WHO Count the sequence not being valid as a "con_seq"
                    error, forcing a reconfigure.
   28  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
      12 Nov 99 IGD Include unistd.h for POSIX - compatibility
                    Add LINUX #ifdefed byte swapping in cserv_crcinit() and
                    Add byte swapping for checksum()
                    Use a different version of cserv_crccalc () with LINUX
      13 Nov 99 IGD Modified for speed LINUX version of cserv_crccalc
                    Inserted  flip..()  into the LINUX version of process()  ULTRA_PKT case
      14 Nov 99 IGD Inserted flip..() into the LINUX version of process() SEQUENCE_SPECIAL case
      17 Nov 99 IGD Removed #ifdefs for the flipX() commands , since
                    the #ifdefs are now defined in util/stuff.c
                    All additional calls to flip() from the old ported version are incorporated
      19 Nov 99 IGD Fixed a bug pointed by bob@certsoft.com:
                    if ((replybuf.ces.dp_seq = pcom->command_tag) &&  (= instead of ==)
       6 Dec 99 IGD curlink.window_size is char, no swapping is necessary, flip2 is removed
                    Contrary, it should be flip2(curlink.synctime)) in the next printf.
                    Additionally, I introduce 7 more byte flips in printf messages of case
                    SEQUENCE_SPECIAL of process. Fixed a minor bug - call to fillbuf did not have brackets	
      14 Dec 99 IGD Commented out the location of the possible data swap of Ultra-SHEAR package. Don't know if
                    we have to do it. Introduced swaps in curlink -> process(); case SEQUENCE_SPECIAL
                    Removed corresponding swaps in printing statements
                    Introduced swaps in pultra  -> process(): case ULTRA_PKT
                    Started porting calibration structure: need a way to do it smart!
      15 Dec 99 IGD Defined byte-swapping pointer to char * tmpa in process for storing arrays to swap
                    Wrote flip_calibrators (void *) routine and implied it in ULTRA_PKT case of process()
                    Wrote flip_cahnnels() and used it in ULTRA_PKT case of process().
      16 Dec 99 IGD Wrote flip_detectors which takes care of assining detectors info to the client
      17 Dec 99 IGD The bug fix implemented previously (Nov 19) is not good, remove it
      23 Jan 01 IGD Byte-swapping of ultrashear packet header is moved inside if (full) {} condition.
                    Previously, we swapped the header as many times as we got segments
 29   17 Mar 01 IGD Added support for -D_BIG_ENDIAN_HEADER: COMSERV for Linux can put MSEED packets in big-
		    endian-order in the shared memory if this compilation flag is set	 	
      17 Mar 01 IGD Header order bits (7 and 8) are in the I/O and clock flags byte (see comments for
                    set_byte_order_SEED_IO_BYTE() ).
 30   24 Aug 07 DSN Separate ENDIAN_LITTLE from LINUX logic.
 31   29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "quanstrc.h"
#include "stuff.h"
#include "service.h"
#include "cfgutil.h"
#include "server.h"
#include "timeutil.h"
#include "logging.h"

#ifdef	LINUX
#include "unistd.h"
#ifndef FIRSTDATA       /* IGD 03/05/01 Add */
#define FIRSTDATA 56
#endif
#endif

#define SEED_IO_BIT6 0x40    /* IGD 03/09/01 */
#define SEED_IO_BIT7 0x80    /* Flags for placing the */				
#define SET_BIG_ENDIAN 1     /* swapping bits into I/O and clock flags byte */
#define SET_LITTLE_ENDIAN 0  /* of fixed SEED header */


short VER_COMLINK = 31 ;

#ifdef COMSERV2
extern complong station;
#else
extern string15 station ;
#endif

/* Comserv external variables used in this file. */
extern pchar dest ;
extern linkstat_rec linkstat ;
extern boolean override ;	// No longer used?
extern int32_t netto_cnt ;

/* External variables used only in this file */
string3 seed_names[20][7] =
           /* VBB   VSP    LG    MP    LP   VLP   ULP */
  { /*V1*/ {"BHZ","EHZ","HLZ","MHZ","LHZ","VHZ","UHZ"},
    /*V2*/ {"BHN","EHN","HLN","MHN","LHN","VHN","UHN"},
    /*V3*/ {"BHE","EHE","HLE","MHE","LHE","VHE","UHE"},
    /*V4*/ {"BXZ","EXZ","HXZ","MXZ","LXZ","VXZ","UXZ"},
    /*V5*/ {"BXN","EXN","HXN","MXN","LXN","VXN","UXN"},
    /*V6*/ {"BXE","EXE","HXE","MXE","LXE","VXE","UXE"},
    /*ZM*/ {"BMZ","EMZ","HMZ","MMZ","LMZ","VMZ","UMZ"},
    /*NM*/ {"BMN","EMN","HMN","MMN","LMN","VMN","UMN"},
    /*EM*/ {"BME","EME","HME","MME","LME","VME","UME"},
    /*L1*/ {"BCI","ECI","HCI","MCI","LCI","VCI","UCI"},
    /*L2*/ {"BTI","ETI","HTI","MTI","LTI","VTI","UTI"},
    /*L3*/ {"BPI","EPI","HPI","MPI","LPI","VPI","UPI"},
    /*L4*/ {"BX1","EX1","HX1","MX1","LX1","VX1","UX1"},
    /*L5*/ {"BX2","EX2","HX2","MX2","LX2","VX2","UX2"},
    /*L6*/ {"BX3","EX3","HX3","MX3","LX3","VX3","UX3"},
    /*L7*/ {"BX4","EX4","HX4","MX4","LX4","VX4","UX4"},
    /*L8*/ {"BX5","EX5","HX5","MX5","LX5","VX5","UX5"},
    /*L9*/ {"BX6","EX6","HX6","MX6","LX6","VX6","UX6"},
    /*R1*/ {"BX7","EX7","HX7","MX7","LX7","VX7","UX7"},
    /*R2*/ {"BX8","EX8","HX8","MX8","LX8","VX8","UX8"}} ;

/* External functions used in this file. */
tring_elem *getbuffer (short qnum) ;
boolean bufavail (short qnum) ;
boolean checkmask (short qnum) ;
void flip_fixed_header(seed_fixed_data_record_header *);
char set_byte_order_SEED_IO_BYTE(char, short); /* IGD 03/09/01 */
char process_set_byte_order_SEED_IO_BYTE(char);

/***********************************************************************
 * comserv_anyQueueBlocking()
 *	This routine checks to see if any of the comserv queues (rings)
 *	would block if you tried to insert a new packet.
 *   Return values:
 *	true: if packet buffer not available in one or more queues.
 *	false: if packet buffer available in all queues.
 ***********************************************************************/
int comserv_anyQueueBlocking() {
    return ! ( bufavail(DATAQ) && bufavail(DETQ) && bufavail(CALQ) &&
	       bufavail(TIMQ) && bufavail(MSGQ) && bufavail(BLKQ) );
}

/***********************************************************************
 * comserv_queue
 *	This routines inserts the provided MiniSEED packet into comserv's
 *	internal queues (rings) based on the packettype;
 *      In the original comserv, this routine was called process.
 *   Return values:
 *	0  : Packet was sent into comserv memory
 *	1  : Packet was not sent due to blocking
 *	-1 : Packet wasn't recognised (nor sent into comserv)
 ***********************************************************************/
int comserv_queue(char* buf,int len,int packettype)
{
    short size ;
    tring_elem *freebuf ;
    seed_fixed_data_record_header *pseed ;

    /* Create a temporary storage area for the received packets seed header */
    seed_fixed_data_record_header my_seed_header;
    int packet_type;
    double packet_time;

    /* Move parameters into local vars */
    memcpy(dest,buf,len);
    packet_type = packettype;
    size = (short ) len;

    /* Increment packet count. */
    linkstat.total_packets++ ;

    /* Debug only statments here */
    // tsize = (short) ((int32_t) term - (int32_t) dest);
    // printf("For debug show calculated size - mserv %d\n",tsize);
    // tsize = (short) ((int32_t) term - (int32_t) dest - 6) ;
    // printf("For debug show calculated size - comlink %d\n",tsize);

    /* At this point, all known packets are 512 bytes in length or less, so reject */
    /* any that are not larger. */
    if (size > 512) 
    {
	LogMessage(CS_LOG_TYPE_ERROR, "Unknown packet size %d\n",size);
	return -1;
    }

    /* All received packets are considered valid (no checksum calc) */
    /* Because they were received by the calling program */
    /* Here we process the packet, and put it into the next avaiable comserv */
    /* Ring buffer slot. */
    netto_cnt = 0 ; /* got a packet, reset timeout */
    linkstat.last_good = dtime () ;

    /*:: NO LONGER SUPPORTING OVERRIDING THE SEED SITENAME IN THE INCOMING MSEED RECORD. */

    /* Copy the header portion of the received packet. */
    memcpy(&my_seed_header,dest,64);

    /* We punt and use the current time as the packet time. */
    packet_time = dtime();

    /* Find a free buffer in the ring for the specified packet time. */
    /* If no free buffer, return an error. */
    /* Caller gets to decide how to handle this situatin. */
    switch(packet_type)
    {
    case RECORD_HEADER_1 :
    {
	freebuf = getbuffer (DATAQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    case BLOCKETTE :
    {
	freebuf = getbuffer (BLKQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    case COMMENTS :
    {
	freebuf = getbuffer (MSGQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    case CLOCK_CORRECTION :
    {
	freebuf = getbuffer (TIMQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    case DETECTION_RESULT :
    {
	freebuf = getbuffer (DETQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    case END_OF_DETECTION :
    {
	freebuf = getbuffer (DATAQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    case CALIBRATION :
    {
	freebuf = getbuffer (CALQ) ;    /* get free buffer */
	if(freebuf == NULL)
	{
	    return 1;
	}
	break;
    }
    default :
    {
	LogMessage(CS_LOG_TYPE_ERROR, "Unknown Packet Type %d\n",packet_type);
	return -1;
    }
    } /* End of Switch */

    /* Now freebuf points to the proper comserv ring buffer. Now we */
    /* put the data into the buffer. */
    pseed = (pvoid) &freebuf->user_data.data_bytes ;
    freebuf->user_data.reception_time = dtime () ;    
    /* reception time */
    freebuf->user_data.header_time = packet_time;
    memcpy (pseed,dest,512) ;
    
    return 0;
}

/* IGD 03/09/01
 * byteswapping of swappable elements in MSEED header
 */

void flip_fixed_header(seed_fixed_data_record_header *pseed)
{
    pseed->header.starting_time.yr = flip2(pseed->header.starting_time.yr);
    pseed->header.starting_time.jday = flip2(pseed->header.starting_time.jday);
    pseed->header.starting_time.tenth_millisec = flip2(pseed->header.starting_time.tenth_millisec);
    pseed->header.samples_in_record = flip2(pseed->header.samples_in_record);
    pseed->header.sample_rate_factor = flip2(pseed->header.sample_rate_factor);
    pseed->header.sample_rate_multiplier = flip2(pseed->header.sample_rate_multiplier);
    pseed->header.tenth_msec_correction = flip4(pseed->header.tenth_msec_correction);
    pseed->header.first_data_byte = flip2(pseed->header.first_data_byte);
    pseed->header.first_blockette_byte = flip2(pseed->header.first_blockette_byte);
    pseed->dob.blockette_type = flip2(pseed->dob.blockette_type);
    pseed->dob.next_offset = flip2(pseed->dob.next_offset);
}

/* IGD 03/09/01
 * This function sets the SEED header byte-order bits in
 * the I/O and clock flags byte [Standard for the Exchange
 * of Earthquake Data, Reference manual p.93, 1993] of
 * the fixed header.
 * -------------------
 * SEED_IO_BIT6 = 0x40;
 * SEED_IO_BIT7 = 0x80;
 * LITTLE_ENDIAN = 0
 * BIG_ENDIAN = 1;
 * ----------------------
 * Here is how it works:
 *	Bit6 Bit7
 *	 0    0	- indetermined (old data)
 *	 1    0 - header is little endian
 *	 0    1 - header is big endian
 */

char set_byte_order_SEED_IO_BYTE(char in, short byteOrder)
{
    if (byteOrder == 1)
    {       /* BIG-ENDIAN CASE */
	in = in &  ~SEED_IO_BIT6; /* Turn OFF bit 6 */
	in = in |  SEED_IO_BIT7; /* Turn ON bit 7 */
    }

    else if (byteOrder == 0)
    {       /* LITTLE-ENDIAN case */
	in = in |  SEED_IO_BIT6; /* Turn ON bit 6 */
	in = in &  ~SEED_IO_BIT7; /* Turn OFF bit 7 */
    }
    else
    {
	/* indetermine ... case */
	in = in &  ~SEED_IO_BIT6; /* Turn OFF bit 6 */
	in = in &  ~SEED_IO_BIT7; /* Turn OFF bit 7 */
    }
    return in;	
}

/* IGD 03/09/01
 * A front end to char set_byte_order_SEED_IO_BYTE(char)
 */

char process_set_byte_order_SEED_IO_BYTE(char myByte)	
{
#ifndef ENDIAN_LITTLE
    myByte = set_byte_order_SEED_IO_BYTE(myByte, SET_BIG_ENDIAN);
#else
#ifdef _BIG_ENDIAN_HEADER
    myByte = set_byte_order_SEED_IO_BYTE(myByte, SET_BIG_ENDIAN);
#else
    myByte = set_byte_order_SEED_IO_BYTE(myByte, SET_LITTLE_ENDIAN);
#endif
#endif
    return (myByte);
}

