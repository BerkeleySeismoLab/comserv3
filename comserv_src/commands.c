/*$Id: commands.c,v 1.2 2005/03/30 22:30:59 isti Exp $*/
/*   Client command handler
     Copyright 1994-2000 Quanterra, Inc.
     Written by Woodrow H. Owens
     Linux porting by ISTI 1999-2000
Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 31 Mar 94 WHO Created
    1 31 Mar 94 WHO Data and blockette buffers merged at client level.
    2  7 Apr 94 WHO Support for local commands added.
    3  9 Apr 94 WHO Remote commands added, one by one.
    4 15 Apr 94 WHO Last two commands added, yeah!
    5 16 Apr 94 WHO Allow CSCM_CAL and CSCM_CHAN commands on shear systems,
                    data will be faked based on config file entries. Set
                    seed format into linkstat.
    6 17 Apr 94 WHO Add code to handle CSQ_LAST and CSQ_TIME options.
    7 30 May 94 WHO Handle comm_mask and new parameters for CSCM_COMM_EVENT.
                    Change method of handling earliest time field for getting
                    packets.
    8  6 Jun 94 WHO Add definitions for privilege bit mask and check them.
    9  9 Jun 94 WHO Cleanup to avoid warnings.
   10 12 Jun 94 WHO PRIV_XXX constants fixed.
   11 18 Aug 94 WHO Fix = instead of == in CSCM_CMD_ACK.
   12 20 Jun 95 WHO CSCM_LINKADJ changed. CSCM_LINKSET added.
   13  3 Oct 95 WHO New definition for reconfigure added.
   14 29 May 96 WHO Start of conversion to run on OS9.
   15  9 Jun 96 WHO Transfer only valid bytes from server to client.
                    Set pshort in unblock call.
   16 15 Jul 96 WHO If there are no Comm Event names, just return a null
                    character at the beginning of the list.
   17  3 Dec 96 WHO Add support for Blockette Queue.
   18 27 Jul 97 WHO Add support for FLOOD control.
   19 17 Oct 97 WHO Add VER_COMMANDS
   20 21 Mar 00 WHO The fix in edition 26 of comlink.c actually started
                     checking the "command_tag" for command echos, which
                     was setup wrong or not at all for most commands.
   21  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
      16 Dec 99 IGD Implemented byte swapping for the message sent to DA for case CSCM_VCO (without success for now)
		    Implemented byte swapping for the messages sent to DA for case  CSCM_LINKADJ
      17 Dec 99 IGD Implemented byte swapping for the messages sent to DA for case   CSCM_MASS_RECENTER
                    Implemented byte swapping for the messages sent to DA for case    CSCM_CAL
      18 Dec 99 IGD Number of changes ; presumably swapping for every case of handler()
   22 24 Aug 07 DSN Separate ENDIAN_LITTLE from LINUX logic.
   23 20 Feb 2012 DSN Fix new debugging code to output only on rambling or insane setting.
   24 29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <limits.h>
#include "quanstrc.h"
#include "stuff.h"
#include "service.h"
#include "server.h"

short VER_COMMANDS = 24 ;     /*IGD LINUX compatible */

extern tuser_privilege user_privilege ;
extern boolean verbose ;
extern boolean rambling ;
extern boolean insane ;
extern boolean detavail_ok ;
extern boolean detavail_loaded ;
extern boolean stop ;
extern boolean follow_up ;
extern boolean xfer_down_ok ;
extern boolean xfer_up_ok ;
extern byte detavail_seg[14] ;
extern byte cmd_seq ;
extern byte upphase ;
extern linkstat_rec linkstat ;
extern short combusy ;
extern short vcovalue ;
extern short highclient ;
extern short resclient ;
extern short uids ;
extern short down_count ;
extern short mappoll ;
extern short xfer_resends ;
extern short sincemap ;
extern pserver_struc base ;
extern tclients clients[MAXCLIENTS] ;
extern tring rings[NUMQ] ;
extern link_record curlink ;
extern linkstat_rec linkstat ;
extern ultra_type *pultra ;
extern int32_t start_time ;
extern int32_t noackmask ;
extern int32_t blockmask ;
extern int32_t comm_mask ;
extern int32_t polltime ;
extern int32_t reconfig_on_err ;
extern int32_t netto ;
extern int32_t netdly ;
extern int32_t grpsize ;
extern int32_t grptime ;
extern DP_to_DA_msg_type gmsg ;
extern seg_map_type xfer_seg ;
extern download_struc *pdownload ;
extern tupbuf *pupbuf ;
extern string59 xfer_source ;
extern string59 xfer_destination ;
extern unsigned short xfer_total ;
extern unsigned short xfer_size ;
extern unsigned short xfer_up_curseg ;
extern unsigned short seg_size ;
extern unsigned short xfer_segments ;
extern unsigned short xfer_bytes ;
extern int upmemid ;
extern char seedformat[4] ;
extern char seedext ;
extern char station_desc[] ;

/* Define those functions that require privileged access */
#define PRIV_CLIENTS 1
#define PRIV_UNBLOCK 2
#define PRIV_RECONFIGURE 4
#define PRIV_SUSPEND_RESUME 8
#define PRIV_TERMINATE 16
#define PRIV_SHELL 32
#define PRIV_VCO 64
#define PRIV_LINKADJ 128
#define PRIV_MASS_RECENTER 256
#define PRIV_CAL 512
#define PRIV_DET 1024
#define PRIV_REC 2048
#define PRIV_COMM 4096
#define PRIV_DOWNLOAD 8192
#define PRIV_UPLOAD 16384

/* Array to lookup privilege mask given the command */
int32_t privilege [CSCM_UPLOAD_ABORT+1] = {0, 0, 0, 0, 0, 0, 0, 0, PRIV_CLIENTS, PRIV_UNBLOCK,
					   PRIV_RECONFIGURE, PRIV_SUSPEND_RESUME, PRIV_SUSPEND_RESUME, 0, PRIV_TERMINATE, PRIV_LINKADJ, 0,
					   0, 0, 0, PRIV_SHELL, PRIV_VCO, PRIV_LINKADJ, PRIV_MASS_RECENTER, PRIV_CAL, PRIV_CAL, PRIV_DET, 
					   PRIV_DET, PRIV_REC, PRIV_COMM, 0, PRIV_DOWNLOAD, PRIV_DOWNLOAD, PRIV_UPLOAD, PRIV_UPLOAD} ;

void unblock (short clientnum) ;
void do_abort (void) ;
void reconfigure (boolean full) ;
void send_tx_packet (byte nr, byte cmd_var, DP_to_DA_msg_type *msg) ;

boolean checkcom (comstat_rec *pcom, short clientnum, boolean dadp)
{
    if (pcom->completion_status != CSCS_IDLE)
	return TRUE ; /* can't start another one until done */
    if ((dadp) && (combusy != NOCLIENT))
	return TRUE ; /* Can't use the server */
    if (++cmd_seq == 0)
	cmd_seq = 1 ;
    return FALSE ;
}

byte handler (pclient_struc svc, short clientnum)
{
    typedef void *pvoid ;

    boolean good, goodone, found ;
    short i, j, k ;
    short lowi ;
    pring_elem bscan[NUMQ] ;
    pdata_user pdata ;
    pclient_station client ;
    last_struc *plast ;
    tclients *pt ;
    pselarray psa ;
    seltype *psel ;
    pvoid pv, pv2 ;
    pchar pc1, pc2 ;
    chan_record *pcr2 ;
    chan_struc *pcs ;
    ultra_rec *pur ;
    comstat_rec *pcom ;
    tring_elem *datatemp ;
    shell_com *pshell ;
    short *pshort ;
    int32_t *plong ;
    linkadj_com *plac ;
    recenter_com *prc ;
    cal_start_com *pcsc ;
    det_enable_com *pdec ;
    det_change_com *pdcc ;
    client_info *pci ;
    one_client *poc ;
    rec_enable_com *prec ;
    rec_one *pro ;
    download_com *pdc ;
    download_result *pdr ;
    upload_com *puc ;
    upload_result *pupres ;
    comm_event_com *pcec ;
    linkset_com *plsc ;
    boolean *pflood ;
    int32_t lowest, priv ;
    int32_t size, msize ;
    seltype cmp ;
    static seltype any = "?????" ;
    DP_to_DA_msg_type msg ;
    int ii;  /*IGD swapping counter */
      
    if (insane) {
	printf ("Enter handler client %d\n", clientnum);
	fflush (stdout);
    }
    pt = &clients[clientnum] ;
    client = (pclient_station) ((uintptr_t) svc + svc->offsets[svc->curstation]) ;
    pcom = (pvoid) ((uintptr_t) svc + client->comoutoffset) ;
    pv = &pcom->moreinfo ;
 
/* Check privilege requirements */
    if ((client->command >= CSCM_ATTACH) && (client->command <= CSCM_UPLOAD_ABORT))
	priv = privilege[client->command] ;
    else
	priv = 0 ;
    if (priv)
    {
	base->server_uid = getuid () ;
	if (base->server_uid != svc->client_uid)
	{
	    found = FALSE ;
	    for (i = 0 ; i < uids ; i++)
		if ((user_privilege[i].user_id == svc->client_uid) &&
		    ((user_privilege[i].user_mask & priv) != 0))
		{
		    found = TRUE ;
		    break ;
		}
	    if (! found)
		return CSCR_PRIVILEGE ;
	}
    }

/* Have permission to proceed */
    if (insane) {
	printf ("handler cmd=%d\n", client->command);
	fflush(stdout);
    }
    switch (client->command)
    {
    case CSCM_ATTACH : break ;
    case CSCM_DATA_BLK :
    {
	client->valdbuf = 0 ;
/* If starting fresh, clear pointers and counters */
	if (client->seqdbuf != CSQ_NEXT)
	{
	    if (rambling) {
		printf ("request = CSQ_FIRST | CSQ_LAST| CSQ_TIME\n");
		fflush (stdout);
	    }
	    client->next_data = 0 ;
	    for (i = DATAQ ; i < NUMQ ; i++)
	    {
		pt->last[i].scan = NULL ;
		pt->last[i].packet = -1 ;
		bscan[i] = rings[i].tail ; /* start at oldest */
		if (pt->blocking)
		    while ((bscan[i] != rings[i].head) &&
			   (! test_bit(bscan[i]->blockmap, clientnum)))
			bscan[i] = (pvoid) bscan[i]->next ;
	    }
	    if ((client->seqdbuf == CSQ_LAST) && (base->next_data > 0))
		client->next_data = base->next_data -1 ;
	}
/* Go through records gotten last time to unblock */
	if (client->seqdbuf != CSQ_FIRST) 
	{
	    if (rambling) {
		printf ("request = CSQ_NEXT | CSQ_LAST | CSQ_TIME\n");
		fflush (stdout);
	    }
	    for (i = DATAQ ; i < NUMQ ; i++)
	    {
		int debug_most_recent = -1;
		plast = &pt->last[i] ;
		bscan[i] = plast->scan ;
		if ((bscan[i] == NULL) ||
		    (bscan[i]->packet_num != plast->packet))
		    bscan[i] = rings[i].tail ; /* not valid, start at oldest */
		while ((bscan[i] != rings[i].head) &&
		       (bscan[i]->packet_num < client->next_data))
		{
		    if (pt->blocking) {
			clr_bit (&bscan[i]->blockmap, clientnum) ;
/*:: start debug */
			debug_most_recent = bscan[i]->packet_num;
/*:: end debug */
		    }
		    bscan[i] = (pvoid) bscan[i]->next ;
		    plast->scan = bscan[i] ;
		    plast->packet = bscan[i]->packet_num ;
		}
/*:: start debug */
		if (rambling) {
		    if (debug_most_recent >= 0) {
			printf ("ack queue %d thru packet_num=%d client %d\n", i, debug_most_recent, clientnum);
			fflush (stdout);
		    }
		}
/*:: end debug */
	    }
	}
/* 
   New records that can be transferred to client. They are transferred in the 
   order that they were received, regardless of packet type.
*/
	pdata = (pdata_user) ((uintptr_t) svc + client->dbufoffset) ;
	while (client->valdbuf < client->reqdbuf)
	{
	    lowest = INT32_MAX ;
	    for (i = DATAQ ; i < NUMQ ; i++)
		if ((test_bit(client->datamask, i)) &&
		    (bscan[i] != rings[i].head) &&
		    (bscan[i]->packet_num < lowest))
		{
		    lowi = i ;
		    lowest = bscan[i]->packet_num ;
		}
	    if (lowest == INT32_MAX)
		break ; /* no more new blockettes */
	    if (test_bit(client->datamask, lowi))
	    {
		good = bscan[lowi]->user_data.header_time >= client->startdbuf ;
		if (good && (lowi < NUMQ)) /* still good and selectors valid */
		{
		    psa = (pselarray) ((uintptr_t) svc + client->seloffset) ;
		    good = FALSE ;
		    memcpy((pchar) &cmp, (pchar) &bscan[lowi]->user_data.data_bytes[13], 5) ;
		    for (k = client->sels[lowi].first ; k <= client->sels[lowi].last ; k++)
		    {
			goodone = TRUE ;
			psel = &((*psa)[k]) ;
			if (memcmp((pchar) psel, (pchar)&any, 5) != 0)
			{
			    for (j = 0 ; j < 5 ; j++)
				if (((*psel)[j] != '?') && ((*psel)[j] != cmp[j]))
				{
				    goodone = FALSE ;
				    break ;
				}
			}
			if (goodone)
			{
			    good = TRUE ;
			    break ;
			}
		    }
		}
		if (good)
		{
		    memcpy ((pchar) pdata, (pchar) &bscan[lowi]->user_data, rings[lowi].xfersize) ;
		    client->valdbuf++ ;
		    pdata = (pdata_user) ((uintptr_t) pdata + client->dbufsize) ;
		    if (insane) {
			printf ("send queue %d packet_num %d client %d\n",
				lowi, bscan[lowi]->packet_num, clientnum);
			fflush(stdout);
		    }
		}
	    }
	    client->next_data = bscan[lowi]->packet_num + 1 ;
	    bscan[lowi] = (pvoid) bscan[lowi]->next ;
	}
	client->seqdbuf = CSQ_NEXT ;
	break ;
    }
    case CSCM_LINK :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (! linkstat.linkrecv)
	    return CSCR_NODATA ;
	if (client->comoutsize < sizeof(link_record))
	    return CSCR_SIZE ;
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	memcpy(pv, (pchar) &curlink, sizeof(link_record)) ;  /*IGD curlink is byte-swapped */
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_CAL :
    {
	if (! linkstat.ultrarecv)
	    return CSCR_NODATA ;
	size = sizeof(cal_record) - ((MAXCAL - pultra->calcount) * sizeof(eachcal)) ;
	if (client->comoutsize < size)
	    return CSCR_SIZE ;
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	pv2 = (pvoid) ((uintptr_t) pultra + pultra->caloffset) ;
	memcpy(pv, pv2, size) ;
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_DIGI :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (! linkstat.ultrarecv)
	    return CSCR_NODATA ;
	if (client->comoutsize < sizeof(digi_record))
	    return CSCR_SIZE ;
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	memcpy (pv, (pchar) pultra, sizeof(digi_record)) ;  /*IGD test this case */
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_ULTRA :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (! linkstat.ultrarecv)
	    return CSCR_NODATA ;
	size = sizeof(ultra_rec) - (CE_MAX * (COMMLENGTH + 1)) ;
	msize = 0 ;
	pc1 = (pchar) ((uintptr_t) pultra + pultra->comoffset) ;
	pc2 = pc1 ;
	for (i = 0 ; i < pultra->comcount ; i++)
	{
	    msize = msize + *pc1 + 1 ;
	    pc1 = (pchar) ((uintptr_t) pc1 + *pc1 + 1) ;
	}
	size = size + msize ;
	if (client->comoutsize < size)
	    return CSCR_SIZE ;
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	pur = (pvoid) pv ;
	pur->vcovalue = vcovalue ;
	pur->pllon = pultra->pllon ;
	pur->umass_ok = pultra->umass_ok ;
	pur->ultra_rev = pultra->ultra_rev ;
	pur->comm_mask = comm_mask ;
	if (pultra->comcount != 0)
	    memcpy ((pchar) &pur->commnames, pc2, msize) ;
	else
	    pur->commnames[0] = '\0' ;
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_LINKSTAT :
    {
	if (client->comoutsize < sizeof(linkstat_rec))
	    return CSCR_SIZE ;
	msize = 0 ;
	for (j = DATAQ ; j < NUMQ ; j++)
	{
	    datatemp = rings[j].head ;
	    for (i = 0 ; i < rings[j].count ; i++)
	    {
		if (datatemp->blockmap)
		    msize++ ;
		datatemp = (pvoid) datatemp->next ;
	    }
	}
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	linkstat.blocked_packets = msize ;
	linkstat.seconds_inop = (uintptr_t) dtime () - start_time ;
	memcpy((pchar) &linkstat.seedformat, (pchar) &seedformat, 4) ;
	linkstat.seedext = seedext ;
	strpas(linkstat.description, station_desc) ;
	linkstat.lsr_sp1 = 0 ;
	linkstat.pollusecs = polltime ;
	linkstat.reconcnt = reconfig_on_err ;
	linkstat.net_idle_to = netto ;
	linkstat.net_conn_dly = netdly ;
	linkstat.grpsize = grpsize ;
	linkstat.grptime = grptime ;
	memcpy(pv, (pchar) &linkstat, sizeof(linkstat_rec)) ;
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_CHAN :
    {
	if (! linkstat.ultrarecv)
	    return CSCR_NODATA ;
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	size = 0 ;
	pcs = (pvoid) pv ;
	pcs->chancount = 0 ;
	pcr2 = (pvoid) ((uintptr_t) pultra + pultra->usedoffset) ;
	for (i = 0 ; i < pultra->usedcount ; i++)
	{
	    psa = (pselarray) ((uintptr_t) svc + client->seloffset) ;
	    good = FALSE ;
	    memcpy((pchar) &cmp, (pchar) &pcr2->seedloc, 2) ;
	    memcpy(&cmp[2], (pchar) &pcr2->seedname, 3) ;
	    for (k = client->sels[CHAN].first ; k <= client->sels[CHAN].last ; k++)
	    {
		goodone = TRUE ;
		psel = &((*psa)[k]) ;
		if (memcmp((pchar) psel, (pchar) &any, 5) != 0)
		{
		    for (j = 0 ; j < 5 ; j++)
			if (((*psel)[j] != '?') && ((*psel)[j] != cmp[j]))
			{
			    goodone = FALSE ;
			    break ;
			}
		}
		if (goodone)
		{
		    good = TRUE ;
		    break ;
		}
	    }
	    if (good)
	    {
		size = size + sizeof(chan_record) ;
		if (client->comoutsize < (size + 1))
		    return CSCR_SIZE ;
		memcpy ((pchar) &((pcs->chans)[pcs->chancount++]), (pchar) pcr2, sizeof(chan_record)) ;
	    }
	    pcr2++ ;
	}
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_CLIENTS :
    {
	if (client->comoutsize < (sizeof(one_client) * highclient + 2))
	    return CSCR_SIZE ;
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	pci = pv ;
	pci->client_count = 0 ;
	for (i = 0 ; i < highclient ; i++)
	{
	    poc = &pci->clients[pci->client_count++] ;
	    poc->client_memid = clients[i].client_memid ;
	    poc->client_pid = clients[i].client_pid ;
	    copy_cname_cs_cs(poc->client_name,clients[i].client_name) ;
	    poc->last_service = clients[i].last_service ;
	    poc->timeout = clients[i].timeout ;
	    poc->blocking = clients[i].blocking ;
	    poc->active = clients[i].active ;
	    poc->reserved = (i < resclient) ;
	    msize = 0 ;
	    for (j = DATAQ ; j < NUMQ ; j++)
	    {
		datatemp = rings[j].head ;
		for (k = 0 ; k < rings[j].count ; k++)
		{
		    if (test_bit(datatemp->blockmap, i))
			msize++ ;
		    datatemp = (pvoid) datatemp->next ;
		}
	    }
	    poc->block_count = msize ;
	}
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_UNBLOCK :
    {
	pshort = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	unblock (*pshort) ;
	clr_bit (&blockmask, *pshort) ;
	break ;
    }
    case CSCM_RECONFIGURE :
    {
	reconfigure (TRUE) ;
	break ;
    }
    case CSCM_SUSPEND :
    {
	set_bit (&noackmask, NUMQ) ;
	linkstat.suspended = TRUE ;
	break ;
    }
    case CSCM_RESUME :
    {
	clr_bit (&noackmask, NUMQ) ;
	linkstat.suspended = FALSE ;
	break ;
    }
    case CSCM_CMD_ACK :
    {
	pcom->completion_status = CSCS_IDLE ;
	if (combusy == clientnum)
	    combusy = NOCLIENT ;
	break ;
    }
    case CSCM_DET_REQUEST :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (! linkstat.ultrarecv)
	    return CSCR_NODATA ;
	if (checkcom(pcom, clientnum, TRUE))
	    return CSCR_BUSY ;
	psa = (pselarray) ((uintptr_t) svc + client->seloffset) ;
	msg.drs.cmd_type = DET_REQ ;
	msg.drs.dp_seq = cmd_seq ;
	msg.drs.rc_sp3 = 0 ;
	k = client->sels[CHAN].first ;
	memcpy ((pchar) &msg.drs.dr_loc, (pchar) &((*psa)[k]), 2) ;
	memcpy ((pchar) &msg.drs.dr_name, &((*psa)[k][2]), 3) ;
	combusy = clientnum ;
	clients[clientnum].outbuf = (pvoid) pcom ;
	clients[clientnum].outsize = client->comoutsize ;
	detavail_ok = TRUE ;
	detavail_loaded = FALSE ;
	for (i = 0 ; i < 14 ; i++)
	    detavail_seg[i] = 0 ;
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, DET_REQ, &msg) ;
	pcom->completion_status = CSCS_INPROGRESS ;
	break ;
    }
    case CSCM_TERMINATE :
    {
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	stop = TRUE ;
	break ;
    }
    case CSCM_SHELL :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pshell = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.scs.cmd_type = SHELL_CMD ;
	msg.scs.dp_seq = cmd_seq ;
	memcpy ((pchar) &msg.scs.sc, (pchar) &pshell->shell_parameter, 80) ; /*IGD char !*/
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, SHELL_CMD, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_FLOOD_CTRL :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pflood = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.fcs.cmd_type = FLOOD_CTRL ;
	msg.fcs.dp_seq = cmd_seq ;
	msg.fcs.flood_on_off = *pflood ;
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, FLOOD_CTRL, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }  
    case CSCM_VCO :
    {
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pshort = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.mmc.cmd_type = AUTO_DAC_CORRECTION ;
	if (*pshort == -1)
	{
	    msg.mmc.cmd_parms.param1 = flip2(1) ; /* Enable PLL */     /*IGD swap bytes before sending to DA */
	    pultra->pllon = TRUE ;
	}
	else
	{
	    msg.mmc.cmd_parms.param1 = flip2(0) ; /* Disable PLL */     /*IGD swap bytes before sending to DA */
	    pultra->pllon = FALSE ;
	}
	msg.mmc.dp_seq = cmd_seq ;
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, AUTO_DAC_CORRECTION, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	if (*pshort != -1)
	{
	    vcovalue = *pshort ;
	    if (linkstat.ultrarecv)
		pultra->vcovalue = *pshort ;
	    if (! curlink.rcecho)
	    {
		pcom->completion_status = CSCS_IDLE ;
		if (checkcom(pcom, clientnum, FALSE))
		    return CSCR_BUSY ;
	    }
	    if (linkstat.ultraon)
	    {
		gmsg.mmc.cmd_type = ACCURATE_DAC_CORRECTION ;
		gmsg.mmc.cmd_parms.param2 = flip2(*pshort >> 8) ;   /*IGD flip before sending to DA */
		gmsg.mmc.cmd_parms.param3 = flip2(*pshort & 255) ;     /*IGD flip before sending to DA */
	    }
	    else
	    {
		gmsg.mmc.cmd_type = MANUAL_CORRECT_DAC ;
		gmsg.mmc.cmd_parms.param2 = flip2(*pshort >> 4) ;    /*IGD flip before sending to DA */
	    }
	    gmsg.mmc.dp_seq = cmd_seq ;
	    if (curlink.rcecho)
		follow_up = TRUE ;
	    else
	    {
		pcom->command_tag = cmd_seq ;
		send_tx_packet (0, ACCURATE_DAC_CORRECTION, &gmsg) ;
		pcom->completion_status = CSCS_FINISHED ;
	    }
	}
	break ;
    }
    case CSCM_LINKADJ :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	plac = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.las.cmd_type = LINK_ADJ ;
	msg.las.dp_seq = cmd_seq ;
	memcpy ((pchar) &msg.las.la, (pchar) plac, sizeof(linkadj_com)) ;
/* IGD Do byte swapping */
#ifdef	ENDIAN_LITTLE	/*IGD #ifdef is redundant, but let it be here anyways */
	msg.las.la.window_size = flip2(msg.las.la.window_size);
	msg.las.la.resendtime = flip2(msg.las.la.resendtime);
	msg.las.la.synctime = flip2(msg.las.la.synctime);
	msg.las.la.resendpkts = flip2(msg.las.la.resendpkts);
	msg.las.la.netdelay = flip2(msg.las.la.netdelay);
	msg.las.la.nettime = flip2(msg.las.la.nettime);
	msg.las.la.netmax = flip2(msg.las.la.netmax);
	msg.las.la.groupsize = flip2(msg.las.la.groupsize);
	msg.las.la.grouptime = flip2(msg.las.la.grouptime);
	msg.las.la.lasp1 = flip2(msg.las.la.lasp1);
	msg.las.la.lasp2  = flip2(msg.las.la.lasp2);
#endif
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, LINK_ADJ, &msg) ;
	curlink.window_size = plac->window_size ;
	curlink.msg_prio = plac->set_msg ;
	curlink.det_prio = plac->set_det ;
	curlink.time_prio = plac->set_time ;
	curlink.cal_prio = plac->set_calp ;
	curlink.resendtime = plac->resendtime ;
	curlink.synctime = plac->synctime ;
	curlink.resendpkts = plac->resendpkts ;
	curlink.netdelay = plac->netdelay ;
	curlink.nettime = plac->nettime ;
	curlink.netmax = plac->netmax ;
	curlink.groupsize = plac->groupsize ;
	curlink.grouptime = plac->grouptime ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_MASS_RECENTER :
    {
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	prc = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	if (linkstat.ultraon)
	{
	    msg.ums.cmd_type = ULTRA_MASS ;
	    msg.ums.dp_seq = cmd_seq ;
	    msg.ums.mbrd = flip2(prc->board) ;    /*IGD flip2 here */
	    msg.ums.mdur = flip2(prc->duration ); /*IGD flip2 here */
	}
	else
	{
	    msg.mmc.cmd_type = MASS_RECENTERING ;
	    msg.mmc.dp_seq = cmd_seq ;
	}
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, msg.ums.cmd_type, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_CAL_START :
    {
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pcsc = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	if (linkstat.ultraon)
	{
	    msg.ucs.cmd_type = ULTRA_CAL ;
	    msg.ucs.dp_seq = cmd_seq ;
	    msg.ucs.rc_sp2 = flip2(0) ;   /*IGD flip2 here */
	    memcpy ((pchar) &msg.ucs.xc, (pchar) pcsc, sizeof(cal_start_com)) ;
#ifdef	ENDIAN_LITTLE                  /*IGD flips here */
	    msg.ucs.xc.calnum  = flip2 (msg.ucs.xc.calnum);
	    msg.ucs.xc.duration  = flip4 (msg.ucs.xc.duration);
	    msg.ucs.xc.amp  = flip2 (msg.ucs.xc.amp);
	    msg.ucs.xc.rmult  = flip2 (msg.ucs.xc.rmult);
	    msg.ucs.xc.map  = flip2 (msg.ucs.xc.map);
	    msg.ucs.xc.settle  = flip2 (msg.ucs.xc.settle);
	    msg.ucs.xc.filt  = flip2 (msg.ucs.xc.filt);
	    msg.ucs.xc.ext_sp2  = flip2 (msg.ucs.xc.ext_sp2);
#endif
	}
	else
	{
	    msg.mmc.cmd_type = START_CAL ;
	    msg.mmc.dp_seq = cmd_seq ;
	    msg.mmc.cmd_parms.param0 = 0 ;          /*IGD char!*/
	    msg.mmc.cmd_parms.param1 = flip2(0x5a) ;
	    j = pcsc->map << ((pcsc->calnum - 1) * 3) ;
	    switch (pcsc->calcmd)
	    {
	    case SINE :
	    {
		msg.mmc.cmd_parms.param2 = flip2(0x10 + (j & 7)) ;  /*IGD flip2 here */
		msg.mmc.cmd_parms.param4 = flip2(pcsc->sfrq - 3) ;    /*IGD flip2 here */
		break ;
	    }
	    case STEP :
	    {
		msg.mmc.cmd_parms.param2 =flip2( 0x30 + (j & 7)) ;  /*IGD flip2 here */
		msg.mmc.cmd_parms.param4 = flip2(0) ;                 /*IGD flip2 here */
		break ;
	    }
	    case RAND :
	    {
		msg.mmc.cmd_parms.param2 = flip2(0x70 + (j & 7)) ; /*IGD flip2 here */
		msg.mmc.cmd_parms.param4 = flip2(pcsc->rmult) ;      /*IGD flip2 here */
		break ;
	    }
	    case WRAND :
	    {
		msg.mmc.cmd_parms.param2 = flip2(0x70 + (j & 7)) ; /*IGD flip2 here */
		msg.mmc.cmd_parms.param4 = flip2(0 - pcsc->rmult) ;  /*IGD flip2 here */
		break ;
	    }
	    }
	    msg.mmc.cmd_parms.param3 = flip2((pcsc->amp / 6) + 17) ;  /*IGD flip2 here */
	    msg.mmc.cmd_parms.param6 = flip2(pcsc->duration / 256) ;  /*IGD flip2 here */
	    msg.mmc.cmd_parms.param5 = flip2(pcsc->duration & 255) ;   /*IGD flip2 here */
	    msg.mmc.cmd_parms.param7 = flip2(((! pcsc->plus) << 1) +
					     pcsc->capacitor + (j & 0x38)) ;      /*IGD flip2 here */
	}
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, msg.ucs.cmd_type, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_CAL_ABORT :
    {
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pshort = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	if (linkstat.ultraon)
	{
	    msg.uss.cmd_type = ULTRA_STOP ;
	    msg.uss.dp_seq = cmd_seq ;
	    msg.uss.sbrd = flip2(*pshort) ;  /*IGD flip2 here*/
	}
	else
	{
	    msg.mmc.cmd_type = STOP_CAL ;
	    msg.mmc.dp_seq = cmd_seq ;
	    msg.mmc.cmd_parms.param0 = 0 ;
	}
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, msg.ums.cmd_type, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_DET_ENABLE :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pdec = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.des.cmd_type = DET_ENABLE ;
	msg.des.dp_seq = cmd_seq ;
	msg.des.rc_sp4 = flip2(0) ;  /*IGD flip2 here just in case*/
	memcpy ((pchar) &msg.des.de, (pchar) pdec, sizeof(det_enable_com)) ;
	      
#ifdef	ENDIAN_LITTLE
	for (ii = 1; ii < msg.des.de.count ; ii++)	{
	    msg.des.de.detectors[ii].detector_id = 	
		flip2(msg.des.de.detectors[ii].detector_id );  /*IGD flip2 here */
	}
	msg.des.de.count = flip2(msg.des.de.count); /*IGD flip2 here */
#endif

	
	send_tx_packet (0, DET_ENABLE, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pchar) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_DET_CHANGE :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pdcc = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.dcs.cmd_type = DET_CHANGE ;
	msg.dcs.dp_seq = cmd_seq ;
	msg.dcs.rc_sp5 = flip2(0) ; /*IGD flip2 here ; just in case */
	memcpy ((pchar) &msg.dcs.dc, (pchar) pdcc, sizeof(det_change_com)) ;
	msg.dcs.dc.id = flip2(msg.dcs.dc.id); /*IGD flip2 here */
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, DET_CHANGE, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pchar) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_REC_ENABLE :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	prec = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.res.cmd_type = REC_ENABLE ;
	msg.res.dp_seq = cmd_seq ;
	msg.res.rc_sp6 = flip2(0) ; /* IGD flip2 here in case */
	memcpy ((pchar) &msg.res.re, (pchar) prec, sizeof(rec_enable_com)) ;
	for (ii=0; ii<msg.res.re.count; ii++)	{
	    msg.res.re.changes[ii].rec_sp1 = flip2(msg.res.re.changes[ii].rec_sp1);
	}                                        /*IGD flip2 and... */
	msg.res.re.count = flip2(msg.res.re.count); /*IGD flip count itself after */
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, REC_ENABLE, &msg) ;
	for (j = 0 ; j < prec->count ; j++)
	{
	    pro = &(prec->changes[j]) ;
	    pcr2 = (pvoid) ((uintptr_t) pultra + pultra->usedoffset) ;
	    for (i = 0 ; i < pultra->usedcount ; i++)
	    {
		if ((memcmp((pchar) &pro->seedname, (pchar) &pcr2->seedname, 3) == 0) &&
		    (memcmp((pchar) &pro->seedloc, (pchar) &pcr2->seedloc, 2) == 0))
		{
		    pcr2->enabled = pro->mask ;
		    pcr2->c_prio = pro->c_prio ;
		    pcr2->e_prio = pro->e_prio ;
		    break ;
		}
		else
		    pcr2++ ;
	    }
	}
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_COMM_EVENT :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, curlink.rcecho))
	    return CSCR_BUSY ;
	pcec = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	msg.ces.cmd_type = COMM_EVENT ;
	msg.ces.dp_seq = cmd_seq ;
	msg.ces.rc_sp1 = flip2(0) ;  /*IGD flip2 just case */
	comm_mask = (comm_mask & ((~ pcec->remote_mask) | pcec->remote_map)) |
	    (pcec->remote_mask & pcec->remote_map) ;
	msg.ces.mask = flip4(comm_mask) ;  /*IGD flip4 */
	plong = pv ;
	*plong = comm_mask ;
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, COMM_EVENT, &msg) ;
	if (curlink.rcecho)
	{
	    combusy = clientnum ;
	    clients[clientnum].outbuf = (pvoid) pcom ;
	    pcom->completion_status = CSCS_INPROGRESS ;
	}
	else
	    pcom->completion_status = CSCS_FINISHED ;
	break ;
    }
    case CSCM_DOWNLOAD :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, TRUE))
	    return CSCR_BUSY ;
	/* As a precaution, send abort message first */
	msg.downs.cmd_type = DOWN_ABT ;
	msg.downs.dp_seq = cmd_seq ;
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, DOWN_ABT, &msg) ;
	pdc = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	pdr = pv ;
	pdr->dpshmid = NOCLIENT ;
	pdr->fsize = 0 ;
	pdr->byte_count = 0 ;
	msg.downs.cmd_type = DOWN_REQ ;
	msg.downs.dp_seq = cmd_seq ;
	memcpy((pchar) &msg.downs.fname, pdc->dasource, 60) ; /* da file name */
	memcpy((pchar) &xfer_source, pdc->dasource, 60) ; /* keep copy for comparison */
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, DOWN_REQ, &msg) ;
	memset((pchar) &xfer_seg, '\0', 128) ; /* clear map */
	pdownload = NULL ;
	xfer_down_ok = TRUE ;
	down_count = 0 ;
	xfer_total = 0 ;
	xfer_size = 0 ;
	combusy = clientnum ;
	clients[clientnum].outbuf = (pchar) pcom ;
	pcom->completion_status = CSCS_INPROGRESS ;
	break ;
    }
    case CSCM_DOWNLOAD_ABORT :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (xfer_down_ok)
	    do_abort () ;
	if (pcom->completion_status == CSCS_IDLE)
	{
	    if (checkcom(pcom, clientnum, FALSE))
		return CSCR_BUSY ;
	    pcom->completion_status = CSCS_FINISHED ;
	}
	break ;
    }
    case CSCM_UPLOAD :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (checkcom(pcom, clientnum, TRUE))
	    return CSCR_BUSY ;
	puc = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	pupres = pv ;
	pupres->bytecount = 0 ;
	pupres->retries = -1 ;
	pupbuf = (pvoid) shmat (puc->dpshmid, NULL, 0) ;
	if ((uintptr_t) pupbuf == ERROR)
	    return CSCR_PRIVATE ;
	upmemid = puc->dpshmid ;
	msg.us.cmd_type = UPLOAD ;
	msg.us.dp_seq = cmd_seq ;
	msg.us.return_map = TRUE ;
	msg.us.upload_control = CREATE_UPLOAD ;
	msg.us.up_union.up_create.file_size = flip2(puc->fsize) ; /*IGD flip2 */
	memcpy((pchar) &msg.us.up_union.up_create.file_name, puc->dadest, 60) ; /* da file name */
	memcpy((pchar) &xfer_destination, puc->dadest, 60) ; /* keep copy for comparison */
	pcom->command_tag = cmd_seq ;
	send_tx_packet (0, UPLOAD, &msg) ;
	memset((pchar) &xfer_seg, '\0', 128) ; /* clear map */
	upphase = WAIT_CREATE_OK ;
	xfer_up_curseg = 0 ;
	seg_size = DP_TO_DA_MESSAGE_LENGTH - 10 ;
	xfer_size = puc->fsize ;
	xfer_segments = (unsigned int) (xfer_size + (seg_size - 1)) / (unsigned int) seg_size ;
	xfer_up_ok = TRUE ;
	xfer_total = xfer_size ;
	xfer_bytes = 0 ;
	sincemap = 0 ;
	mappoll = 30 ;
	xfer_resends = -1 ;
	combusy = clientnum ;
	clients[clientnum].outbuf = (pchar) pcom ;
	pcom->completion_status = CSCS_INPROGRESS ;
	break ;
    }
    case CSCM_UPLOAD_ABORT :
    {
	if (! linkstat.ultraon)
	    return CSCR_INVALID ;
	if (xfer_up_ok)
	    do_abort () ;
	if (pcom->completion_status == CSCS_IDLE)
	{
	    if (checkcom(pcom, clientnum, FALSE))
		return CSCR_BUSY ;
	    pcom->completion_status = CSCS_FINISHED ;
	}
	break ;
    }
    case CSCM_LINKSET :
    {
	plsc = (pvoid) ((uintptr_t) svc + client->cominoffset) ;
	polltime = plsc->pollusecs ;
	reconfig_on_err = plsc->reconcnt ;
	netto = plsc->net_idle_to ;
	netdly = plsc->net_conn_dly ;
	grpsize = plsc->grpsize ;
	grptime = plsc->grptime ;
	break ;
    }
    default :
    {
	pcom->completion_status = CSCR_INVALID ;
	break ;
    }
    }
    return CSCR_GOOD ;
}
