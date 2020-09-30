/*$Id: comlink.c,v 1.2 2005/03/30 22:30:59 isti Exp $*/
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
                    Add LINUX #ifdefed byte swapping in gcrcinit() and
                    Add byte swapping for checksum()
                    Use a different version of gcrccalc () with LINUX
      13 Nov 99 IGD Modified for speed LINUX version of gcrccalc
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
		    Changed printf to LogMessage() calls.
 31   12 Mar 09 DSN Another fix for reference through NULL pointer for dead client.
 32   01 Dec 09 DCK Fix/add logging for corrupt data packet with odd length, set proper workorder for
		    blockettes in comments.
 33   01 Mar 2012 DSN Removed (again) the unneeded flip2 calls for blockette info for COMMENTS.
 34   24 Apr 2017 DSN Removed line terminator from LogMessage calls.
 35   29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#ifndef _OSK
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <stdlib.h>
#include <sgstat.h>
#include <sg_codes.h>
#include "os9inet.h"
#endif
#include "quanstrc.h"
#include "stuff.h"
#include "service.h"
#include "cfgutil.h"
#include "server.h"
#include "timeutil.h"
#include "logging.h"
#ifdef _OSK
#include "os9stuff.h"
#endif

#ifdef	LINUX
#include "unistd.h"
#endif

#ifndef FIRSTDATA       /* IGD 03/05/01 Add */
#define FIRSTDATA 56
#endif

#define SEED_IO_BIT6 0x40    /* IGD 03/09/01 */
#define SEED_IO_BIT7 0x80    /* Flags for placing the */				
#define SET_BIG_ENDIAN 1     /* swapping bits into I/O and clock flags byte */
#define SET_LITTLE_ENDIAN 0  /* of fixed SEED header */


short VER_COMLINK = 35 ;

extern seed_net_type network ;
extern complong station ;
int32_t reconfig_on_err = 25 ;
boolean anystation = FALSE ;

extern boolean noultra ;
extern pchar src, srcend, dest, destend, term ;
extern unsigned char sbuf[BLOB] ;
extern DA_to_DP_buffer dbuf ;
extern byte last_packet_received ;
extern byte lastchar ;
extern int path ;
extern int sockfd ;
extern struct sockaddr_in cli_addr ;
extern int upmemid ;
extern DP_to_DA_buffer temp_pkt ;
extern tcrc_table crctable ;
extern tclients clients[MAXCLIENTS] ;
extern pserver_struc base ;

extern byte inphase ;
extern byte upphase ;
extern int32_t seq ;
extern int32_t comm_mask ;
extern boolean verbose ;
extern boolean rambling ;
extern boolean insane ;
extern linkstat_rec linkstat ;
extern boolean detavail_ok ;
extern boolean detavail_loaded ;
extern boolean first ;
extern boolean firstpacket ;
extern boolean seq_valid ;
extern boolean xfer_down ;
extern boolean xfer_down_ok ;
extern boolean xfer_up_ok ;
extern boolean map_wait ;
extern boolean ultra_seg_empty ;
extern boolean override ;
extern boolean follow_up ;
extern boolean serial ;
extern boolean udplink ;
extern boolean notify ;
extern short ultra_percent ;
extern unsigned short lowest_seq ;
extern short linkpoll ;
extern int32_t con_seq ;
extern int32_t netdly_cnt ;
extern int32_t netdly ;
extern int32_t netto_cnt ;
extern int32_t grpsize ;
extern int32_t grptime ;
extern int32_t link_retry ;
extern double last_sent ;
extern short combusy ;
extern byte cmd_seq ;
extern link_record curlink ;
extern string59 xfer_destination ;
extern string59 xfer_source ;
extern byte ultra_seg[14] ;
extern byte detavail_seg[14] ;
extern seg_map_type xfer_seg ;
extern unsigned short seg_size ;
extern unsigned short xfer_size ;
extern unsigned short xfer_up_curseg ;
extern unsigned short xfer_bytes ;
extern unsigned short xfer_total ;
extern unsigned short xfer_last ;
extern unsigned short xfer_segments ;
extern unsigned short cal_size ;
extern unsigned short used_size ;
extern unsigned short ultra_size ;
extern unsigned short detavail_size ;
extern cal_record *pcal ;
extern short sequence_mod ;
extern short vcovalue ; 
extern short xfer_resends ;
extern short mappoll ;
extern short sincemap ;
extern short down_count ;
extern short minctr ;
extern short txwin ;
extern download_struc *pdownload ;     
extern ultra_type *pultra ;
extern tupbuf *pupbuf ;
DP_to_DA_msg_type replybuf ;
extern DP_to_DA_msg_type gmsg ;

extern string3 seed_names[20][7] ;
extern location_type seed_locs[20][7] ;

static byte this_packet ;

static int maxbytes = 0 ;

int32_t julian (time_array *gt) ;
pchar time_string (double jul) ;
tring_elem *getbuffer (short qnum) ;
boolean bufavail (short qnum) ;
boolean checkmask (short qnum) ;
double seedheader (header_type *h, seed_fixed_data_record_header *sh) ;
double seedblocks (seed_record_header *sl, commo_record *log) ;
signed char encode_rate (short rate) ;
void seedsequence (seed_record_header *seed, int32_t seq) ;
static short seq_seen[DEFAULT_WINDOW_SIZE] = { 0,0,0,0,0,0,0,0};
void flip_calibrators (char *);   /*IGD byte swapping routine*/
void flip_channels(char *, int);  /*IGD byte swapping routine for channels */
void flip_detectors(char *);      /* IGD Detectors byte-swapping */
void flip_fixed_header(seed_fixed_data_record_header *);
char set_byte_order_SEED_IO_BYTE(char, short); /* IGD 03/09/01 */
char process_set_byte_order_SEED_IO_BYTE(char);
typedef struct sockaddr *psockaddr ;

typedef struct
{
    byte cmd ;              /* command number */
    byte ack ;              /* acknowledgement of packet n */
    byte dpseq ;            /* sequence for DP to DA commands */
    char leadin ;           /* leadin character */
    short len ;             /* Length of message */
    DP_to_DA_msg_type buf ; /* buffer for the raw message */
} txbuf_type ;

static short nextin = 0 ;
static short nextout = 0 ;
static txbuf_type txbuf[64] ;

pchar seednamestring (seed_name_type *sd, location_type *loc) ;

void gcrcinit (void)
{
    short count, bits ;
    int32_t tdata, accum ;
	
    for (count = 0 ; count < 256 ; count++)
    {
	tdata = ((int32_t) count) << 24 ;
	accum = 0 ;
	for (bits = 1 ; bits <= 8 ; bits++)
	{
	    if ((tdata ^ accum) < 0)
		accum = (accum << 1) ^ CRC_POLYNOMIAL ;
	    else
		accum = (accum << 1) ;
	    tdata = tdata << 1 ;
	}
	    
	accum = flip4( accum ); /*IGD flip4 here */
	crctable[count] = accum ;
    }
}

#ifdef	ENDIAN_LITTLE
int32_t gcrccalc (pchar b, short len)
{
    complong crc ;
    int temp;
	
    crc.l = 0 ;
    while (len-- > 0)
    {
	temp = (crc.b[0] ^ *b++) & 255;
	crc.l = (crc.l >> 8) & (int32_t) 0xffffff;
	crc.l = crc.l ^ crctable[temp];
    } 	
    return crc.l ;
}
#else
int32_t gcrccalc (pchar b, short len)
{
    complong crc ;
	
    crc.l = 0 ;
    while (len-- > 0)
        crc.l = (crc.l << 8) ^ crctable[(crc.b[0] ^ *b++) & 255] ;
    return crc.l ;
}
#endif

unsigned short checksum (pchar addr, short size)
{
    unsigned short ck ;
	
    ck = 0 ;
    while (size-- > 0)
	ck = ck + ((short) *addr++ & 255) ;
    return(flip2(ck)); /*IGD flip2 here */
}

void send_window (void)
{
    short i, len ;
    int numwrit ;
    pchar ta, tstr ;
    char transmit_buf[2 * sizeof(DP_to_DA_buffer) + 2] ;
    complong ltemp ;
    byte b ;
    txbuf_type *cur ;

    while (txwin > 0)
    {
	cur = &txbuf[nextout] ;
	temp_pkt.c.cmd = cur->cmd ;
	temp_pkt.c.ack = cur->ack ;
	temp_pkt.msg = cur->buf ;
	temp_pkt.msg.scs.dp_seq = cur->dpseq ;
	transmit_buf[0] = cur->leadin ;
	len = cur->len ;
	ta = (pchar) ((uintptr_t) &temp_pkt + 6) ;
	temp_pkt.c.chksum = checksum(ta, len - 6) ;
	ltemp.l = gcrccalc(ta, len - 6) ;
	temp_pkt.c.crc = ltemp.s[0] ;
	temp_pkt.c.crc_low = ltemp.s[1] ;
	if (udplink) {
	    numwrit = sendto (path, (pchar) &temp_pkt, len, 0,
			      (psockaddr) &cli_addr, sizeof(cli_addr)) ;
	    if (insane) {
		char str[8192];
		pchar p = (pchar)&temp_pkt;
		int i = 0, l = 0;
		str[0] = 0;
		LogMessage(CS_LOG_TYPE_INFO, "numwrit = %d  len = %d", numwrit, len);
		for (i=0; i<len; i++) {
		    unsigned int ival;
		    ival = p[i];
		    sprintf (str+l, "%2x ", ival);
		    l += 3;
		}
		LogMessage(CS_LOG_TYPE_INFO, "packet = %s", str);
	    }
	}
	else
	{
	    tstr = &transmit_buf[1] ;
	    ta = (pchar) &temp_pkt ;
	    for (i = 0 ; i < len ; i++)
	    {
		b = (*ta >> 4) & 15 ;
		if (b > 9)
		    *tstr++ = b + 55 ;
		else
		    *tstr++ = b + 48 ;
		b = *ta++ & 15 ;
		if (b > 9)
		    *tstr++ = b + 55 ;
		else
		    *tstr++ = b + 48 ;
	    }
	}
	txwin-- ;
	if (insane)
	{
	    if (cur->cmd == ACK_MSG)
		LogMessage(CS_LOG_TYPE_INFO, "Acking packet %d from slot %d, packets queued=%d", cur->ack, nextout, txwin) ;
	    else
		LogMessage(CS_LOG_TYPE_INFO, "Sending command %d from slot %d, packets queued=%d", ord(cur->cmd), nextout, txwin) ;
	}
	nextout = (nextout + 1) & 63 ;
	if ((path >= 0) && !udplink)
	{
	    numwrit = write(path, (pchar) &transmit_buf, len * 2 + 1) ;
	    if ((numwrit < 0) && (verbose))
		perror ("Error writing to port ") ;
	}
	last_sent = dtime () ;
    }
}

void send_tx_packet (byte nr, byte cmd_var, DP_to_DA_msg_type *msg)
{
    txbuf_type *cur ;
	
    cur = &txbuf[nextin] ;
    if (cmd_var == ACK_MSG)
    {
	cur->cmd = cmd_var ;
	cur->ack = nr ;
	cur->leadin = LEADIN_ACKNAK ;
	cur->len = DP_TO_DA_LENGTH_ACKNAK ;
	cur->dpseq = 0 ;
    }
    else
    {
	cur->cmd = cmd_var ;
	cur->ack = last_packet_received ;
	cur->buf = *msg ;
	cur->dpseq = cmd_seq++ ;
	cur->len = DP_TO_DA_LENGTH_CMD ;
	cur->leadin = LEADIN_CMD ;
    } ;
    txwin++ ;
    if (insane)
	LogMessage(CS_LOG_TYPE_INFO, "Placing outgoing packet in slot %d, total in window=%d", nextin, txwin) ;
    nextin = (nextin + 1) & 63 ;
    if (txwin >= grpsize)
	send_window () ;
}

void send_ack (void)
{
    DP_to_DA_msg_type msg ;
	
    last_packet_received = this_packet ;
    send_tx_packet (this_packet, ACK_MSG, &msg) ;
}

void request_ultra (void)
{
    DP_to_DA_msg_type mmsg ;
	
    if (path < 0)
	return ;
    mmsg.scs.cmd_type = ULTRA_REQ ;
    send_tx_packet (0, ULTRA_REQ, &mmsg) ;
    if (rambling)
	LogMessage(CS_LOG_TYPE_INFO, "Requesting ultra packet") ;
}

void request_link (void)
{
    DP_to_DA_msg_type mmsg ;
	
    linkpoll = 0 ;
    if (path < 0)
	return ;
    mmsg.scs.cmd_type = LINK_REQ ;
    send_tx_packet (0, LINK_REQ, &mmsg) ;
    if (rambling)
	LogMessage(CS_LOG_TYPE_INFO, "Requesting link packet") ;
}

void request_map (void)
{
    DP_to_DA_msg_type mmsg ;
	
    mmsg.us.cmd_type = UPLOAD ;
    mmsg.us.dp_seq = cmd_seq ;
    mmsg.us.return_map = TRUE ;
    if (upphase == WAIT_CREATE_OK)
    {
	mmsg.us.upload_control = CREATE_UPLOAD ;
	mmsg.us.up_union.up_create.file_size = flip2(xfer_size) ; /*IGD flip2 here */
	memcpy (mmsg.us.up_union.up_create.file_name, xfer_destination, 60) ;
    }
    else
	mmsg.us.upload_control = MAP_ONLY ;
    send_tx_packet (0, UPLOAD, &mmsg) ;
    mappoll = 30 ;
}

void reconfigure (boolean full)
{
    short i ;
	
    if (full)
    {
	linkstat.linkrecv = FALSE ;
	seq_valid = FALSE ;
	lowest_seq = 300 ;
	con_seq = 0 ;
	linkpoll = 0 ;
	request_link () ;
    }
    if (linkstat.ultraon)
    {
	linkstat.ultrarecv = FALSE ;
	ultra_seg_empty = TRUE ;
	for (i = 0 ; i < 14 ; i++)
	    ultra_seg[i] = 0 ;
	if (ultra_size)
	{
	    free(pultra) ;
	    ultra_size = 0 ;
	}
	ultra_percent = 0 ;
	if (!full)
	    request_ultra () ;
    }
    else
	for (i = 0 ; i < DEFAULT_WINDOW_SIZE ; i++)
            seq_seen[i] = 0 ;
}

void clearmsg (DP_to_DA_msg_type *m)
{
    m->mmc.dp_seq = 0 ;
    m->mmc.cmd_parms.param0 = 0 ;
    m->mmc.cmd_parms.param1 = 0 ;
    m->mmc.cmd_parms.param2 = 0 ;
    m->mmc.cmd_parms.param3 = 0 ;
    m->mmc.cmd_parms.param4 = 0 ;
    m->mmc.cmd_parms.param5 = 0 ;
    m->mmc.cmd_parms.param6 = 0 ;
    m->mmc.cmd_parms.param7 = 0 ;
}

boolean checkbusy (void) /* returns TRUE if client is foreign or alive */
{
    return (combusy != NOCLIENT) &&
	((clients[combusy].client_address->client_uid != base->server_uid) ||
#ifdef _OSK
	 (kill(clients[combusy].client_pid, SIGWAKE) != ERROR)) ;
#else
    (kill(clients[combusy].client_pid, 0) != ERROR)) ;
#endif
}

void do_abort (void)
{
    DP_to_DA_msg_type mmsg ;
    comstat_rec *pcom ;
    download_result *pdr = NULL ;
	
    if (xfer_up_ok)
    {
	xfer_up_ok = FALSE ;
	clearmsg (&mmsg) ;
	mmsg.us.cmd_type = UPLOAD ;
	mmsg.us.dp_seq = cmd_seq ;
	mmsg.us.return_map = FALSE ;
	mmsg.us.upload_control = ABORT_UPLOAD ;
	send_tx_packet (0, UPLOAD, &mmsg) ;
	clearmsg (&mmsg) ;
	mmsg.us.cmd_type = UPLOAD ;
	mmsg.us.dp_seq = cmd_seq ;
	mmsg.us.return_map = FALSE ;
	mmsg.us.upload_control = ABORT_UPLOAD ;
	send_tx_packet (0, UPLOAD, &mmsg) ;
	shmdt((pchar)pupbuf) ;
	if (checkbusy ())
	{
	    pcom = (pvoid) clients[combusy].outbuf ;
	    if (upmemid != NOCLIENT)
	    {
		shmctl(upmemid, IPC_RMID, NULL) ;
		upmemid = NOCLIENT ;
	    }
	    if (pcom != NULL && pcom->completion_status == CSCS_INPROGRESS)
		pcom->completion_status = CSCS_ABORTED ;
	    combusy = NOCLIENT ;
	}
	xfer_size = 0 ;
    } ;
    if (xfer_down_ok)
    {
	xfer_down_ok = FALSE ;
	clearmsg (&mmsg) ;
	mmsg.scs.cmd_type = DOWN_ABT ;
	send_tx_packet (0, DOWN_ABT, &mmsg) ;
	clearmsg (&mmsg) ;
	mmsg.scs.cmd_type = DOWN_ABT ;
	send_tx_packet (0, DOWN_ABT, &mmsg) ;
	if (pdownload != NULL)
	    shmdt((pchar)pdownload) ;
	if (checkbusy ())
	{
	    pcom = (pvoid) clients[combusy].outbuf ;
	    if (pcom) pdr = (pvoid) &pcom->moreinfo ;
	    if (pdr != NULL && pdr->dpshmid != NOCLIENT)
	    {
		shmctl(pdr->dpshmid, IPC_RMID, NULL) ;
		pdr->dpshmid = NOCLIENT ;
	    }
	    if (pcom != NULL && pcom->completion_status == CSCS_INPROGRESS)
		pcom->completion_status = CSCS_ABORTED ;
	    combusy = NOCLIENT ;
	}
    }
}

void next_segment (void)
{
    DP_to_DA_msg_type mmsg ;
    unsigned short i ;
    pchar p1, p2 ;
    boolean allsent ;
    unsigned short off, cnt ;
    comstat_rec *pcom ;
    upload_result *pupres ;

    clearmsg (&mmsg) ;
    mmsg.scs.cmd_type = UPLOAD ;
    mmsg.us.return_map = FALSE ;
    mmsg.us.upload_control = SEND_UPLOAD ;
    if (checkbusy ())
    {
	pcom = (pvoid) clients[combusy].outbuf ;
	pupres = (pvoid) &pcom->moreinfo ;
    }
    else
    {
	do_abort () ;
	return ;
    }
    if (upphase == SENDING_UPLOAD)
    {
	i = xfer_up_curseg ;
	allsent = TRUE ;
	while (i < xfer_segments)
	{
	    if (((xfer_seg[i / 8]) & ((byte) (1 << (i % 8)))) == 0)
	    {
		allsent = FALSE ;
		off = seg_size * i ;
		mmsg.us.up_union.up_send.byte_offset = flip2(off) ;   /*IGD flip2 here*/
		xfer_up_curseg = i + 1 ;
		mmsg.us.up_union.up_send.seg_num = flip2( xfer_up_curseg) ; /*IGD flip2 here */
		if (((unsigned int) off + (unsigned int) seg_size) >= (unsigned int) xfer_size)
		{
		    cnt = xfer_size - off ;
		    xfer_up_curseg = 0 ;
		    mmsg.us.return_map = TRUE ;
		    mappoll = 30 ;
		    sincemap = 0 ;
		    upphase = WAIT_MAP ;
		    if (xfer_resends < 0)
			xfer_resends = 0 ;
		}
		else
		{
		    cnt = seg_size ;
		    if (++sincemap >= 10)
		    {
			mmsg.us.return_map = TRUE ;
			mappoll = 30 ;
			sincemap = 0 ;
			upphase = WAIT_MAP ;
		    }
		} ;
		mmsg.us.up_union.up_send.byte_count = flip2(cnt) ; /*IGD flip2 here*/
		p1 = (pchar) ((uintptr_t) pupbuf + off) ;
		p2 = (pchar) &mmsg.us.up_union.up_send.bytes ;
		memcpy(p2, p1, cnt) ;
		xfer_bytes = xfer_bytes + cnt ;
		if (xfer_bytes > xfer_total)
		    xfer_bytes = xfer_total ;
		if (xfer_resends >= 0)
		    xfer_resends++ ;
		pupres->bytecount = xfer_bytes ;
		pupres->retries = xfer_resends ;
		send_tx_packet (0, UPLOAD, &mmsg) ;
		break ;
	    }
	    i++ ;
	}
	if (allsent)
	    for (i = 0 ; i < xfer_segments ; i++)
		if (((xfer_seg[i / 8]) & ((byte)(1 << (i % 8)))) == 0)
		{
		    xfer_up_curseg = 0 ;
		    upphase = WAIT_MAP ;
		    request_map () ;
		    return ;
		}
	if (allsent)
	{
	    xfer_up_ok = FALSE ;
	    upphase = UP_IDLE ;
	    mmsg.us.upload_control = UPLOAD_DONE ;
	    xfer_size = 0 ;
	    shmdt((pchar)pupbuf) ;
	    pcom->completion_status = CSCS_FINISHED ;
	    combusy = NOCLIENT ;
	    send_tx_packet (0, UPLOAD, &mmsg) ;
	}
    }
}

void process_upmap (void)
{
    comstat_rec *pcom ;
	
    mappoll = 0 ;
    switch (upphase)
    {
    case WAIT_CREATE_OK :
    {
	if (dbuf.data_buf.cu.upload_ok)
	{
	    upphase = SENDING_UPLOAD ;
	    memcpy ((pchar) &xfer_seg, (pchar) &dbuf.data_buf.cu.segmap, sizeof(seg_map_type)) ;
	}
	else
	{
	    upphase = UP_IDLE ;
	    xfer_up_ok = FALSE ;
	    shmdt((pchar)pupbuf) ;
	    if (checkbusy ())
	    {
		pcom = (pvoid) clients[combusy].outbuf ;
		if (upmemid != NOCLIENT)
		{
		    shmctl(upmemid, IPC_RMID, NULL) ;
		    upmemid = NOCLIENT ;
		}
		if (pcom->completion_status == CSCS_INPROGRESS)
		    pcom->completion_status = CSCS_CANT ;
		combusy = NOCLIENT ;
	    }
	    xfer_size = 0 ;
	}
	break ;
    }
    case WAIT_MAP :
    {
	upphase = SENDING_UPLOAD ;
	memcpy ((pchar) &xfer_seg, (pchar) &dbuf.data_buf.cu.segmap, sizeof(seg_map_type)) ;
    }
    }
}

void setseed (byte cmp, byte str, seed_name_type *seed, location_type *loc)
{
    memcpy((pchar) seed, (pchar) &seed_names[cmp][str], 3) ;
    memcpy((pchar) loc, (pchar) &seed_locs[cmp][str], 2) ;
}
	
double seed_jul (seed_time_struc *st)
{
    double t ;
    t = jconv (flip2(st->yr) - 1900, flip2(st->jday)) ;  /*IGD 03/04/01 flip2 */
    t = t + (double) st->hr * 3600.0 + (double) st->minute * 60.0 +
	(double) st->seconds + (double) flip2(st->tenth_millisec) * 0.0001 ; /* IGD 03/04/01 flip2 */
    return t ;
}
	
void process (void)
{
    typedef char string5[6] ;
    typedef block_record *tpbr ;

    char rcelu[2] = {'N', 'Y'} ;
    string5 lf[2] = {"QSL", "Q512"} ;
    short dp_sum, i, j, l ;
    int32_t dp_crc ;
    boolean full ;
    byte k ;
    pchar ta ;
    short size ;
    unsigned short bc, xfer_offset ;
    typedef error_control_packet *tecp ;
    tecp pecp ;
    pchar p1, p2 ;
    time_quality_descriptor *ptqd ;
    calibration_result *pcal ;
    clock_exception *pce ;
    squeezed_event_detection_report *ppick ;
    commo_reply *preply ;
    complong ltemp ;
    tring_elem *freebuf ;
    seed_fixed_data_record_header *pseed ;
    comstat_rec *pcr, *pcom ;
    download_result *pdr ;
    det_request_rec *pdetavail ;
    download_struc *pds ;
    tpbr pbr ;
    int32_t *pl ;
    char seedst[5] ;
    char s1[64], s2[64] ;
#ifdef	ENDIAN_LITTLE
    pchar tmpa;          /* IGD tmp byte-swapping array */
    timing * tmp_timing; /* IGD 03/05/01 tmp Byte-swapping pointer for blockette 500 */
    threshold_detect *dett;	
    step_calibration *calstep ;
    sine_calibration *calsine ;
    random_calibration *calrand ;
    abort_calibration *calabort ;
#endif
    dest = (pchar) &dbuf.seq ; /* reset buffer pointer */
    linkstat.total_packets++ ;
    size = (short) ((uintptr_t) term - (uintptr_t) dest - 6) ;
    if ((size & 1) == 0) /* SUN gets pissed if size is odd */
    {
	pecp = (tecp) term ;
	pecp = (tecp) ((uintptr_t) pecp - 6) ;
	dp_sum = checksum(dest, size) ;
	dp_crc = gcrccalc(dest, size) ;
	ltemp.s[0] = pecp->crc ;
	ltemp.s[1] = pecp->crc_low ;
    }
    else
	pecp = NULL ;

    if (verbose)
    {
	if (pecp == NULL)
	    LogMessage(CS_LOG_TYPE_ERROR, "TYPE=%d sq=%d Packet size=%d",
		       dbuf.data_buf.cl.frame_type,dbuf.seq,size);
	else
	    LogMessage(CS_LOG_TYPE_INFO, 
		       "TYPE=%d sq=%d Packet size=%d%s pecp=%p dpsum=%d chksum=%d%s ltemp.l=%d dpcrc=%d%s",
		       dbuf.data_buf.cl.frame_type,dbuf.seq,size, (size==514?" ":"***"),(uintptr_t) pecp, 
		       dp_sum, pecp->chksum,(dp_sum == pecp->chksum?" ":" ***"),
		       ltemp.l, dp_crc,(ltemp.l==dp_crc?" ":" ***")); /* DCK DEBUG */
    }
    if ((pecp != NULL) && (dp_sum == pecp->chksum) && (ltemp.l == dp_crc))
    {
	netto_cnt = 0 ; /* got a packet, reset timeout */
	linkstat.last_good = dtime () ;
	this_packet = dbuf.seq ;
	if (size < 514)
	{
	    ta = (pchar) ((uintptr_t) dest + size) ;
	    memset (ta, '\0', 514 - size) ;
	}
	if (linkstat.ultraon)
	{
	    switch (dbuf.ctrl)
	    {
	    case SEQUENCE_SPECIAL :
	    {
		if (dbuf.data_buf.cl.frame_type == LINK_PKT)
		{
		    linkstat.linkrecv = TRUE ;
		    seq_valid = FALSE ;
		    last_packet_received = dbuf.seq ;	
		    sequence_mod = flip2(dbuf.data_buf.cl.seq_modulus) ;  /*IGD flip2 here */
		    curlink.msg_prio = dbuf.data_buf.cl.msg_prio ;
		    curlink.det_prio = dbuf.data_buf.cl.det_prio ;
		    curlink.time_prio = dbuf.data_buf.cl.time_prio ;
		    curlink.cal_prio = dbuf.data_buf.cl.cal_prio ;
		    curlink.window_size = flip2(dbuf.data_buf.cl.window_size) ; /*IGD flip2 here: assign char to short !*/
		    curlink.total_prio = dbuf.data_buf.cl.total_prio ;          /*IGD flips in curlink -> OK/ */
		    curlink.link_format = dbuf.data_buf.cl.link_format ;
		    linkstat.data_format = curlink.link_format ;
		    curlink.rcecho = dbuf.data_buf.cl.rcecho ;
		    curlink.resendtime = flip2(dbuf.data_buf.cl.resendtime) ;
		    curlink.synctime = flip2(dbuf.data_buf.cl.synctime) ;
		    curlink.resendpkts = flip2(dbuf.data_buf.cl.resendpkts) ;
		    curlink.netdelay = flip2(dbuf.data_buf.cl.netdelay) ;
		    curlink.nettime = flip2(dbuf.data_buf.cl.nettime) ;
		    curlink.netmax = flip2(dbuf.data_buf.cl.netmax) ;
		    curlink.groupsize = flip2(dbuf.data_buf.cl.groupsize) ;
		    curlink.grouptime = flip2(dbuf.data_buf.cl.grouptime) ;
		    if (verbose)
		    {
			LogMessage(CS_LOG_TYPE_INFO, "Window=%d  Modulus=%d  Start=%d  RC Echo=%c  Data Format=%s",
				   curlink.window_size, sequence_mod, last_packet_received,
				   rcelu[curlink.rcecho], lf[linkstat.data_format]) ;

			LogMessage(CS_LOG_TYPE_INFO, "Total Levels=%d  Msg=%d  Det=%d  Time=%d  Cal=%d  Sync Time=%d",
				   curlink.total_prio - 1, curlink.msg_prio, curlink.det_prio,
				   curlink.time_prio, curlink.cal_prio, curlink.synctime) ;
			LogMessage(CS_LOG_TYPE_INFO, "Resend Time=%d  Resend Pkts=%d  Group Pkt Size=%d  Group Timeout=%d",
				   curlink.resendtime, curlink.resendpkts,
				   curlink.groupsize, curlink.grouptime) ;
			LogMessage(CS_LOG_TYPE_INFO, "Net Restart Dly=%d  Net Conn. Time=%d  Net Packet Limit=%d",
				   curlink.netdelay, curlink.nettime, curlink.netmax) ;
		    }
		    reconfigure (FALSE) ;
		}
		return ;
	    }
	    case SEQUENCE_INCREMENT : ;
	    case SEQUENCE_RESET :
	    {
		if (seq_valid)
		{
		    if (dbuf.seq != (byte) ((((unsigned int) last_packet_received) + 1) % (unsigned int) sequence_mod))
		    {
			if (checkmask (-1))
			{
			    if (verbose)
				LogMessage(CS_LOG_TYPE_INFO, "Packet Toss-Checkmask") ;
			    return ;
			}
			linkstat.seq_errors++ ;
			if (verbose)
			    LogMessage(CS_LOG_TYPE_INFO, "SEQ EXP=%d  GOT=%d",
				       ((unsigned int) last_packet_received + 1) % (unsigned int) sequence_mod, dbuf.seq) ;
			this_packet = last_packet_received ;
			send_ack () ;
			if (++con_seq > reconfig_on_err && reconfig_on_err > 0)
			{
			    LogMessage(CS_LOG_TYPE_INFO, "%s reconfigure due to sequence errors",
				       localtime_string(dtime())) ;
			    reconfigure (TRUE) ;
			}
			return ;
		    }
		    else
		    {
			if (insane)
			    LogMessage(CS_LOG_TYPE_INFO, "Sequence Correct") ;
			con_seq = 0 ;
		    }
		}
		else if (linkstat.linkrecv && (dbuf.seq == last_packet_received))
		{
		    if (insane)
			LogMessage(CS_LOG_TYPE_INFO, "Sequence Now Valid");
		    seq_valid = TRUE ;
		}
		else
		{
		    if (verbose)
			LogMessage(CS_LOG_TYPE_INFO, "Sequence Not Yet Valid") ;
		    if (++con_seq > reconfig_on_err && reconfig_on_err > 0)
		    {
			LogMessage(CS_LOG_TYPE_INFO, "%s reconfigure due to sequence not yet valid",
				   localtime_string(dtime())) ;
			reconfigure (TRUE) ;
		    }
		    return ;
		}
	    }
	    }
	}
	else /* old system */
	{
	    if (seq_valid)
	    {
		if (dbuf.seq != (byte) ((((unsigned int) last_packet_received) + 1) % (unsigned int) sequence_mod))
		{
		    if (checkmask (-1))
			return ;
		    linkstat.seq_errors++ ;
		    if (verbose)
			LogMessage(CS_LOG_TYPE_INFO, "SEQ EXP=%d  GOT=%d",
				   ((unsigned int) last_packet_received + 1) % (unsigned int) sequence_mod, dbuf.seq) ;
		    this_packet = last_packet_received ;
		    send_ack () ;
		    if (++con_seq > reconfig_on_err && reconfig_on_err > 0)
		    {
			LogMessage(CS_LOG_TYPE_INFO, "reconfigure due to sequence errors");
			reconfigure (TRUE) ;
		    }
		    return ;
		}
		else
		    con_seq = 0 ;
	    }
	    else if ((unsigned short) dbuf.seq == lowest_seq)
	    {
		seq_valid = TRUE ;
		if (verbose)
		    LogMessage(CS_LOG_TYPE_INFO, "Start=%d", this_packet) ;
	    }
	    else
	    {
		/*:: Mark all sequence numbers that we have seen.  */
		/*:: When we have seen a sequence number twice, */
		/*:: determine the first of the sequence that we should accept. */
		int i ;
	
		if (dbuf.seq < DEFAULT_WINDOW_SIZE)
		{
		    if (seq_seen[dbuf.seq] > 0)
		    {
			int prev_low = -1 ;
			int cur_low = -1 ;
			int looking_for_start = 1 ;
			for (i = 0 ; i < DEFAULT_WINDOW_SIZE ; i++)
			{
			    if ((!seq_seen[i]) && (!looking_for_start))
			    {
				prev_low = cur_low ;
				cur_low = -1 ;
				looking_for_start = 1 ;
			    }
			    else if (looking_for_start && seq_seen[i])
			    {
				cur_low = i ;
				looking_for_start = 0 ;
			    }
			}
			if (cur_low < 0)
			    cur_low = prev_low ;
			if (cur_low >= 0)
			    lowest_seq = cur_low ;
		    }
		    else seq_seen[dbuf.seq] = 1 ;
		}
		return ;
	    }
	}
	switch (dbuf.data_buf.cr.h.frame_type)
	{
	case RECORD_HEADER_1 : ;
	case RECORD_HEADER_2 : ;
	case RECORD_HEADER_3 :
	{
	    if (linkstat.data_format == CSF_Q512)
		dbuf.data_buf.cr.h.frame_count = 7 ;
	    if (checkmask (DATAQ))
		return ;
	    else
		send_ack () ;
	    if (!linkstat.ultraon)
	    {
		setseed(dbuf.data_buf.cr.h.component, dbuf.data_buf.cr.h.stream,
			&dbuf.data_buf.cr.h.seedname, &dbuf.data_buf.cr.h.location) ;
		memcpy((pchar) &dbuf.data_buf.cr.h.seednet, (pchar) &network, 2) ;
	    }
	    if (override)
		memcpy((pchar) &dbuf.data_buf.cr.h.station, (pchar) &station, 4) ;
	    else if ((!anystation) && (memcmp((pchar) &dbuf.data_buf.cr.h.station, (pchar) &station, 4) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s data received instead of %4.4s",
                           &dbuf.data_buf.cr.h.station, &station) ;
	    if (firstpacket)
	    {
		firstpacket = FALSE ;
		if (linkstat.ultraon)
		{
		    if (!linkstat.linkrecv)
			request_link () ;
		}
	    }
	    /* Unless an option is specified to override the station, an error should
	       be generated if the station does not agree. Same goes for network.
	    */

	    /* put into data buffer ring */
	    freebuf = getbuffer (DATAQ) ;                  /* get free buffer */
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    freebuf->user_data.reception_time = dtime () ;    /* reception time */
	    freebuf->user_data.header_time = seedheader (&dbuf.data_buf.cr.h, pseed) ; /* convert header to SEED */
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#ifdef	_BIG_ENDIAN_HEADER	/* IGD 03/03/01 */
	    /*
	     * If a user requested to store an MSEED header in big-endian byte order on little-endian
	     * system, we are going to swap back some variables to big-endian order
	     *	IGD 03/02-09/01
	     */
	    flip_fixed_header(pseed);	   	
	    pseed->deb.blockette_type = flip2(pseed->deb.blockette_type);
	    pseed->deb.next_offset  = flip2(pseed->deb.next_offset);
#endif
#endif
		
	    if (rambling)
	    {
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.%s Header time %s, received at %s", 
			   &dbuf.data_buf.cr.h.station,
			   seednamestring(&dbuf.data_buf.cr.h.seedname, &dbuf.data_buf.cr.h.location),
			   time_string(freebuf->user_data.header_time),
			   time_string(freebuf->user_data.reception_time)) ;
	    }
	    p1 = (pvoid) ((uintptr_t) pseed + 64) ;             /* skip header */
	    memcpy (p1, (pchar) &dbuf.data_buf.cr.frames, 448) ;   /* and copy data portion */	
	    break ;
	}
	case BLOCKETTE :
	{
	    if (checkmask (BLKQ))
		return ;
	    else
		send_ack () ;
	    pbr = (tpbr) &dbuf.data_buf ;
	    strcpy ((pchar)&seedst, "     ") ; /* initialize to spaces */
	    j = 0 ;
	    for (i = 0 ; i <= 3 ; i++)
		if (station.b[i] != ' ')
		    seedst[j++] = station.b[i] ; /* move in non space characters */
	    if (override)
		memcpy((pchar) &(pbr->hdr.station_ID_call_letters), (pchar) &seedst, 5) ;
	    else if ((!anystation) &&
		     (memcmp((pchar) &(pbr->hdr.station_ID_call_letters), (pchar) &seedst, 5) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s data received instead of %4.4s",
                           &(pbr->hdr.station_ID_call_letters), &station) ;
	    /* Unless an option is specified to override the station, an error should
	       be generated if the station does not agree. Same goes for network.
	    */
	    /* put into data buffer ring */
	    freebuf = getbuffer (BLKQ) ;                  /* get free buffer */
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    freebuf->user_data.reception_time = dtime () ;    /* reception time */
	    freebuf->user_data.header_time = seed_jul (&pbr->hdr.starting_time) ; /* convert SEED time to julian */
	    if (rambling)
	    {
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.%s Header time %s, received at %s", 
			   (pchar) &(pbr->hdr.station_ID_call_letters),
			   seednamestring(&(pbr->hdr.channel_id), &(pbr->hdr.location_id)),
			   time_string(freebuf->user_data.header_time),
			   time_string(freebuf->user_data.reception_time)) ;
	    }
	    pl = (pvoid) &pbr->hdr ;
	    seedsequence (&pbr->hdr, flip4(*pl) ) ; /*IGD 03/05/01 flip4 */
	    pseed = (pvoid) pbr;
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#ifndef _BIG_ENDIAN_HEADER
	    /* IGD 03/05/01
	     * If this code is executed on little-endian processor,
	     * pbr->hdr structure here is all BIG_ENDIAN.
	     * We will byte-swap some members of pbr structure if:
	     * 1) The program is compiled with -DENDIAN_LITTLE flag;
	     * 2) It is not compiled with -D_BIG_ENDIAN_HEADER compilation flag
	     * Note that the check for -DENDIAN_LITTLE flag is done inside flip2()/flip4()
	     */
	    flip_fixed_header(pseed);	   						
	    pbr->bmin.data_offset = flip2(pbr->bmin.data_offset);
	    pbr->bmin.record_num = flip4(pbr->bmin.record_num);
#endif
#endif
	    memcpy ((pchar) &freebuf->user_data.data_bytes, (pchar) pbr, 512) ; /* copy record into buffer */
	    break ;
	}
	case EMPTY : ;
	case FLOOD_PKT :
	    if (checkmask (-1))
		return ;
	    else
	    {
		send_ack () ;
		linkstat.sync_packets++ ;
		break ;
	    }
	case COMMENTS :
	{
	    if (checkmask (MSGQ))
		return ;
	    else
		send_ack () ;
	    if (linkstat.ultraon)
	    {
		p1 = (pchar) &dbuf.data_buf.cc.ct ;
		p1 = (pchar) ((uintptr_t) p1 + (uintptr_t) *p1 + 1) ;
		p2 = (pchar) &dbuf.data_buf.cc.cc_station ;
		memcpy(p2, p1, sizeof(uintptr_t) + sizeof(seed_net_type) +
		       sizeof(location_type) + sizeof(seed_name_type)) ;
	    }
	    if (!linkstat.ultraon)
	    {
		memcpy((pchar) &dbuf.data_buf.cc.cc_station, (pchar) &station, 4) ;
		memcpy((pchar) &dbuf.data_buf.cc.cc_net, (pchar) &network, 2) ;
	    }
	    if (override)
		memcpy((pchar) &dbuf.data_buf.cc.cc_station, (pchar) &station, 4) ;
	    else if ((!anystation) && (memcmp((pchar) &dbuf.data_buf.cc.cc_station, (pchar) &station, 4) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s message received instead of %4.4s",
                           &dbuf.data_buf.cc.cc_station, &station) ;
	    if (linkstat.ultraon && (!linkstat.ultrarecv) &&
		(strncasecmp((pchar) &dbuf.data_buf.cc.ct, "FROM AQSAMPLE: Acquisition begun", 32) == 0))
		request_ultra () ;
	    /* Put into blockette ring */
	    if (rambling)
	    {
		dbuf.data_buf.cc.ct[dbuf.data_buf.cc.ct[0]+1] = '\0' ;
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.%s", &dbuf.data_buf.cc.cc_station,
			   &dbuf.data_buf.cc.ct[1]) ;
	    } ;
	    freebuf = getbuffer (MSGQ) ;
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    freebuf->user_data.reception_time = dtime () ;    /* reception time */
	    freebuf->user_data.header_time = seedblocks ((pvoid) pseed, &dbuf.data_buf) ;
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#if (defined _BIG_ENDIAN_HEADER)
	    /* IGD 03/05/01
	     * If this code is executed on little-endian processor, the comments
	     * from the previous case are applicable o what we do here
	     */			
	    flip_fixed_header(pseed);	
#endif
#endif

	    break ;
	}
	case CLOCK_CORRECTION :
	{
	    if (checkmask (TIMQ))
		return ;
	    else
		send_ack () ;
	    if (vcovalue < 0)
            {
		if (linkstat.ultraon)
		    vcovalue = flip2(dbuf.data_buf.ce.header_elog.clk_exc.vco) ; /*IGD 03/05/01 flip2 */
		else
		{
		    ptqd = (pvoid) &dbuf.data_buf.ce.header_elog.clk_exc.correction_quality ;
		    vcovalue = (unsigned short) ptqd->time_base_VCO_correction * 16 ;
		}
	    }
	    pce = &dbuf.data_buf.ce.header_elog.clk_exc ;
	    if (!linkstat.ultraon)
	    {
		memcpy((pchar) &pce->cl_station, (pchar) &station, 4) ;
		memcpy((pchar) &pce->cl_net, (pchar) &network, 2) ;
	    }
	    if (override)
		memcpy((pchar) &pce->cl_station, (pchar) &station, 4) ;
	    else if ((!anystation) && (memcmp((pchar) &pce->cl_station, (pchar) &station, 4) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s timing received instead of %4.4s",
                           &pce->cl_station, &station) ;
	    /* put into blockette ring */
	    freebuf = getbuffer (TIMQ) ;
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    freebuf->user_data.reception_time = dtime () ;    /* reception time */
	    freebuf->user_data.header_time = seedblocks ((pvoid) pseed, &dbuf.data_buf) ;
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#if (defined _BIG_ENDIAN_HEADER)			
	    /* IGD 03/05/01
	     * If this code is executed on little-endian processor, the comments
	     * from the previous case are applicable o what we do here
	     */			
	    flip_fixed_header(pseed);	
	    tmp_timing = (pvoid) ((uintptr_t) pseed + FIRSTDATA);    /* IGD 03/05/01 extract blockette 500
								      * for byte-swapping
								      */
	    tmp_timing->blockette_type = flip2(tmp_timing->blockette_type);
	    tmp_timing->next_blockette = flip2(tmp_timing->next_blockette);
	    tmp_timing->vco_correction = flip_float(tmp_timing->vco_correction);
	    tmp_timing->time_of_exception.yr = flip2(tmp_timing->time_of_exception.yr);
	    tmp_timing->time_of_exception.jday = flip2(tmp_timing->time_of_exception.jday);
	    tmp_timing->time_of_exception.tenth_millisec =
		flip2(tmp_timing->time_of_exception.tenth_millisec);
	    tmp_timing->exception_count = flip4(tmp_timing->exception_count);
#endif
#endif
	    if (rambling)
	    {
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.Clock time-mark %s, received at %s", &pce->cl_station,
			   time_string(freebuf->user_data.header_time), 
			   time_string(freebuf->user_data.reception_time)) ;
	    }
	    break ;
	}
	case DETECTION_RESULT :
	{
	    if (checkmask (DETQ))
		return ;
	    else
		send_ack () ;
	    ppick = &dbuf.data_buf.ce.header_elog.det_res.pick ;
	    if (!linkstat.ultraon)
	    {
		setseed (ppick->component, ppick->stream,
			 &ppick->seedname, &ppick->location) ;
		memset((pchar) &ppick->detname, ' ', 24) ;
		ppick->sedr_sp1 = 0 ;
		memcpy((pchar) &ppick->ev_station, (pchar) &station, 4) ;
		memcpy((pchar) &ppick->ev_network, (pchar) &network, 2) ;
	    }
	    if (override)
		memcpy((pchar) &ppick->ev_station, (pchar) &station, 4) ;
	    else if ((!anystation) && (memcmp((pchar) &ppick->ev_station, (pchar) &station, 4) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s detection received instead of %4.4s",
                           &ppick->ev_station, &station) ;
	    /* put into blockette ring */
	    freebuf = getbuffer (DETQ) ;
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    freebuf->user_data.reception_time = dtime () ;
	    freebuf->user_data.header_time = seedblocks ((pvoid) pseed, &dbuf.data_buf) ;
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#if (defined _BIG_ENDIAN_HEADER)			
	    /* IGD 03/06/01
	     * If this code is executed on little-endian processor, the comments
	     * from the previous cases are applicable o what we do here
	     */			
	    flip_fixed_header(pseed);	

	    /* IGD 03/06/01 Note that it is irrelevent for byte-swapping if
	     * the actual detector blockete is 200 or 201. We cast it as 200 here
	     */	
	    dett = (pvoid) ((uintptr_t) pseed + FIRSTDATA);

	    dett = (pvoid) ((uintptr_t) pseed + FIRSTDATA);
	    dett->blockette_type = flip2(dett->blockette_type);
	    dett->next_blockette = flip2(dett->next_blockette);
	    dett->signal_amplitude = flip_float(dett->signal_amplitude);
	    dett->signal_period = flip_float (dett->signal_period);
	    dett->background_estimate = flip_float(dett->background_estimate);
	    dett->signal_onset_time.yr = flip2(dett->signal_onset_time.yr);
	    dett->signal_onset_time.jday = flip2(dett->signal_onset_time.jday);
	    dett->signal_onset_time.tenth_millisec = flip2(dett->signal_onset_time.tenth_millisec);
#endif
#endif

	    if (rambling)
	    {
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.%3.3s Detection   %s, received at %s", 
			   &ppick->ev_station,
			   &dbuf.data_buf.ce.header_elog.det_res.pick.seedname,
			   time_string(freebuf->user_data.header_time),
			   time_string(freebuf->user_data.reception_time)) ;
	    }
	    break ;
	}
	case END_OF_DETECTION :
	{
	    if (checkmask (DATAQ))
		return ;
	    else
		send_ack () ;
	    freebuf = getbuffer (DATAQ) ;                  /* get free buffer */
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    ppick = &dbuf.data_buf.ce.header_elog.det_res.pick ;
	    if (!linkstat.ultraon)
	    {
		setseed (ppick->component, ppick->stream,
			 &ppick->seedname, &ppick->location) ;
		ppick->sedr_sp1 = 0 ;
		memcpy((pchar) &ppick->ev_station, (pchar) &station, 4) ;
		memcpy((pchar) &ppick->ev_network, (pchar) &network, 2) ;
	    }
	    if (override)
		memcpy((pchar) &ppick->ev_station, (pchar) &station, 4) ;
	    else if ((!anystation) && (memcmp((pchar) &ppick->ev_station, (pchar) &station, 4) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s end of detection received instead of %4.4s",
                           &ppick->ev_station, &station) ;
	    freebuf->user_data.reception_time = dtime () ;    /* reception time */
	    freebuf->user_data.header_time = seedblocks ((pvoid) pseed, &dbuf.data_buf) ;
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#if (defined _BIG_ENDIAN_HEADER)			
	    /* IGD 03/06/01
	     * If this code is executed on little-endian processor, the comments
	     * from the previous cases are applicable o what we do here
	     */			
	    flip_fixed_header(pseed);	
#endif
#endif

	    if (rambling)
	    {
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.%3s Detect End  %s, received at %s", 
			   &ppick->ev_station,
			   &dbuf.data_buf.ce.header_elog.det_res.pick.seedname,
			   time_string(freebuf->user_data.header_time),
			   time_string(freebuf->user_data.reception_time)) ;
	    }
	    break ;
	}
	case CALIBRATION :
	{
	    if (checkmask (CALQ))
		return ;
	    else
		send_ack () ;
	    pcal = &dbuf.data_buf.ce.header_elog.cal_res ;
	    if (!linkstat.ultraon)
	    {
		setseed (pcal->cr_component, pcal->cr_stream,
			 &pcal->cr_seedname, &pcal->cr_location) ;
		setseed (pcal->cr_input_comp, pcal->cr_input_strm,
			 &pcal->cr_input_seedname, &pcal->cr_input_location) ;
		pcal->cr_flags2 = 0 ;
		pcal->cr_0dB = 0 ;
		pcal->cr_0dB_low = 0 ;
		pcal->cr_sfrq = Hz1_0000 ;
		pcal->cr_filt = 0 ;
		memcpy ((pchar) &pcal->cr_station, (pchar) &station, 4) ;
		memcpy ((pchar) &pcal->cr_network, (pchar) &network, 2) ;
	    }
	    if (override)
		memcpy((pchar) &pcal->cr_station, (pchar) &station, 4) ;
	    else if ((!anystation) && (memcmp((pchar) &pcal->cr_station, (pchar) &station, 4) != 0))
		LogMessage(CS_LOG_TYPE_INFO, "Station %4.4s calibration received instead of %4.4s",
                           &pcal->cr_station, &station) ;
	    /* store in blockette ring */
	    freebuf = getbuffer (CALQ) ;
	    pseed = (pvoid) &freebuf->user_data.data_bytes ;
	    freebuf->user_data.reception_time = dtime () ;    /* reception time */
	    freebuf->user_data.header_time = seedblocks ((pvoid) pseed, &dbuf.data_buf) ;
	    pseed->header.IO_flags = process_set_byte_order_SEED_IO_BYTE(pseed->header.IO_flags); /* IGD 03/09/01 */
#ifdef	ENDIAN_LITTLE
#if (defined _BIG_ENDIAN_HEADER)			
	    /* IGD 03/07/01
	     * If this code is executed on little-endian processor, the comments
	     * from the previous cases are applicable o what we do here
	     */			
	    flip_fixed_header(pseed);	
	    switch (pseed->deb.blockette_type)
	    {
	    case 300 :  /* STEP_CALIBRATION */
	    {
		calstep = (pvoid) ((uintptr_t) pseed + FIRSTDATA);
		calstep->fixed.blockette_type = flip2(calstep->fixed.blockette_type);
		calstep->fixed.next_blockette = flip2(calstep->fixed.next_blockette);
		calstep->fixed.calibration_time.yr = flip2(calstep->fixed.calibration_time.yr);
		calstep->fixed.calibration_time.jday = flip2(calstep->fixed.calibration_time.jday);
		calstep->fixed.calibration_time.tenth_millisec
		    = flip2(calstep->fixed.calibration_time.tenth_millisec);
		calstep->calibration_duration = flip4(calstep->calibration_duration);
		calstep->interval_duration = flip4(calstep->interval_duration);
		calstep->step2.calibration_amplitude = flip_float(calstep->step2.calibration_amplitude);
		calstep->step2.ref_amp = flip_float(calstep->step2.ref_amp); 	
		break ;
	    }
	    case 310 :  /* SINE_CALIBRATION */
	    {
		calsine = (pvoid) ((uintptr_t) pseed + FIRSTDATA);
		calsine->fixed.blockette_type = flip2(calsine->fixed.blockette_type);
		calsine->fixed.next_blockette = flip2(calsine->fixed.next_blockette);
		calsine->fixed.calibration_time.yr = flip2(calsine->fixed.calibration_time.yr);
		calsine->fixed.calibration_time.jday = flip2(calsine->fixed.calibration_time.jday);
		calsine->fixed.calibration_time.tenth_millisec =
		    flip2(calsine->fixed.calibration_time.tenth_millisec);
		calsine->calibration_duration = flip4(calsine->calibration_duration);
		calsine->sine_period = flip_float(calsine->sine_period);
		calsine->sine2.calibration_amplitude = flip_float(calsine->sine2.calibration_amplitude);
		calsine->sine2.ref_amp = flip_float(calsine->sine2.ref_amp);
		break ;
	    }
	    case 320 :  /* RANDOM_CALIBRATION */
	    {
		calrand = (pvoid) ((uintptr_t) pseed + FIRSTDATA);
		calrand->fixed.blockette_type = flip2(calrand->fixed.blockette_type);
		calrand->fixed.next_blockette = flip2(calrand->fixed.next_blockette);
		calrand->fixed.calibration_time.yr = flip2(calrand->fixed.calibration_time.yr);
		calrand->fixed.calibration_time.jday = flip2(calrand->fixed.calibration_time.jday);
		calrand->fixed.calibration_time.tenth_millisec =
		    flip2(calrand->fixed.calibration_time.tenth_millisec);
		calrand->calibration_duration = flip4(calrand->calibration_duration);
		calrand->random2.calibration_amplitude = flip_float(calrand->random2.calibration_amplitude);
		calrand->random2.ref_amp = flip_float(calrand->random2.ref_amp);

		break ;
	    } ;
	    case 395 :  /* ABORT_CALIBRATION */
	    {
		calabort = (pvoid) ((uintptr_t) pseed + FIRSTDATA);
		calabort = (pvoid) ((uintptr_t) pseed + FIRSTDATA);
		calabort->fixed.blockette_type = flip2(calabort->fixed.blockette_type);
		calabort->fixed.next_blockette = flip2(calabort->fixed.next_blockette);
		calabort->fixed.calibration_time.yr = flip2(calabort->fixed.calibration_time.yr);
		calabort->fixed.calibration_time.jday = flip2(calabort->fixed.calibration_time.jday);
		calabort->fixed.calibration_time.tenth_millisec =
		    flip2(calabort->fixed.calibration_time.tenth_millisec);
		calabort->res = flip2(calabort->res);
		break ;
	    }
	    }
#endif
#endif

	    if (rambling)
	    {
		LogMessage(CS_LOG_TYPE_INFO, "%4.4s.%3.3s Calibration %s, received at %s", 
			   &pcal->cr_station,
			   &dbuf.data_buf.ce.header_elog.cal_res.cr_seedname,
			   time_string(freebuf->user_data.header_time),
			   time_string(freebuf->user_data.reception_time)) ;
	    }
	    break ;
	}
	case ULTRA_PKT :
	{
	    if (checkmask (-1))
		return ;
	    else
		send_ack () ;
	    if (!linkstat.ultrarecv)
	    {
		preply = &dbuf.data_buf.cy ;
		full = TRUE ;
		if (ultra_seg_empty)
		{
		    ultra_seg_empty = FALSE ;
		    pultra = (pvoid) malloc(flip2(preply->total_bytes)) ;  /*IGD flip2 here */
		}
		ultra_seg[flip2 (preply->this_seg) / 8 ] = ultra_seg[flip2( preply->this_seg ) / 8] | /*IGD two flip2 here */
		    (byte) (1 << (flip2( preply->this_seg ) % 8))  ;  /*IGD  flip2 here */
		ta = (pchar) ((uintptr_t) pultra + flip2( preply->byte_offset )) ; /*IGD flip2 here */
		memcpy(ta, (pchar) &preply->bytes, flip2( preply->byte_count )) ; /*IGD flip2 here */
		preply->total_seg = flip2(preply->total_seg);	 /*IGD flip2 here */
		j=0;
		for (i = 1 ; i <= preply->total_seg ; i++)  
		    if ((ultra_seg[i / 8] & ((byte) (1 << (i % 8)))) == 0)
			full = FALSE ;
		    else
			j++ ;
		ultra_percent = (int) ((j / preply->total_seg) * 100.0) ;  /*IGD flip2 here */
		preply->total_seg = flip2(preply->total_seg);	 /*IGD flip2 back here */
		if (full)
		{
		    /* IGD Time to do byte swapping if we are with little-endian */
#ifdef	ENDIAN_LITTLE
		    /* IGD: We are going to swap several elements of pultra here */
		    /* IGD 01/21/01 swapping of pultra header is moved here */
		    /* because at this place we are guaranteed to byteswap header only once */
		    pultra->vcovalue   = flip2(pultra->vcovalue);
		    pultra->comcount   = flip2(pultra->comcount);
		    pultra->comoffset  = flip2(pultra->comoffset);
		    pultra->usedcount  = flip2(pultra->usedcount);
		    pultra->usedoffset = flip2(pultra->usedoffset);
		    pultra->calcount   = flip2(pultra->calcount);
		    pultra->caloffset  = flip2(pultra->caloffset);
		    pultra->ut_sp2    =  flip2(pultra->ut_sp2);
		    pultra->comm_mask  = flip4(pultra->comm_mask);     /*IGD : 9 flip2 and flip4 */
		    tmpa = (pvoid) ((uintptr_t) pultra  + pultra->caloffset) ; /*Swap calibrators structure */
		    flip_calibrators (tmpa);
		    tmpa = (pvoid) ((uintptr_t) pultra + pultra->usedoffset - 2); /*Swap channel structure */
		    flip_channels(tmpa, pultra->usedcount);
		
#endif

		    linkstat.ultrarecv = TRUE ;
		    vcovalue = pultra->vcovalue ;
		    if (pultra->ultra_rev >= 1)
			comm_mask = pultra->comm_mask ;
		    if (rambling)
			LogMessage(CS_LOG_TYPE_INFO, "Ultra record received with %d bytes",
				   flip2(preply->total_bytes)) ;   /*IGD flip2 here */
		}
	    }
	    break ;
	}
	case DET_AVAIL :
	{
	    if (checkmask (-1))
		return ;
	    else
		send_ack () ;
	    if (detavail_ok && (!detavail_loaded))
	    {
		preply = &dbuf.data_buf.cy ;
		full = TRUE ;
		pdetavail = NULL ;
		/* Make sure there is a valid client for this data */
		if (checkbusy ())
		{
		    pcr = (pvoid) clients[combusy].outbuf ;
		    if ((unsigned int) clients[combusy].outsize >= (unsigned int) flip2 (preply->total_bytes))  /*IGD flip2 here*/
			pdetavail = (pvoid) &pcr->moreinfo ;
		}
		if (pdetavail == NULL)
		{
		    detavail_ok = FALSE ;
		    combusy = NOCLIENT ;
		    break ;
		}
		detavail_seg[flip2(preply->this_seg) / 8] = detavail_seg[flip2(preply->this_seg) / 8] |
		    (byte) (1 << (flip2(preply->this_seg) % 8))  ;  /*IGD three flip2 here */
		ta = (pchar) ((uintptr_t) pdetavail + flip2(preply->byte_offset)) ;   /*IGD flip2 here */
		memcpy (ta, (pchar) &preply->bytes, flip2(preply->byte_count)) ;   /*IGD flip2 here */
		j = 0 ;
		for (i = 1 ; i <= flip2(preply->total_seg) ; i++) /*IGD flip2 here */
		    if ((detavail_seg[i / 8] & ((byte) (1 << (i % 8)))) == 0)
			full = FALSE ;
		    else
			j++ ;
		if (full)
		{
#ifdef	ENDIAN_LITTLE	/* IGD Need to do byte swapping in memory */
		    flip_detectors(ta)  ;
#endif
		    detavail_loaded = TRUE ;
		    pcr->completion_status = CSCS_FINISHED ;
		    combusy = NOCLIENT ;
		}
	    }
	    break ;
	}
	case DOWNLOAD :
	{
	    if (checkmask (-1))
		return ;
	    else
		send_ack () ;
	    if (xfer_down_ok && checkbusy ())
	    {
		preply = &dbuf.data_buf.cy ;
		pcom = (pvoid) clients[combusy].outbuf ;
		pdr = (pvoid) &pcom->moreinfo ;
		if (flip2(preply->this_seg) == 1)  /*IGD flip2 here */
		{
		    pds = (pvoid) &preply->bytes ;
		    xfer_total = flip2(pds->file_size) ; /*IGD flip2 here */
		    pdr->fsize = xfer_total ;
		    strpcopy (s1, pds->file_name) ;
		    strpcopy (s2, xfer_source) ;
		    if (strcasecmp((pchar) &s1, (pchar) &s2) != 0)
		    {
			do_abort () ;
			break ;
		    }
		    if (!pds->filefound)
		    {
			pcom->completion_status = CSCS_NOTFOUND ;
			do_abort () ;
			break ;
		    }
		    if (pds->toobig)
		    {
			pcom->completion_status = CSCS_TOOBIG ;
			break ;
		    }
		}
		if (xfer_size == 0)
		{
		    xfer_size = flip2(preply->total_bytes) ; /*IGD flip2 here */
		    pdr->dpshmid = shmget(IPC_PRIVATE, xfer_size, IPC_CREAT | PERM) ;
		    if (pdr->dpshmid == ERROR)
		    {
			pcom->completion_status = CSCR_PRIVATE ;
			do_abort () ;
			break ;
		    }
		    pdownload = (pvoid) shmat (pdr->dpshmid, NULL, 0) ;
		    if ((uintptr_t) pdownload == ERROR)
		    {
			pcom->completion_status = CSCR_PRIVATE ;
			do_abort () ;
			break ;
		    }
		    xfer_segments = flip2 (preply->total_seg) ; /* IGD flip2 here */
		}
/* Isolate client from header, start the data module with the actual file contents */
		xfer_offset = sizeof(download_struc) - 65000 ; /* source bytes to skip */
		ta = (pchar) ((uintptr_t) pdownload + flip2(preply->byte_offset) - xfer_offset) ; /* destination */  /*IGD flip2 here*/
		p1 = (pchar) &preply->bytes ; /* source */
		bc = flip2 (preply->byte_count) ; /* number of bytes */  /*IGD flip2 here */
		if (flip2(preply->this_seg) == 1) /*IGD flip2 here */
		{
		    bc = bc - xfer_offset ; /* first record contains header */
		    p1 = p1 + xfer_offset ;
		    ta = ta + xfer_offset ;
		}
		memcpy (ta, p1, bc) ;
		i = flip2(preply->this_seg) - 1 ; /*IGD flip2 here */
		j = i / 8 ;
		k = (byte) (1 << (i % 8)) ;
		if ((xfer_seg[j] & k) == 0)
		    pdr->byte_count = pdr->byte_count + bc ;  /* not already received */
		xfer_seg[j] = xfer_seg[j] | k ;
		l = 0 ;
		for (i = 0 ; i <= 127 ; i++)
		{
		    k = xfer_seg[i] ;
		    for (j = 0 ; j <= 7 ; j++)
			if ((k & (byte) (1 << j)) != 0)
			    l++ ;
		}
		if ((unsigned int) l >= (unsigned int) xfer_segments)
		{
		    xfer_down_ok = FALSE ;
		    down_count = 0 ;
		    pdr->byte_count = xfer_total ;
		    shmdt((pchar)pdownload) ;
		    pcom->completion_status = CSCS_FINISHED ;
		    combusy = NOCLIENT ;
		    xfer_size = 0 ;
		}
	    }
	    else if (++down_count > 5)
	    {
		xfer_down_ok = TRUE ;
		do_abort () ;
	    }
	    break ;
	}
	case UPMAP :
	{
	    if (checkmask (-1))
		return ;
	    else
		send_ack () ;
	    if (xfer_up_ok)
		process_upmap () ;
	    break ;
	}
	case CMD_ECHO :
	{
	    if (checkmask (-1))
		return ;
	    else
		send_ack () ;
	    if (checkbusy ())
	    {
		pcom = (pvoid) clients[combusy].outbuf ;
		preply = &dbuf.data_buf.cy ;
		memcpy ((pchar) &replybuf, (pchar) &preply->bytes, flip2(preply->byte_count)) ;  /*IGD flip2 here */
		if ((replybuf.ces.dp_seq == pcom->command_tag) &&
		    (pcom->completion_status == CSCS_INPROGRESS))
                {
			if (follow_up)
			{ /* Set AUTO DAC, now send prepared ACCURATE DAC */
			    if (++cmd_seq == 0)
				cmd_seq = 1 ;
			    gmsg.mmc.dp_seq = cmd_seq ;
			    send_tx_packet (0, gmsg.mmc.cmd_type, &gmsg) ;
			    pcom->command_tag = cmd_seq ;
			    follow_up = FALSE ;
			}
			else
			{
			    pcom->completion_status = CSCS_FINISHED ;
			    combusy = NOCLIENT ;
			}
		}
	    }
	    break ;
	}
	default :
	{
	    linkstat.last_bad = dtime () ;
	    LogMessage(CS_LOG_TYPE_ERROR, "INVALID RECORD TYPE=%d", dbuf.data_buf.cr.h.frame_type) ;
	    if (checkmask (-1))
		return ;
	    else
		send_ack () ;
	    break ;
	}
	}
    }
    else
    {
	linkstat.last_bad = dtime () ;
	linkstat.check_errors++ ;
	if (verbose) {
	    char str[4096];
	    int i, l;
	    if( pecp == NULL) {
		LogMessage(CS_LOG_TYPE_ERROR, "Packet size is odd size=%d",size);
	    }
	    else {
		LogMessage(CS_LOG_TYPE_ERROR, "CHECKSUM ERROR ON PACKET %d, BYTE COUNT=%d, CHECKSUM ERRORS=%d",
			   last_packet_received, size, linkstat.check_errors) ;
		LogMessage(CS_LOG_TYPE_ERROR, "COMPUTED CHECKSUM = %d,  PACKET CHECKSUM = %d", (int)dp_sum, (int)pecp->chksum);
		LogMessage(CS_LOG_TYPE_ERROR, "COMPUTED CRC = %08x,  PACKET CRC = %08x = %04hx %04hx", dp_crc, ltemp.l, ltemp.s[0], ltemp.s[1]);
	    }
	    str[0] = 0;
	    l = 0;
	    if (insane) {
		for (i=0; i<size+6; i++) {
		    unsigned int ival;
		    ival = dest[i];
		    sprintf (str+l, "%2x ", ival);
		    l += 3;
		}
		LogMessage(CS_LOG_TYPE_ERROR, "packet = %s", str);
/*::		    for (l=0; l<256; l++) {
  printf ("crctable[%d] = %d\n", l, crctable[l]);
  }
  ::*/
	    }
	}
    }
}

void fillbuf (void)
{
    int numread ;
    socklen_t clilen;
#ifdef _OSK
    u_int32 err, count ;
#endif
	
    src = (pchar) &sbuf ;
    srcend = src ;
    if (!serial)
    {
	if ((path < 0) && (netdly_cnt >= netdly))
	{
	    clilen = sizeof(cli_addr) ;
	    path = accept(sockfd, (psockaddr) &cli_addr, &clilen) ;
	    netdly_cnt = 0 ;
	    if ((verbose) && (path >= 0))
		LogMessage(CS_LOG_TYPE_INFO, "Network connection with DA opened") ;
	    if (linkstat.ultraon)
	    {
		linkstat.linkrecv = FALSE ;
		linkstat.ultrarecv = FALSE ;
		seq_valid = FALSE ;
		linkpoll = link_retry ;
	    }
	}
	if (path < 0)
	    return ;
    }
#ifdef _OSK
    if (serial) /* make sure we don't block here */
    {
	err = _os_gs_ready(path, &count) ;
	if ((err == EOS_NOTRDY) || (count == 0))
	    return ;
	else if (err != 0)
	{
	    numread = -1 ;
	    errno = err ;
	}
	else
	{
	    if (count > BLOB)
		count = BLOB ;
	    err = blockread (path, count, src) ;
	    if (err == 208)
		numread = read(path, src, count) ;
	    else if (err == 0)
		numread = count ;
	    else
	    {
		numread = -1 ;
		errno = err ;
	    }
	}
    }
    else
	numread = read(path, src, BLOB) ;
#else
    numread = read(path, src, BLOB) ;
#endif
    if (numread > 0)
    {
	srcend = (pchar) ((uintptr_t) srcend + numread) ;
	if ((insane) /* && (numread > maxbytes) */)
	{
	    maxbytes = numread ;
	    LogMessage(CS_LOG_TYPE_ERROR, "%d bytes read", numread) ;
	}
    }
    else if (numread < 0)
	if (errno != EWOULDBLOCK)
	{
	    linkstat.io_errors++ ;
	    linkstat.lastio_error = errno ;
	    if (serial)
	    {
		if (verbose)
		    perror ("Error reading from port ") ;
	    }
	    else
	    {
		if (verbose)
		    perror ("Network connection with DA closed\n") ;
		shutdown(path, 2) ;
		close(path) ;
		seq_valid = FALSE ;
		if (linkstat.ultraon)
		{
		    linkstat.linkrecv = FALSE ;
		    linkstat.ultrarecv = FALSE ;
		}
		path = -1 ; /* signal not open */
	    }
	}
}
	
short inserial (pchar b)
{
	
    if (src == srcend)
	fillbuf () ;
	
    if (src != srcend)
    {
	*b = *src++ ;
	return 1 ;
    }
    else
	return 0 ;
}
	
void dlestrip (void)
{
    boolean indle ;
	
    term = NULL ;
    indle = (lastchar == DLE) ;
    while ((src != srcend) && (dest != destend))
    {
	if (indle)
	{
	    *dest++ = *src ;
	    indle = FALSE ;
	}
	else if (*src == DLE)
	    indle = TRUE ;
	else if (*src == ETX)
	{
	    term = dest ;
	    src++ ;
	    lastchar = NUL ;
	    return ;
	}
	else
	    *dest++ = *src ;
	src++ ;
    }
    if (indle)
	lastchar = DLE ;
    else
	lastchar = NUL ;
}
	
void check_input (void)
{
    int numread ;
    short err ;
    char b ;
	
    if (udplink)
    {
	numread = recv(path, dest, destend - dest, 0) ;
	if (numread > 0)
	{
	    term = (pchar) ((uintptr_t) dest + numread) ;
	    process () ;
	}
	else if (numread < 0)
	    if (errno != EWOULDBLOCK)
	    {
		linkstat.io_errors++ ;
		linkstat.lastio_error = errno ;
		seq_valid = FALSE ;
		if (linkstat.ultraon)
		{
		    linkstat.linkrecv = FALSE ;
		    linkstat.ultrarecv = FALSE ;
		}
	    }
	return ;
    }
    if (src == srcend)
	fillbuf () ;
    switch (inphase)
    {
    case SOHWAIT :
	dest = (pchar) &dbuf.seq ;
    case SYNWAIT :
    {
	while (inphase != INBLOCK)
	{
	    lastchar = NUL ;
	    err = inserial(&b) ;
	    if (err != 1)
		return ;
	    if ((b == SOH))
		inphase = SYNWAIT ;
	    else if ((b == SYN) && (inphase == SYNWAIT))
		inphase = INBLOCK ;
	    else
		inphase = SOHWAIT ;
	}
    }
    case INBLOCK :
    {
	if (src == srcend)
	    fillbuf (); /*IGD -> used to be just fillbuf with no brackets, fixed*/
	if (src != srcend)
	{
	    dlestrip () ;
	    if (dest == destend)
		inphase = ETXWAIT ;
	    else if (term != NULL)
	    {
		inphase = SOHWAIT ;
		process () ;
		break ;
	    }
	    else
		break ;
	}
	else
	    break ;
    }
    case ETXWAIT :
    {
	if (src == srcend)
	    fillbuf () ;
	err = inserial (&b) ;
	if (err == 1)
	{
	    if (b == ETX)
	    {
		inphase = SOHWAIT ;
		term = dest ;
		process () ;
		break ;
	    }
	    else
	    {
		inphase = SOHWAIT ;
		break ;
	    }
	}
    }
    }
}

/******************************************************************************
                 BYTE-SWAPPING FUNCTIONS for little-endian vesion
		Ilya Dricker, (i.dricker@isti.com) ISTI
*******************************************************************************/

/*
  Ilya Dricker (i.dricker@isti.com) ISTI 12/15/99
*/
void flip_calibrators (char *tmpa)    {
    int i, j;
    ((cal_record *) tmpa)->number  =  flip2(((cal_record *) tmpa)->number)   ;
    for (i=0; i< ((cal_record *) tmpa)->number; i++)	{
	(((cal_record *) tmpa)->acal)[i].board =  flip2 ((((cal_record *) tmpa)->acal)[i].board);
	(((cal_record *) tmpa)->acal)[i].min_settle =  flip2 ((((cal_record *) tmpa)->acal)[i].min_settle);
	(((cal_record *) tmpa)->acal)[i].max_settle =  flip2 ((((cal_record *) tmpa)->acal)[i].max_settle);
	(((cal_record *) tmpa)->acal)[i].inc_settle =  flip2 ((((cal_record *) tmpa)->acal)[i].inc_settle);
	(((cal_record *) tmpa)->acal)[i].min_mass_dur =  flip2 ((((cal_record *) tmpa)->acal)[i].min_mass_dur);
	(((cal_record *) tmpa)->acal)[i].max_mass_dur =  flip2 ((((cal_record *) tmpa)->acal)[i].max_mass_dur);
	(((cal_record *) tmpa)->acal)[i].inc_mass_dur =  flip2 ((((cal_record *) tmpa)->acal)[i].inc_mass_dur);
	(((cal_record *) tmpa)->acal)[i].def_mass_dur =  flip2 ((((cal_record *) tmpa)->acal)[i].def_mass_dur);
	(((cal_record *) tmpa)->acal)[i].min_filter =  flip2 ((((cal_record *) tmpa)->acal)[i].min_filter);
	(((cal_record *) tmpa)->acal)[i].max_filter =  flip2 ((((cal_record *) tmpa)->acal)[i].max_filter);
	(((cal_record *) tmpa)->acal)[i].min_amp =  flip2 ((((cal_record *) tmpa)->acal)[i].min_amp);
	(((cal_record *) tmpa)->acal)[i].max_amp =  flip2 ((((cal_record *) tmpa)->acal)[i].max_amp);
	(((cal_record *) tmpa)->acal)[i].amp_step =  flip2 ((((cal_record *) tmpa)->acal)[i].amp_step);
	(((cal_record *) tmpa)->acal)[i].monitor =  flip2 ((((cal_record *) tmpa)->acal)[i].monitor);
	(((cal_record *) tmpa)->acal)[i].rand_min_period =  flip2 ((((cal_record *) tmpa)->acal)[i].rand_min_period);
	(((cal_record *) tmpa)->acal)[i].rand_max_period =  flip2 ((((cal_record *) tmpa)->acal)[i].rand_max_period);
	(((cal_record *) tmpa)->acal)[i].default_step_filt =  flip2 ((((cal_record *) tmpa)->acal)[i].default_step_filt);
	(((cal_record *) tmpa)->acal)[i].default_rand_filt =  flip2 ((((cal_record *) tmpa)->acal)[i].default_rand_filt);
	(((cal_record *) tmpa)->acal)[i].ct_sp2 =  flip2 ((((cal_record *) tmpa)->acal)[i].ct_sp2);


	(((cal_record *) tmpa)->acal)[i].map =  flip4 ((((cal_record *) tmpa)->acal)[i].map);
	(((cal_record *) tmpa)->acal)[i].waveforms =  flip4 ((((cal_record *) tmpa)->acal)[i].waveforms);
	(((cal_record *) tmpa)->acal)[i].sine_freqs =  flip4 ((((cal_record *) tmpa)->acal)[i].sine_freqs);
		

	for (j=0; j<4; j++)	{
	    ((((cal_record *) tmpa)->acal)[i].default_sine_filt)[j] =
		flip4 (((((cal_record *) tmpa)->acal)[i].default_sine_filt)[j]);
	    ((((cal_record *) tmpa)->acal)[i].durations)[j].min_dur  =
		flip4 (((((cal_record *) tmpa)->acal)[i].durations)[j].min_dur);
	    ((((cal_record *) tmpa)->acal)[i].durations)[j].max_dur  =
		flip4 (((((cal_record *) tmpa)->acal)[i].durations)[j].max_dur);
	    ((((cal_record *) tmpa)->acal)[i].durations)[j].inc_dur  =
		flip4 (((((cal_record *) tmpa)->acal)[i].durations)[j].inc_dur);
	}

    }	
}

/* IGD This is byte-swapping for channels: we only swap chancount, which seems like unused
   anyways, and the sampling rate for the channels
   Ilya Dricker (i.dricker@isti.com) ISTI 12/15/99
*/

void flip_channels(char *tmpa, int channels)
{
    int i;
    ((chan_struc *) tmpa)->chancount  =   flip2(((chan_struc *) tmpa)->chancount);
    for (i=0; i<channels; i++)	
	((chan_struc *) tmpa)->chans[i].rate  =  flip2(((chan_struc *) tmpa)->chans[i].rate);
}

/* IGD This is byte-swapping for detectors
   Ilya Dricker (i.dricker@isti.com) ISTI 12/16/99
*/
void flip_detectors(char *ta)	
{
    int i;

    ((det_request_rec *) ta)->count =  flip2(((det_request_rec *) ta)->count);
    ((det_request_rec *) ta)->dat_sp1 =  flip2(((det_request_rec *) ta)->dat_sp1);
    for (i=0; i<   ((det_request_rec *) ta)->count; i++)  {
	(((det_request_rec *) ta)->dets)[i].id    =  flip2((((det_request_rec *) ta)->dets)[i].id) ;
	flip4array((int32_t *)(((det_request_rec *) ta)->dets)[i].cons, 42) ;    /*IGD this is 42 byte-long char with parameters ,
										   which is interpreted as 13 long words in the
										   dpda client; swap it */
    }
}
/* IGD 03/09/01
 * byteswapping of swappable elements in MSEED header
 *********************************/
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
 Bit6 Bit7
 0    0	- indetermined (old data)
 1    0 - header is little endian
 0    1 - header is big endian
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
 ******************************************/
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

