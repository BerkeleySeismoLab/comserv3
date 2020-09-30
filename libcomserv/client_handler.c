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
   23 29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
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

short VER_COMMANDS = 23 ;     /*IGD LINUX compatible */

/* Comserv external variables used in this file. */
extern int retVal;
extern tuser_privilege user_privilege ;
extern boolean stop ;
extern byte cmd_seq ;
extern short combusy ;
extern short highclient ;
extern short resclient ;
extern short uids ;
extern pserver_struc base ;
extern tclients clients[MAXCLIENTS] ;
extern tring rings[NUMQ] ;
extern linkstat_rec linkstat ;
extern int32_t start_time ;
extern int32_t noackmask ;
extern int32_t blockmask ;
extern int32_t polltime ;
extern int32_t reconfig_on_err ;
extern int32_t netto ;
extern int32_t netdly ;
extern int32_t grpsize ;
extern int32_t grptime ;
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

/* External variables used only in this file */
int32_t reconfig_on_err = 25 ;
char seedformat[4] = { 'V', '2', '.', '3' } ;
char seedext = 'B' ;
/* Array to lookup privilege mask given the command */
int32_t privilege [CSCM_UPLOAD_ABORT+1] = {0, 0, 0, 0, 0, 0, 0, 0, PRIV_CLIENTS, PRIV_UNBLOCK,
    PRIV_RECONFIGURE, PRIV_SUSPEND_RESUME, PRIV_SUSPEND_RESUME, 0, PRIV_TERMINATE, PRIV_LINKADJ, 0,
    0, 0, 0, PRIV_SHELL, PRIV_VCO, PRIV_LINKADJ, PRIV_MASS_RECENTER, PRIV_CAL, PRIV_CAL, PRIV_DET, 
    PRIV_DET, PRIV_REC, PRIV_COMM, 0, PRIV_DOWNLOAD, PRIV_DOWNLOAD, PRIV_UPLOAD, PRIV_UPLOAD} ;

/* External functions used in this file. */
void unblock (short clientnum) ;

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

byte client_handler (pclient_struc svc, short clientnum)
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
    pvoid pv ;
    comstat_rec *pcom ;
    tring_elem *datatemp ;
    short *pshort ;
    client_info *pci ;
    one_client *poc ;
    int32_t lowest, priv ;
    int32_t msize ;
    seltype cmp ;
    static seltype any = "?????" ;
      
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
    retVal = client->command;

    switch (client->command)
    {
    case CSCM_ATTACH : break ;
    case CSCM_DATA_BLK :
    {
	client->valdbuf = 0 ;
/* If starting fresh, clear pointers and counters */
	if (client->seqdbuf != CSQ_NEXT)
	{
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
	    for (i = DATAQ ; i < NUMQ ; i++)
	    {
		plast = &pt->last[i] ;
		bscan[i] = plast->scan ;
		if ((bscan[i] == NULL) ||
		    (bscan[i]->packet_num != plast->packet))
		    bscan[i] = rings[i].tail ; /* not valid, start at oldest */
		while ((bscan[i] != rings[i].head) &&
		       (bscan[i]->packet_num < client->next_data))
		{
		    if (pt->blocking)
			clr_bit (&bscan[i]->blockmap, clientnum) ;
		    bscan[i] = (pvoid) bscan[i]->next ;
		    plast->scan = bscan[i] ;
		    plast->packet = bscan[i]->packet_num ;
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
	return CSCR_NODATA ;
    }
    case CSCM_CAL :
    {
	return CSCR_NODATA ;
    }
    case CSCM_DIGI :
    {
	return CSCR_NODATA ;
    }
    case CSCM_ULTRA :
    {
	return CSCR_NODATA ;
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
	return CSCR_NODATA ;
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
	return CSCR_INVALID ;
    }
    case CSCM_SUSPEND :
    {
	set_bit (&noackmask, NUMQ) ;
	linkstat.suspended = TRUE ;
	retVal = CSCM_SUSPEND; 
	break ;
    }
    case CSCM_RESUME :
    {
	clr_bit (&noackmask, NUMQ) ;
	linkstat.suspended = FALSE ;
	retVal = CSCM_RESUME;
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
	return CSCR_NODATA ;
    }
    case CSCM_TERMINATE :
    {
	if (checkcom(pcom, clientnum, FALSE))
	    return CSCR_BUSY ;
	pcom->command_tag = cmd_seq ;
	pcom->completion_status = CSCS_FINISHED ;
	stop = TRUE ;
	retVal = CSCM_TERMINATE;
	break ;
    }
    case CSCM_SHELL :
    {
	return CSCR_INVALID ;
    }
    case CSCM_FLOOD_CTRL :
    {
	return CSCR_INVALID ;
    }  
    case CSCM_VCO :
    {
	return CSCR_INVALID ;
    }
    case CSCM_LINKADJ :
    {
	return CSCR_INVALID ;
    }
    case CSCM_MASS_RECENTER :
    {
	return CSCR_INVALID ;
    }
    case CSCM_CAL_START :
    {
	return CSCR_INVALID ;
    }
    case CSCM_CAL_ABORT :
    {
	return CSCR_INVALID ;
    }
    case CSCM_DET_ENABLE :
    {
	return CSCR_INVALID ;
    }
    case CSCM_DET_CHANGE :
    {
	return CSCR_INVALID ;
    }
    case CSCM_REC_ENABLE :
    {
	return CSCR_INVALID ;
    }
    case CSCM_COMM_EVENT :
    {
	return CSCR_INVALID ;
    }
    case CSCM_DOWNLOAD :
    {
	return CSCR_INVALID ;
    }
    case CSCM_DOWNLOAD_ABORT :
    {
	return CSCR_INVALID ;
    }
    case CSCM_UPLOAD :
    {
	return CSCR_INVALID ;
    }
    case CSCM_UPLOAD_ABORT :
    {
	return CSCR_INVALID ;
    }
    case CSCM_LINKSET :
    {
	return CSCR_INVALID ;
    }
    default :
    {
	pcom->completion_status = CSCR_INVALID ;
	break ;
    }
    }
    return CSCR_GOOD ;
}
