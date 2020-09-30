/*$Id: dpda.c,v 1.3 2005/06/16 20:52:10 isti Exp $*/
/*   Send and receive commands from dp to da.
     Copyright 1994-1998 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0  8 Apr 94 WHO First created from test.c
    1 12 Apr 94 WHO The saga continues.
    2 16 Apr 94 WHO In "getcal", set the "channels" structure to the
                    calibrator specified.
    3 30 May 94 WHO Update Ultra Info and Comm Event commands.
    4  6 Jun 94 WHO Add string display of CSCR_PRIVILEGE error.
    5 10 Jun 94 WHO Cleanup to avoid warnings.
    6  9 Aug 94 WHO Change Download File & Upload File to avoid
                    confusing Doug. Add display of last_good and last_bad
                    in linkstat command.
    7 11 Aug 94 WHO Check for NULL result from gets on EOF.
    8 23 Dec 94 WHO Fix changing comm/event comlink priority checks.
    9 16 Jun 95 WHO Changes to link settings to conform to new field
                    definitions.
   10 20 Jun 95 WHO Show new fields in linkstat and add support for linkset.
   11 29 May 96 WHO Start of conversion to run on OS9.
   12 15 Jul 96 WHO If first character in comm event name list is null, there
                    is no list. Show station connected to on banner.
   13  4 Aug 96 WHO In a few places, treat a CR only entry to a prompt as an
                    abort command.
   14 28 Sep 96 WHO In getcal, return zero when the user selects calibrator
                    number 0, instead of using board 1. When entering channel
                    list for calibrate, allow comma separator.
   15 16 Jun 97 WHO Display and accept "DEF" for default comlink priority.
   16 27 Jul 97 WHO Add command to turn flooding on and off.
   17               Unwise changes, reversed.
   18 16 Oct 97 WHO Don't show stream for channel dump.
   19  9 Nov 98 WHO Add CSDP_ABT to indicate command aborted by operator.
                    Abort detector commands if user enters CR. All locations
                    with calibrator channels.
   20 12 Nov 98 WHO Change get_det so that it validates the entered channel
                    before requesting detector info for it.
   21  9 Nov 99 IGD Porting to SuSE 6.1 LINUX {s
   22 29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#define BUF_SIZE 200
#define WAIT_SECONDS 30
#define CSDP_ABT 100
tclientname name = "CMDS" ;
tservername sname = "RAND" ;
tstations_struc stations ;
chan_struc *pcs = NULL ;
pclient_struc me ;
pclient_station this ;
comstat_rec *pcomm ;
pselarray psa ;
char s1[BUF_SIZE], s2[BUF_SIZE], s3[BUF_SIZE], chanstring[80] ;

pchar seednamestring (seed_name_type *sd, location_type *loc) ;

typedef char char15[16] ;
typedef char char7[8] ;
char15 waves[4] = { "Sine", "Step", "Red Noise", "White Noise" } ;

char7 freqs[Hz0_0005+1] = { "DC", "25.000", "10.000", "8.0000", "6.6667", "5.0000", "4.0000", "3.3333",
               "2.5000", "2.0000", "1.6667", "1.2500", "1.0000", "0.8000", "0.6667", "0.5000", "0.2000",
               "0.1000", "0.0667", "0.0500", "0.0400", "0.0333", "0.0250", "0.0200", "0.0167", "0.0125",
               "0.0100", "0.0050", "0.0020", "0.0010", "0.0005" } ;

char15 outpath[6] = { "Cont-Comlink", "Event-Comlink", "Cont-Tape", "Event-Tape", "Cont-Disk", "Event-Disk" } ;

char7 formats[2] = { "QSL", "Q512" } ;

link_record curlink = { 0, 0, 0, 0, 0, 0, CSF_QSL, 0, 0, 0, 0, 0, 0, 0, 0 } ;
linkstat_rec linkstat = { TRUE, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, 0, 0, 0, CSF_QSL } ;

char7 channels[4] ;
short channel_count ;

/* Convert comlink priority to string, either numeric or "DEF" */
  pchar cl_prio (byte clpr)
    {
      static char s[8] ;
      short m ;
      
      m = clpr ;
      if (clpr == 120)
          strcpy (s, "DEF") ;
        else
          sprintf(s, "%d", m) ;
      return (pchar) &s ;
    }
 
/* Show a bitmap a series of bit number separated by spaces */
  pchar showmap (int32_t map)
    {
      short m ;
      static char s[80] ;
      char s2[80] ;
      
      s[0] = '\0' ;
      for (m = 0 ; m < 32 ; m++)
        if (test_bit(map, m))
            {
              sprintf(s2, "%d ", m) ;
              strcat(s, s2) ;
            }
      return (pchar) &s ;
    }

/* Show waveform bitmap as a series of names separated by spaces */
  pchar showwaves (int32_t map)
    {
      short m ;
      static char s[80] ;
      
      s[0] = '\0' ;
      for (m = SINE ; m <= WRAND ; m++)
        if (test_bit(map, m))
            {
              strcat(s, waves[m]) ;
              strcat(s, "   ") ;
            }
      return (pchar) &s ;
    }

/* Shows a frequency bitmap as a series of frequencies separated by spaces */ 
  void showfreqs (int32_t map)
    {
      short k ;
      char s1[80] ;

      s1[0] = '\0' ;
      for (k = Hz25_000 ; k <= Hz0_0005 ; k++)
      if (test_bit(map, k))
          {
            strcat (s1, freqs[k]) ;
            strcat (s1, " ") ;
            if (strlen(s1) > 65)
                {
                  printf ("      %s\n", s1) ;
                  s1[0] = '\0' ;
                }
          }
      if (s1[0] != '\0')
          printf ("      %s\n", s1) ;
    }

/* Find the lowest bit on in a bitmap */
  short lowest (int32_t cm)
    {
      short i ;
      
      for (i = 0 ; i <= 31 ; i++)
        if (test_bit(cm, i))
            return i + 1 ;
      return 0 ;
    }

/* Find the highest bit on in a bitmap */
  short highest (int32_t cm)
    {
      short i ;
      
      for (i = 31 ; i >= 0 ; i--)
        if (test_bit(cm, i))
            return i + 1 ;
      return 0 ;
    }

/* Show an output bitmap as series of paths separated by spaces */ 
  pchar showout (byte map)
    {
      short k ;
      static char s1[80] ;
      
      s1[0] = '\0' ;
      for (k = 0 ; k <= 5 ; k++)
        if (test_bit(map, k))
            {
              strcat(s1, outpath[k]) ;
              strcat(s1, " ") ;
            }
      return (pchar) &s1 ;
    }

/* Write text equivalent of command status to screen */
  void showerr (short err)
    {
      switch (err)
        {
          case CSCR_ENQUEUE :
            {
              printf ("No Empty Service Queues\n") ;
              break ;
            }
          case CSCR_TIMEOUT :
            {
              printf ("Command not processed by server\n") ;
              break ;
            }
          case CSCR_INIT :
            {
              printf ("Server in initialization\n") ;
              break ;
            }
          case CSCR_REFUSE :
            {
              printf ("Could not attach to server\n") ;
              break ;
            }
          case CSCR_NODATA :
            {
              printf ("The requested data is not available\n") ;
              break ;
            }
          case CSCR_BUSY :
            {
              printf ("Command buffer in use\n") ;
              break ;
            }
          case CSCR_INVALID :
            {
              printf ("Command is not known by server or is invalid for this DA\n") ;
              break ;
            }
          case CSCR_DIED :
            {
              printf ("Server is dead\n") ;
              break ;
            }
          case CSCR_CHANGE :
            {
              printf ("Server has restarted since last service request\n") ;
              break ;
            }
          case CSCR_PRIVATE :
            {
              printf ("Could not create my shared memory segment\n") ;
              break ;
            }
          case CSCR_SIZE :
            {
              printf ("Command buffer is too small\n") ;
              break ;
            }
          case CSCR_PRIVILEGE :
            {
              printf ("That command is privileged\n") ;
              break ;
            }
          case CSCS_IDLE :
            {
              printf ("Command buffer is Idle\n") ;
              break ;
            }
          case CSCS_INPROGRESS :
            {
              printf ("Waiting for response from DA\n") ;
              break ;
            }
          case CSCS_FINISHED :
            {
              printf ("Command response available\n") ;
              break ;
            }
          case CSCS_REJECTED :
            {
              printf ("Command rejected by DA\n") ;
              break ;
            }
          case CSCS_ABORTED :
            {
              printf ("File transfer aborted\n") ;
              break ;
            }
          case CSCS_NOTFOUND :
            {
              printf ("File not found\n") ;
              break ;
            }
          case CSCS_TOOBIG :
            {
              printf ("File is larger than 65K bytes\n") ;
              break ;
            }
          case CSCS_CANT :
            {
              printf ("Cannot create file on DA\n") ;
              break ;
            }
          case CSDP_ABT :
            {
              printf ("Command aborted by Operator\n") ;
              break ;
            }
        }
    }

/* 
   If a command is in progress, wait up to 30 seconds for it to finish, else
   return actual status.
*/
  short wait_finished (comstat_rec *pcomm) 
    {
      short ct ;

      for (ct = 0 ; ct < WAIT_SECONDS ; ct++)
        if (pcomm->completion_status == CSCS_FINISHED)
            return CSCS_FINISHED ;
        else if (pcomm->completion_status == CSCS_INPROGRESS)
            sleep (1) ;
          else
            break ;
      return pcomm->completion_status ;
    }

/* Write the prompt and current value, if user enters CR, return current value, else new value */
  int32_t replace (pchar s, int32_t v)
    {
      int32_t l ;
      char s2[BUF_SIZE] ;
      
      printf ("%s (CR = %d) : ", s, v) ;
      gets_buf (s2, BUF_SIZE) ;
      if (s2[0] != '\0')
          sscanf (s2, "%d", &l) ;
        else
          l = v ;
      return l ;
    }

/* Write the prompt and explanation of CR. CR returns -1, else returns new value */
  short canchange (pchar s)
    {
      int32_t l ;
      char s2[BUF_SIZE] ;
      
      printf ("%s, (CR = No Change) : ", s) ;
      gets_buf (s2, BUF_SIZE) ;
      if (s2[0] != '\0')
          sscanf (s2, "%d", &l) ;
        else
          l = -1 ;
      return (short) l ;
    }
            
/* Gets the channel mapping from the server and stores it in dynamically allocated memory (PCS) */     
  short loadchan (void)
    {
      short err ;
      short *pshort ;
      int32_t size ;

      this->sels[CHAN].first = 0 ;
      this->sels[CHAN].last = 0 ;
      this->command = CSCM_CHAN ;
      err = cs_svc(me, 0) ;
      if (err == CSCR_GOOD)
          {
            pshort = (pvoid) &pcomm->moreinfo ;
            if (pcs)
                free (pcs) ;
            size = *pshort * sizeof(chan_record) + sizeof(short) ;
            pcs = (pvoid) malloc(size) ;
            memcpy ((pchar) pcs, (pchar) &pcomm->moreinfo, size) ;
            pcomm->completion_status = CSCS_IDLE ;
          }
      return err ;
    }

  short getcal (void)
    {
      int sel ;
      short low, high, i, j, k, err ;
      boolean found ;
      cal_record *pcal ;
      eachcal *pe ;
    
      channel_count = 0 ;
      if (pcs == NULL)
          err = loadchan () ;
        else
          err = CSCR_GOOD ;
      if (err == CSCR_GOOD)
          {
            this->command = CSCM_CAL ;
            err = cs_svc(me, 0) ;
            if (err == CSCR_GOOD)
                {
                  pcal = (pvoid) &pcomm->moreinfo ;
                  for (i = 0 ; i < pcal->number ; i++)
                    {
                      pe = &pcal->acal[i] ;
                      if (pe->max_mass_dur > 0)
                          {
                            low = lowest(pe->map) ;
                            high = highest(pe->map) ;
                            s1[0] = '\0' ;
                            for (j = low ; j <= high ; j++)
                                {
                                  k = 0 ;
                                  found = FALSE ;
                                  while ((! found) && (k < pcs->chancount))
                                    {
                                      if (pcs->chans[k].physical == j)
                                          {
                                            if (s1[0] != '\0')
                                                 strcat (s1, ",") ;
                                            strcat(s1, seednamestring(&pcs->chans[k].seedname,
                                                                      &pcs->chans[k].seedloc)) ;
                                            found = TRUE ;
                                          }
                                        else
                                          k++ ;
                                    }
                                }
                            strpcopy (s2, pe->name) ;
                            printf ("Calibrator #%d = %s, Channels %s\n", i + 1, s2, s1) ;
                          }
                    }
                  pcomm->completion_status = CSCS_IDLE ;
                  printf ("Calibrator number [0 to exit] : ") ;
                  gets_buf(s1, BUF_SIZE) ;
                  if (s1[0] == '\0')
                      return 0 ;
                  sscanf (s1, "%i", &sel) ;
                  if ((sel < 1) || (sel > pcal->number))
                      return 0 ; /* exit */
                  pe = &pcal->acal[sel - 1] ;
                  if (pe->max_mass_dur > 0)
                      {
                        low = lowest(pe->map) ;
                        high = highest(pe->map) ;
                        chanstring[0] = '\0' ;
                        for (j = low ; j <= high ; j++)
                            {
                              k = 0 ;
                              found = FALSE ;
                              while ((! found) && (k < pcs->chancount))
                                {
                                  if (pcs->chans[k].physical == j)
                                      {
                                        strcpy(s2, seednamestring(&pcs->chans[k].seedname,
                                                                   &pcs->chans[k].seedloc)) ;
                                        if (chanstring[0] != '\0')
                                            strcat (chanstring, ",") ;
                                        if (channel_count < 4)
                                            strcpy (channels[channel_count++], s2) ;
                                       strcat (chanstring, s2) ;
                                       found = TRUE ;
                                      }
                                    else
                                      k++ ;
                                }
                            }
                      }
                  return sel ;
                }
              else
                {
                  showerr (err) ;
                  return 0 ;
                }
          }
        else
          {
            showerr (err) ;
            return 0 ;
          }
    }

/* Take a string in the form [LL-]SSS and return a string suitable as a selector */
  pchar seedloc (pchar s)
    {
      static char s1[20] ;
      char s2[20] ;

      strcpy (s1, s) ; /* pass by value of strings not supported by C */
      untrail (s1) ;
      upshift (s1) ;
      comserv_split (s1, s2, '-') ;
      if (s2[0] == '\0')
          {
            strcpy (s2, s1) ; /* no location, move to channel, set location to any */
            strcpy (s1, "  ") ;
          }
      while (strlen(s1) < 2)
        strcat(s1, " ") ; /* make sure location is 2 characters */
      s1[2] = '\0' ; /* and no more */
      while (strlen(s2) < 3)
        strcat(s2, " ") ; /* make sure channel is 3 characters */
      s2[3] = '\0' ; /* and no more */
      strcat(s1, s2) ; /* merge location and channel */
      return (pchar) &s1 ;
    }
  
  short get_det (void)
    {
      short err, i ;
      boolean banner ;
      boolean found ;
      char s1[100] ;
      chan_record *pcr ;
      
      if (pcs == NULL)
          err = loadchan () ;
        else
      err = CSCR_GOOD ;
      banner = TRUE ;
      if (err == CSCR_GOOD)
          {
            s1[0] = '\0' ;
            for (i = 0 ; i < pcs->chancount ; i++)
              {
                pcr = &pcs->chans[i] ;
                if (pcr->det_count)
                    {
                      if (banner)
                          {
                            printf ("Channels with detectors are :\n") ;
                            banner = FALSE ;
                          }
                      strcat (s1, seednamestring(&pcr->seedname, &pcr->seedloc)) ;
                      strcat (s1, " ") ;
                      if (strlen(s1) > 70)
                          {
                            printf("%s\n", s1) ;
                            s1[0] = '\0' ;
                          }
                    }
              }
            if (s1[0] != '\0')
                printf ("%s\n", s1) ;
            if (banner)
                return CSCR_NODATA ;
            do
              {
                pcomm->completion_status = CSCS_IDLE ;
                printf ("Seed Specification [LL-]SSS (CR to abort) : ") ;
                gets_buf (s1, BUF_SIZE) ;
                if (s1[0] == '\0')
                    return CSDP_ABT ;
                found = FALSE ;
                for (i = 0 ; i < pcs->chancount ; i++)
                  {
                    pcr = &pcs->chans[i] ;
                    if ((pcr->det_count) &&
#ifdef _OSK
                       (strcasecmp((pchar)s1, seednamestring(&pcr->seedname, &pcr->seedloc)) == 0))
#else
                       (strcasecmp((pchar)&s1, seednamestring(&pcr->seedname, &pcr->seedloc)) == 0))
#endif
                        {
                          found = TRUE ;
                          break ;
                        }
                  }
              }
            while (! found) ;  
            strcpy ((*psa)[1], seedloc (s1)) ; /* make it selector 1 */
            this->sels[CHAN].first = 1 ;
            this->sels[CHAN].last = 1 ;
            this->command = CSCM_DET_REQUEST ;
            err = cs_svc(me, 0) ;
            if (err == CSCR_GOOD)
                {
                  err = wait_finished (pcomm) ;
                  if (err == CSCS_FINISHED)
                      {
                        pcomm->completion_status = CSCS_IDLE ; /* not really done, but close enough */
                        return CSCR_GOOD ;
                      }
                    else
                      return err ;
                }
              else
                return err ;
          }
        else
          return err ;
    }

 
int main (int argc, char *argv[])
{
    short i, j, k, err, high ;
    int sel, sel2, upshmid ;
    boolean found, stopcal, cp_valid, ep_valid, good, enable ;
    pchar pc1, pc2 ;
    linkstat_rec *pls ;
    link_record *plink ;
    digi_record *pdigi ;
    ultra_rec *pur ;
    cal_record *pcal ;
    eachcal *pe ;
    det_request_rec *pdrr ;
    det_descr *pdd ;
    int32_t *plong, *plong2 ;
    short *pshort, *pshort2 ;
    chan_record *pcr, *pcr2 ;
    shell_com *pshell ;
    linkadj_com *padj ;
    recenter_com *prc ;
    cal_start_com *pcsc ;
    det_enable_com *pdec ;
    tdurations *pd ;
    det_change_com *pdcc ;
    client_info *pci ;
    one_client *poc ;
    rec_enable_com *prec ;
    rec_one *pro ;
    download_com *pdc ;
    download_result *pdr ;
    upload_com *puc ;
    tupbuf *pupbuf ;
    upload_result *pupres ;
    comm_event_com *pcec ;
    linkset_com *plsc ;
    boolean *pflood ;
    char7 s[4] ;
    byte widx[4] ;
    byte fidx[32] ;
    byte frq ;
    int32_t ltemp, mtemp ;
    unsigned short cnt ;
    int32_t l ;
    FILE *path ;
    struct stat statbuf ;

/* Allow override of station name on command line */
    if (argc >= 2)
    {
	strncpy(sname, argv[1], SERVER_NAME_SIZE) ;
	sname[SERVER_NAME_SIZE-1] = '\0' ;
    }
    upshift(sname) ;

/* Generate an entry for the station */      
    cs_setup (&stations, name, sname, TRUE, TRUE, 0, 2, CSIM_MSG, 5000) ;

    if (stations.station_count == 0)
    {
	printf ("Station not found\n") ;
	exit (3) ;
    }
 
/* Create my segment and attach to the selected station */      
    me = cs_gen (&stations) ;
    this = (pclient_station) ((uintptr_t) me + me->offsets[0]) ;
    this->command = CSCM_LINKSTAT ;
    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
    pls = (pvoid) &pcomm->moreinfo ;
    pcomm->completion_status = CSCS_IDLE ;
    psa = (pselarray) ((uintptr_t) me + this->seloffset) ;
    if (cs_svc(me, 0) == CSCR_GOOD)
	memcpy ((pchar) &linkstat, (pchar) &pcomm->moreinfo, sizeof(linkstat_rec)) ;
    pcomm->completion_status = CSCS_IDLE ;
    this->command = CSCM_LINK ;
    err = cs_svc(me, 0) ;
    if (err == CSCR_GOOD)
	memcpy ((pchar) &curlink, (pchar) &pcomm->moreinfo, sizeof(link_record)) ;
    pcomm->completion_status = CSCS_IDLE ;
      
    do
    {
	printf ("Edition 20 Station %s--------------------------------------------------\n", sname) ;
	printf (" 0=Exit                   1=Link Status            2=Link Format\n") ;
	printf (" 3=Digitizer Info         4=Ultra Info             5=Calibrator Info\n") ;
	printf (" 6=Channel Info           7=Detector Info          8=Client Info\n") ;
	printf (" 9=Unblock packets       10=Reconfigure Link      11=Suspend Link\n") ;
	printf ("12=Resume Link           13=Command Clear         14=Command Status\n") ;
	printf ("15=Terminate Server      16=Shell Command         17=Set VCO Value\n") ;
	printf ("18=Change Link Settings  19=Mass Recenter         20=Calibrate\n") ;
	printf ("21=Detector Enable       22=Detector Change       23=Recording Enables\n") ;
	printf ("24=Comm Event            25=Xfer File to Host     26=Xfer File to DA\n") ;
	printf ("27=Server Link Settings  28=Flooding Control\n") ;
	printf ("Command : ") ;
	if (gets_buf (s1, BUF_SIZE) == NULL)
	    break ;
	sscanf (s1, "%i", &sel) ;
	if (sel == 0)
	    break ;
	printf ("-------------------------------------------------------------------------\n") ;
	switch (sel)
	{
	case 1 :
	{
	    this->command = CSCM_LINKSTAT ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		memcpy ((pchar) &linkstat, (pchar) &pcomm->moreinfo, sizeof(linkstat_rec)) ;
		pls = (pvoid) &pcomm->moreinfo ;
		printf ("  Ultra=%d, Link Recv=%d, Ultra Recv=%d, Suspended=%d, Data Format=%s\n",
			pls->ultraon, pls->linkrecv, pls->ultrarecv, pls->suspended,
			(char *)formats[pls->data_format]) ;
		printf ("  Total Packets=%d, Sync Packets=%d, Sequence Errors=%d\n",
			pls->total_packets, pls->sync_packets, pls->seq_errors) ;
		printf ("  Checksum Errors=%d, IO Errors=%d, Last IO Error=%d\n",
			pls->check_errors, pls->io_errors, pls->lastio_error) ;
		printf ("  Blocked Packets=%d, Seed Format=%5.5s, Seconds in Operation=%d\n",
			pls->blocked_packets, pls->seedformat, pls->seconds_inop) ;
		if (pls->last_good > 1)
		    printf ("  Last Good Packet Received at=%s\n", time_string(pls->last_good)) ;
		if (pls->last_bad > 1)
		    printf ("  Last Bad Packet Received at=%s\n", time_string(pls->last_bad)) ;
		strpcopy (s1, pls->description) ;
		printf ("  Station Description=%s\n", s1) ;
		printf ("  Polling=%dus, Reconfig=%d, Net Timeout=%d, Net Polling=%d\n",
			pls->pollusecs, pls->reconcnt, pls->net_idle_to, pls->net_conn_dly) ;
		printf ("  Group Size=%d, Group Timeout=%d\n", pls->grpsize, pls->grptime) ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 2 :
	{
	    this->command = CSCM_LINK ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		memcpy ((pchar) &curlink, (pchar) &pcomm->moreinfo, sizeof(link_record)) ;
		plink = (pvoid) &pcomm->moreinfo ;
		printf ("  Window Size=%d, Total priority levels=%d, Message Priority=%d\n",
			plink->window_size, plink->total_prio - 1, plink->msg_prio) ;
		printf ("  Detection Priority=%d, Timing Priority=%d, Cal Priority=%d, CmdEcho=%d\n",
			plink->det_prio, plink->time_prio, plink->cal_prio, plink->rcecho) ;
		printf ("  Resend Time=%d  Sync Time=%d  Resend Pkts=%d  Net Restart Dly=%d\n",
			plink->resendtime, plink->synctime, plink->resendpkts, plink->netdelay) ;
		printf ("  Net Conn. Time=%d  Net Packet Limit=%d  Group Pkt Size=%d  Group Timeout=%d\n",
			plink->nettime, plink->netmax, plink->groupsize, plink->grouptime) ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 3 :
	{
	    this->command = CSCM_DIGI ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		pdigi = (pvoid) &pcomm->moreinfo ;
		strpcopy (s1, pdigi->name) ;
		strpcopy (s2, pdigi->version) ;
		strpcopy (s3, pdigi->clockmsg) ;
		printf ("  Digitizer=%s, Version=%s\n", s1, s2) ;
		printf ("  Clock Message=%s\n", s3) ;
		printf ("  Prefilter OK=%d, Detector Load OK=%d, Set Map OK=%d\n",
			pdigi->prefilter_ok, pdigi->detector_load_ok, pdigi->setmap_ok) ;
		printf ("  Clock String OK=%d, Int/Ext OK=%d, Send Message OK=%d\n",
			pdigi->clockstring_ok, pdigi->int_ext_ok, pdigi->send_message_ok) ;
		printf ("  Message Channel OK=%d, Set OSC OK=%d, Set Clock OK=%d\n",
			pdigi->message_chan_ok, pdigi->set_osc_ok, pdigi->set_clock_ok) ;
		printf ("  Wait For data Seconds=%d\n", pdigi->wait_for_data) ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 4 :
	{
	    this->command = CSCM_ULTRA ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pur = (pvoid) &pcomm->moreinfo ;
		printf ("  VCO Value=%d, PLL ON=%d, Mass Rec. OK=%d, Revision=%d\n",
			pur->vcovalue, pur->pllon, pur->umass_ok, pur->ultra_rev) ;
		s1[0] = '\0' ;
		pc1 = (pvoid) &pur->commnames ;
		if (pc1[0] != '\0')
		{
		    for (i = 0 ; i < CE_MAX ; i++)
		    {
			strpcopy (s2, pc1) ;
			pc1 = (pchar) ((uintptr_t) pc1 + *pc1 + 1) ;
			strcat (s1, s2) ;
			if (test_bit(pur->comm_mask, i))
			    strcat (s1, "+") ;
			else
			    strcat (s1, "-") ;
			strcat (s1, " ") ;
			if (strlen(s1) > 70)
			{
			    printf ("  %s\n", s1) ;
			    s1[0] = '\0' ;
			}
		    }
		    if (s1[0] != '\0')
			printf ("  %s\n", s1) ;
		}
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 5 :
	{
	    this->command = CSCM_CAL ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pcal = (pvoid) &pcomm->moreinfo ;
		printf ("  Number of calibrators=%d, Mass Rec. OK=%d\n",
			pcal->number, pcal->mass_ok) ;
		for (i = 0 ; i < pcal->number ; i++)
		{
		    pe = &pcal->acal[i] ;
		    printf ("  Calibrator number %d\n", i + 1) ;
		    printf ("    Coupling Option=%d, Polarity Option=%d, Board=%d\n",
			    pe->coupling_option, pe->polarity_option, pe->board) ;
		    printf ("    Min Settle=%d, Max Settle=%d, Settle Increment=%d\n",
			    pe->min_settle, pe->max_settle, pe->inc_settle) ;
		    printf ("    Min Mass=%d, Max Mass=%d, Mass Increment=%d, Mass Default=%d\n",
			    pe->min_mass_dur, pe->max_mass_dur, pe->inc_mass_dur, pe->def_mass_dur) ;
		    printf ("    Min Filter=%d, Max Filter=%d\n",
			    pe->min_filter, pe->max_filter) ;
		    printf ("    Min Amplitude=%d, Max Amplitude=%d, Amplitude Step=%d\n",
			    pe->min_amp, pe->max_amp, pe->amp_step) ;
		    printf ("    Monitor Chan=%d, Min Random Period=%d, Max Random Period=%d\n",
			    pe->monitor, pe->rand_min_period, pe->rand_max_period) ;
		    printf ("    Default Step Filter=%d, Default Random Period=%d\n",
			    pe->default_step_filt, pe->default_rand_filt) ;
		    for (k = SINE ; k <= WRAND ; k++)
		    {
			printf ("    %s Periods\n", (char *)waves[k]) ;
			printf ("      Minimum=%d, Maximum=%d, Increment=%d\n",
				pe->durations[k].min_dur, pe->durations[k].max_dur, pe->durations[k].inc_dur) ;
		    }
		    printf ("    Channel Map=%s\n", showmap(pe->map)) ;
		    printf ("    Waveforms supported=%s\n", showwaves(pe->waveforms)) ;
		    printf ("    Sine wave frequencies supported : \n") ;
		    showfreqs (pe->sine_freqs) ;
		    for (k = 0 ; k < MAXCAL_FILT ; k++)
		    {
			printf ("    Default Sine frequencies for filter number %d\n", k + 1) ;
			showfreqs (pe->default_sine_filt[k]) ;
		    }
		    strpcopy (s1, pe->name) ;
		    strpcopy (s2, pe->filtf) ;
		    printf ("    Calibrator Name=%s\n", s1) ;
		    printf ("    Filter Description=%s\n", s2) ;
		}
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 6 :
	{
	    err = loadchan () ;
	    if (err == CSCR_GOOD)
	    {
		for (i = 0 ; i < pcs->chancount ; i++)
		{
		    pcr = &pcs->chans[i] ;
		    printf ("    %s Physical=%d, Detectors=%d", 
			    seednamestring (&pcr->seedname, &pcr->seedloc), 
			    pcr->physical, pcr->det_count) ;
		    if ((pcr->available & 1) != 0)
			printf (", Cprio=%s", cl_prio(pcr->c_prio)) ;
		    if ((pcr->available & 2) != 0)
			printf (", Eprio=%s", cl_prio(pcr->e_prio)) ;
		    printf (", Rate=") ;
		    if (pcr->rate > 0)
			printf ("%4.3f\n", 1.0 * pcr->rate) ;
		    else
			printf ("%4.3f\n", 1.0 / (0 - pcr->rate)) ;
		    printf ("      Available Outputs=%s\n", showout(pcr->available)) ;
		    printf ("      Enabled   Outputs=%s\n", showout(pcr->enabled)) ;
		}
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 7 :
	{
	    err = get_det () ;
	    if (err == CSCR_NODATA)
	    {
		printf ("No detectors available\n") ;
		break ;
	    }
	    if (err != CSCR_GOOD)
	    {
		showerr (err) ;
		break ;
	    }
	    pdrr = (pvoid) &pcomm->moreinfo ;
	    printf ("%d Detectors found\n", pdrr->count) ;
	    for (i = 0 ; i < pdrr->count ; i++)
	    {
		pdd = &pdrr->dets[i] ;
		strpcopy (s1, pdd->name) ;
		strpcopy (s2, pdd->params[0]) ;
		printf ("Detector %s, Type=%s, Enabled=%d, Remote=%d, ID=%d\n",
			s1, s2, pdd->enabled, pdd->remote, pdd->id) ;
		s1[0] = '\0' ;
		plong = (pvoid) &pdd->cons ;
		for (j = 1 ; j < 12 ; j++)
		{
		    if ((pdd->params[j])[0] != '\0')
		    {
			strpcopy (s2, pdd->params[j]) ;
			strcat (s1, s2) ;
			strcat (s1, "=") ;
			if (j == 11)
			{
			    pshort = (pvoid) plong ;
			    sprintf(s2, "%d", *pshort) ;
			}
			else
			    sprintf(s2, "%d", *plong) ;
			strcat (s1, s2) ;
			strcat (s1, " ") ;
			if (strlen(s1) > 70)
			{
			    printf("  %s\n", s1) ;
			    s1[0] = '\0' ;
			}
		    }
		    plong++ ;
		}
		if (s1[0] != '\0')
		    printf("  %s\n", s1) ;
	    }
	    break ;                             
	}
	case 8 : /* Client info */
	{
	    this->command = CSCM_CLIENTS ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pci = (pvoid) &pcomm->moreinfo ;
		for (i = 0 ; i < pci->client_count ; i++)
		{
		    poc = &(pci->clients[i]) ;
		    printf ("Client Number=%d Name=%s, Memory ID=%d, Process ID=%d, Active=%d\n",
			    i + 1, cname_str_cs(poc->client_name), poc->client_memid, 
			    poc->client_pid, poc->active) ;
		    printf ("    Timeout=%d, Packets Blocked=%d, Blocking=%d, Reserved=%d\n",
			    poc->timeout, poc->block_count, poc->blocking, poc->reserved) ;
		    printf ("    Last Service=%s\n", time_string(poc->last_service)) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 9 :
	{
	    printf ("Unblock packets from Client Number : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if (s1[0] == '\0')
		break ;
	    sscanf (s1, "%i", &sel2) ;
	    if ((sel2 < 1) || (sel2 > MAXCLIENTS))
	    {
		printf ("Invalid client number\n") ;
		break ;
	    }
	    pshort = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    *pshort = sel2 - 1 ;
	    this->command = CSCM_UNBLOCK ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		printf ("  Packets unblocked\n") ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 10 :
	{
	    this->command = CSCM_RECONFIGURE ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		printf ("  Reconfiguration in progress\n") ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 11 :
	{
	    this->command = CSCM_SUSPEND ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		printf ("  Link suspended\n") ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 12 :
	{
	    this->command = CSCM_RESUME ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		printf ("  Link resumed\n") ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 13 :
	{
	    this->command = CSCM_CMD_ACK ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
		printf ("  Command Status Cleared\n") ;
	    else
		showerr (err) ;
	    break ;
	}
	case 14 :
	{
	    showerr (pcomm->completion_status) ;
	    break ;
	}
	case 15 :
	{
	    this->command = CSCM_TERMINATE ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
		printf ("Server Terminated\n") ;
	    else
		showerr (err) ;
	    break ;
	}
	case 16 :
	{
	    this->command = CSCM_SHELL ;
	    pshell = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    pshell->log_local = FALSE ;
	    pshell->log_host = FALSE ;
	    printf ("Shell: ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if (s1[0] == '\0')
		break ;
	    strpas (pshell->shell_parameter, s1) ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("Shell Command Sent\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 17 :
	{
	    this->command = CSCM_VCO ;
	    pshort = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    printf ("VCO Value (0-4095, -1=PLL) : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if (s1[0] == '\0')
		break ;
	    sscanf (s1, "%i", &sel) ;
	    *pshort = sel ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("VCO value sent\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 18 :
	{
	    this->command = CSCM_LINKADJ ;
	    padj = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    padj->window_size = replace ("Window Size", curlink.window_size) ;
	    padj->set_msg = replace ("Message Priority", curlink.msg_prio) ;
	    padj->set_det = replace ("Detection Priority", curlink.det_prio) ;
	    padj->set_time = replace ("Timing Priroity", curlink.time_prio) ;
	    padj->set_calp = replace ("Calibration Priority", curlink.cal_prio) ;
	    padj->resendtime = replace ("Resend Timeout", curlink.resendtime) ;
	    padj->synctime = replace ("Sync Packet Interval", curlink.synctime) ;
	    padj->resendpkts = replace ("Packets in Resend Block", curlink.resendpkts) ;
	    padj->netdelay = replace ("Network Restart Delay", curlink.netdelay) ;
	    padj->nettime = replace ("Network Connect Timeout", curlink.nettime) ;
	    padj->netmax = replace ("Network Packet Limit", curlink.netmax) ;
	    padj->groupsize = replace ("Packets in Group", curlink.groupsize) ;
	    padj->grouptime = replace ("Group Timeout", curlink.grouptime) ;
	    padj->lasp1 = 0 ;
	    padj->lasp2 = 0 ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("New Link parameters sent\n") ;
		    curlink.window_size = padj->window_size ;
		    curlink.msg_prio = padj->set_msg ;
		    curlink.det_prio = padj->set_det ;
		    curlink.time_prio = padj->set_time ;
		    curlink.cal_prio = padj->set_calp ;
		    curlink.resendtime = padj->resendtime ;
		    curlink.synctime = padj->synctime ;
		    curlink.resendpkts = padj->resendpkts ;
		    curlink.netdelay = padj->netdelay ;
		    curlink.nettime = padj->nettime ;
		    curlink.netmax = padj->netmax ;
		    curlink.groupsize = padj->groupsize ;
		    curlink.grouptime = padj->grouptime ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 19 :
	{
	    sel = getcal () ;
	    if (sel == 0)
		break ;
	    pcal = (pvoid) &pcomm->moreinfo ;
	    pe = &pcal->acal[sel-1] ;
	    if (pe->max_mass_dur == 0)
		break ;
	    if (pe->min_mass_dur != pe->max_mass_dur)
	    {
		printf ("Duration in ms [%d-%d CR=%d] : ", pe->min_mass_dur, pe->max_mass_dur, pe->def_mass_dur) ;
		gets_buf(s1, BUF_SIZE) ;
		if (s1[0] == '\0')
		    k = pe->def_mass_dur ;
		else
		{
		    sscanf (s1, "%i", &sel2) ;
		    k = sel2 ;
		}
	    }
	    else
		k = pe->def_mass_dur ;
	    if ((k < pe->min_mass_dur) || (k > pe->max_mass_dur))
		printf ("Invalid duration\n") ;
	    else
	    {
		prc = (pvoid) ((uintptr_t) me + this->cominoffset) ;
		prc->board = sel ;
		prc->duration = k ;
		this->command = CSCM_MASS_RECENTER ;
		err = cs_svc(me, 0) ;
		if (err == CSCR_GOOD)
		{
		    err = wait_finished (pcomm) ;
		    if (err == CSCS_FINISHED)
		    {
			printf ("Mass Recentering Command sent\n") ;
			pcomm->completion_status = CSCS_IDLE ;
		    }
		    else
			showerr (err) ;
		}
		else
		    showerr (err) ;
	    }
	    break ;
	} 
	case 20 :
	{
	    sel = getcal () ;
	    if (sel == 0)
		break ;
	    pcal = (pvoid) &pcomm->moreinfo ;
	    pe = &pcal->acal[sel-1] ;
	    high = highest(pe->map) ;
	    pcsc = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    pcsc->calnum = sel ;
	    stopcal = FALSE ;
	    do
	    {
		pcsc->map = 0 ;
		printf ("Channels to Calibrate [i.e. %s, CR=Stop Cal] : ", chanstring) ;
		gets_buf (s1, BUF_SIZE) ;
		if (s1[0] == '\0')
		{
		    pshort = (pvoid) pcsc ;
		    *pshort = sel ;
		    this->command = CSCM_CAL_ABORT ;
		    err = cs_svc(me, 0) ;
		    if (err == CSCR_GOOD)
		    {
			err = wait_finished (pcomm) ;
			if (err == CSCS_FINISHED)
			{
			    printf ("Calibrate Abort Command sent\n") ;
			    pcomm->completion_status = CSCS_IDLE ;
			}
			else
			    showerr (err) ;
		    }
		    else
			showerr (err) ;
		    stopcal = TRUE ;
		    break ;
		}
		err = FALSE ;
		i = 0 ;
		while (s1[i] != '\0')
		{
		    if (s1[i] == ',')
			s1[i] = ' ' ;
		    i++ ;
		}
		for (i = 0 ; i < 4 ; i++)
		    s[i][0] = '\0' ;
		sscanf (s1, "%s%s%s%s", s[0], s[1], s[2], s[3]) ;
		for (i = 0 ; i < 4 ; i++)
		{
		    if (s[i][0] == '\0')
			break ;
		    found = FALSE ;
		    for (j = 0 ; j < channel_count ; j++)
			if (strcasecmp((pchar)&channels[j], (pchar)&s[i]) == 0)
			{
			    pcsc->map = pcsc->map | (1 << j) ;
			    found = TRUE ;
			    break ;
			}
		    if (! found)
			err = TRUE ;
		}
	    }
	    while (err) ;
	    if ((stopcal) || (pcsc->map == 0))
		break ;
	    pcsc->plus = FALSE ;
	    pcsc->capacitor = FALSE ;
	    pcsc->settle = 1 ;
	    pcsc->autoflag = 0 ;
	    pcsc->ext_sp1 = 0 ;
	    pcsc->ext_sp2 = 0 ;
	    pcsc->filt = pe->min_filter ;
	    if (pe->coupling_option)
	    {
		printf ("Capacitive or Resistive Coupling (C/*R) : ") ;
		gets_buf(s1, BUF_SIZE) ;
		pcsc->capacitor = ((s1[0] == 'C') || (s1[0] == 'c')) ;
	    }
	    if (pe->min_amp == pe->max_amp)
	    {
		pcsc->amp = pe->max_amp ;
		printf ("Amplitude fixed at %d dB\n", pe->max_amp) ;
	    }
	    else
		do
		{
		    printf ("Amplitude [%d to %d dB in %d dB steps] : ",
                            pe->min_amp, pe->max_amp, pe->amp_step) ;
		    gets_buf(s1, BUF_SIZE) ;
		    sscanf (s1, "%i", &sel2) ;
		    pcsc->amp = sel2 ;
		}
		while ((sel2 > pe->max_amp) || (sel2 < pe->min_amp) ||
		       (((0 - pcsc->amp) % pe->amp_step) != 0)) ;
	    do
	    {
		i = 0 ;
		for (k = SINE ; k <= WRAND ; k++)
		    if (test_bit(pe->waveforms, k))
		    {
			widx[i++] = k ;
			printf ("%d=%s ", i, (char *)waves[k]) ;
		    }
		if (i == 1)
		{
		    k = widx[0] ;
		    printf ("Waveform is %s\n", (char *)waves[k]) ;
		    break ;
		}
		else
		{
		    printf (": ") ;
		    gets_buf(s1, BUF_SIZE) ;
		    sscanf (s1, "%i", &sel2) ;
		    j = sel2 ;
		    if ((j > 0) & (j <= i))
		    {
			k = widx[j - 1] ;
			break ;
		    }
		}
	    }
	    while (1) ;
	    pcsc->calcmd = k ;
	    switch (k)
	    {
	    case STEP :
	    {
		if (pe->polarity_option)
		{
		    printf ("Polarity of step [*P/N] : ") ;
		    gets_buf(s1, BUF_SIZE) ;
		    pcsc->plus = ! ((s1[0] == 'N') || (s1[0] == 'n')) ;
		}
		else
		    pcsc->plus = TRUE ;
		pcsc->filt = pe->default_step_filt ;
		break ;
	    }
	    case SINE :
	    {
		i = 0 ;
		for (frq = Hz25_000 ; frq <= Hz0_0005 ; frq++)
		    if (test_bit(pe->sine_freqs, frq))
		    {
			fidx[i++] = frq ;
			printf ("%2d=%sHz ", i, (char *)&(freqs[frq])) ;
			if ((i % 6) == 0)
			    printf ("\n") ;
		    }
		if ((i % 6) != 0)
		    printf ("\n") ;
		do
		{
		    printf ("Sine frequency number [1-%d] : ", i) ;
		    gets_buf(s1, BUF_SIZE) ;
		    sscanf (s1, "%i", &sel2) ;
		}
		while ((sel2 < 1) || (sel2 > i)) ;
		pcsc->sfrq = fidx[sel2 - 1] ;
		for (i = 0 ; i < MAXCAL_FILT ; i++)
		    if (test_bit(pe->default_sine_filt[i], pcsc->sfrq))
		    {
			pcsc->filt = i + 1 ;
			break ;
		    }
		break ;
	    }
	    case RAND : ;
	    case WRAND :
	    {
		if (pe->rand_min_period != pe->rand_max_period)
		{
		    do
		    {
			printf ("Period multiplier [%d-%d] : ",
				pe->rand_min_period, pe->rand_max_period) ;
			gets_buf(s1, BUF_SIZE) ;
			sscanf (s1, "%i", &sel2) ;
			pcsc->rmult = sel2 ;
		    }
		    while ((pcsc->rmult < pe->rand_min_period) || 
			   (pcsc->rmult > pe->rand_max_period)) ;
		}
		else
		    pcsc->rmult = pe->rand_min_period ;
		pcsc->filt = pe->default_rand_filt ;
	    }
	    }
	    if (pe->max_filter > 0)
	    {
		j = pcsc->filt ;
		do
		{
		    strpcopy (s2, pe->filtf) ;
		    printf ("3dB Filter cutoff %s [*%d] : ", s2, j) ;
		    gets_buf(s1, BUF_SIZE) ;
		    if (s1[0] != '\0')
		    {
			sscanf (s1, "%i", &sel2) ;
			pcsc->filt = sel2 ;
		    }
		}
		while ((pcsc->filt < pe->min_filter) || (pcsc->filt > pe->max_filter)) ;
	    }
	    pd = &pe->durations[k] ;
	    do
	    {
		if (pd->inc_dur == 60)
		{
		    printf ("Duration in minutes [%d-%d] : ", pd->min_dur / 60, 
			    pd->max_dur / 60) ;
		    gets_buf(s1, BUF_SIZE) ;
		    if (strcasecmp((pchar)&s1, "MAX") == 0)
		    {
			pcsc->duration = 0 ;
			break ;
		    }
		    sscanf (s1, "%d", &sel2) ;
		    pcsc->duration = sel2 * 60 ;
		}
		else
		{
		    printf ("Duration in seconds [%d-%d] : ", pd->min_dur, pd->max_dur) ;
		    gets_buf(s1, BUF_SIZE) ;
		    if (strcasecmp((pchar)&s1, "MAX") == 0)
		    {
			pcsc->duration = 0 ;
			break ;
		    }
		    sscanf (s1, "%d", &sel2) ;
		    pcsc->duration = sel2 ;
		}
	    }
	    while ((pcsc->duration < pd->min_dur) || (pcsc->duration > pd->max_dur)) ;
	    if (pe->inc_settle == 1)
		do
		{
		    printf ("Relay settling delay in seconds [%d-%d] : ",
                            pe->min_settle, pe->max_settle) ;
		    gets_buf(s1, BUF_SIZE) ;
		    sscanf (s1, "%d", &sel2) ;
		    pcsc->settle = sel2 ;
		}
		while ((pcsc->settle < pe->min_settle) || (pcsc->settle > pe->max_settle)) ;
	    else if (pe->inc_settle == 60)
		do
		{            
		    printf ("Relay settling delay in minutes [%d-%d] : ",
                            pe->min_settle / 60, pe->max_settle / 60) ;
		    gets_buf(s1, BUF_SIZE) ;
		    sscanf (s1, "%d", &sel2) ;
		    pcsc->settle = sel2 * 60 ;
		}
		while ((pcsc->settle < pe->min_settle) || (pcsc->settle > pe->max_settle)) ;
	    this->command = CSCM_CAL_START ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("Calibration Start Command sent\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 21 :
	{
	    pdec = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    pdec->count = 0 ;
	    printf ("You must know the detector ID to use this command. Use command 7 to obtain IDs\n") ;
	    while (pdec->count < 20)
	    {
		printf ("ID of detector [CR when done] : ") ;
		gets_buf (s1, BUF_SIZE) ;
		if (s1[0] == '\0')
		    break ;
		sscanf (s1, "%i", &sel2) ;
		pdec->detectors[pdec->count].detector_id = sel2 ;
		pdec->detectors[pdec->count].de_sp1 = 0 ;
		printf ("Enable Detector [Y/N] : ") ;
		gets_buf (s1, BUF_SIZE) ;
		pdec->detectors[pdec->count++].enable = ((s1[0] == 'Y') || (s1[0] == 'y')) ;
	    }
	    if (pdec->count)
	    {
		this->command = CSCM_DET_ENABLE ;
		err = cs_svc(me, 0) ;
		if (err == CSCR_GOOD)
		{
		    err = wait_finished (pcomm) ;
		    if (err == CSCS_FINISHED)
		    {
			printf ("Detector Enable Command sent\n") ;
			pcomm->completion_status = CSCS_IDLE ;
		    }
		    else
			showerr (err) ;
		}
		else
		    showerr (err) ;
	    }
	    break ;
	}
	case 22 :
	{
	    err = get_det () ;
	    if (err == CSCR_NODATA)
	    {
		printf ("No detectors available\n") ;
		break ;
	    }
	    if (err != CSCR_GOOD)
	    {
		showerr (err) ;
		break ;
	    }
	    pdrr = (pvoid) &pcomm->moreinfo ;
	    printf ("%d Detectors found\n", pdrr->count) ;
	    for (i = 0 ; i < pdrr->count ; i++)
	    {
		pdd = &pdrr->dets[i] ;
		strpcopy (s1, pdd->name) ;
		strpcopy (s2, pdd->params[0]) ;
		printf ("Detector %s, Type=%s, Enabled=%d, Remote=%d, ID=%d\n",
			s1, s2, pdd->enabled, pdd->remote, pdd->id) ;
		s1[0] = '\0' ;
	    }
	    printf ("Detector ID to Change : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if (s1[0] == '\0')
		break ;
	    sscanf (s1, "%i", &sel2) ;
	    found = FALSE ;
	    for (i = 0 ; i < pdrr->count ; i++)
	    {
		pdd = &pdrr->dets[i] ;
		if (pdd->id == sel2)
		{
		    found = TRUE ;
		    break ;
		}
	    }
	    if (! found)
	    {
		printf ("Detector ID not found\n") ;
		break ;
	    }
	    pdcc = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    pdcc->id = sel2 ;
	    pdcc->dct_sp = 0 ;
	    printf ("Enable Detector [Y/N] : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    pdcc->enab = ((s1[0] == 'Y') || (s1[0] == 'y')) ;
	    plong = (pvoid) &pdd->cons ;
	    plong2 = (pvoid) &pdcc->ucon ;
	    for (j = 1 ; j < 12 ; j++)
	    {
		if ((pdd->params[j])[0] != '\0')
		{
		    strpcopy (s2, pdd->params[j]) ;
		    if (j == 11)
		    {
			pshort = (pvoid) plong ;
			pshort2 = (pvoid) plong2 ;
			*pshort2 = replace(s2, *pshort) ;
		    }
		    else
			*plong2 = replace (s2, *plong) ;
		}
		plong++ ;
		plong2++ ;
	    }
	    this->command = CSCM_DET_CHANGE ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("Detector Change Command sent\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 23 :
	{
	    this->command = CSCM_LINK ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		plink = (pvoid) &pcomm->moreinfo ;
		high = plink->total_prio - 1 ;
	    }
	    else
	    {
		showerr (err) ;
		break ;
	    }
	    pcomm->completion_status = CSCS_IDLE ;
	    err = loadchan () ;
	    if (err != CSCR_GOOD)
	    {
		showerr (err) ;
		break ;
	    }
	    prec = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    prec->count = 0 ;
	    while (1)
	    {
		if (prec->count == 8)
		    break ;
		printf ("Channel specification to change ([LL-]SSS or CR if done) : ") ;
		gets_buf(s1, BUF_SIZE) ;
		upshift(s1) ;
		if (s1[0] == '\0')
		    break ;
		found = FALSE ;
		for (j = 0 ; j < pcs->chancount ; j++)
		{
		    pcr = &pcs->chans[j] ;
		    if (strcmp(s1, seednamestring(&pcr->seedname, &pcr->seedloc)) == 0)
		    {
			found = TRUE ;
			break ;
		    }
		}
		if (found)
		{
		    pro = &prec->changes[prec->count++] ;
		    memcpy(pro->seedname, pcr->seedname, 3) ;
		    memcpy(pro->seedloc, pcr->seedloc, 2) ;
		    ltemp = 0 ;
		    for (k = 0 ; k <= 5 ; k++)
			if (test_bit(pcr->available, k))
			{
			    if (test_bit(pcr->enabled, k))
				strcpy (s2, "Y") ;
			    else
				strcpy (s2, "N") ;
			    printf ("Enable %s (Y/N, CR=%s) : ", (char *)&(outpath[k]), s2) ;
			    gets_buf(s1, BUF_SIZE) ;
			    if (s1[0] == '\0')
				strcpy(s1, s2) ;
			    if ((s1[0] == 'Y') || (s1[0] == 'y'))
				set_bit (&ltemp, k) ;
			}
		    pro->mask = ltemp ;
		    pro->c_prio = pcr->c_prio ;
		    pro->e_prio = pcr->e_prio ;
		    cp_valid = ((pcr->available & 1) != 0) ;
		    ep_valid = ((pcr->available & 2) != 0) ;
		    if (cp_valid)
		    {
			while (1)
			{
			    printf ("Continuous priority (CR = %s) : ", cl_prio(pcr->c_prio)) ;
			    gets_buf (s2, BUF_SIZE) ;
			    if (s2[0] != '\0')
				if (strcasecmp((pchar) s2, "DEF") == 0)
				    l = 120 ;
				else
				    sscanf (s2, "%d", &l) ;
			    else
				l = pcr->c_prio ;
			    i = l ;
			    good = (((i >= 1) && (i <= high)) || (i == 120)) ;
			    if (cp_valid && ep_valid)
				for (j = 0 ; j < pcs->chancount ; j++)
				{
				    pcr2 = &pcs->chans[j] ;
				    if ((pcr2->stream != pcr->stream) &&
					((pcr2->c_prio == i) || (pcr2->e_prio == i)))
				    {
					good = FALSE ;
					break ;
				    }
				}
			    if (good)
			    {
				pro->c_prio = i ;
				break ;
			    }
			    else
				printf ("Invalid priority value\n") ;
			}
		    }
		    if (ep_valid)
		    {
			while (1)
			{
			    printf ("Event priority (CR = %s) : ", cl_prio(pcr->e_prio)) ;
			    gets_buf (s2, BUF_SIZE) ;
			    if (s2[0] != '\0')
				if (strcasecmp((pchar) s2, "DEF") == 0)
				    l = 120 ;
				else
				    sscanf (s2, "%d", &l) ;
			    else
				l = pcr->e_prio ;
			    i = l ;
			    good = (((i >= 1) && (i <= high)) || (i == 120)) ;
			    if (cp_valid && ep_valid)
				for (j = 0 ; j < pcs->chancount ; j++)
				{
				    pcr2 = &pcs->chans[j] ;
				    if ((pcr2->stream != pcr->stream) &&
					((pcr2->c_prio == i) || (pcr2->e_prio == i)))
				    {
					good = FALSE ;
					break ;
				    }
				}
			    if (good)
			    {
				pro->e_prio = i ;
				break ;
			    }
			    else
				printf ("Invalid priority value\n") ;
			}
		    }
		}
	    }
	    if (prec->count)
	    {
		this->command = CSCM_REC_ENABLE ;
		err = cs_svc(me, 0) ;
		if (err == CSCR_GOOD)
		{
		    err = wait_finished (pcomm) ;
		    if (err == CSCS_FINISHED)
		    {
			printf ("Recording Enable Command sent\n") ;
			pcomm->completion_status = CSCS_IDLE ;
		    }
		    else
			showerr (err) ;
		}
		else
		    showerr (err) ;
	    }
	    break ;
	}
	case 24 :
	{
	    this->command = CSCM_ULTRA ;
	    err = cs_svc(me, 0) ;
	    if (err != CSCR_GOOD)
	    {
		showerr (err) ;
		break ;
	    }
	    printf ("Follow Comm Event name with + to enable, - to disable\n") ;
	    pcomm->completion_status = CSCS_IDLE ;
	    pur = (pvoid) &pcomm->moreinfo ;
	    ltemp = 0 ;
	    mtemp = 0 ;
	    while (1)
	    {
		printf ("Comm Event name to Change (CR to stop) : ") ;
		gets_buf (s1, BUF_SIZE) ;
		untrail(s1) ;
		if (s1[0] == '\0')
		    break ;
		upshift(s1) ;
		found = FALSE ;
		i = strlen(s1) - 1 ;
		enable = (s1[i] == '+') ;
		s1[i] = '\0' ;
		pc1 = (pvoid) &pur->commnames ;
		for (i = 0 ; i < CE_MAX ; i++)
		{
		    strpcopy (s2, pc1) ;
		    if (strcmp(s2, s1) == 0)
		    {
			set_bit (&mtemp, i) ; /* change this bit */
			if (enable)
			    set_bit (&ltemp, i) ;
			else
			    clr_bit (&ltemp, i) ;
			found = TRUE ;
			break ;
		    }
		    else
			pc1 = (pchar) ((uintptr_t) pc1 + *pc1 + 1) ;
		}
		if (! found)
		    printf ("Name not found\n") ;
	    }
	    pcec = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    pcec->remote_map = ltemp ;
	    pcec->remote_mask = mtemp ;
	    this->command = CSCM_COMM_EVENT ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("Comm Event Command sent\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 25 :
	{
	    pdc = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    printf ("File name on DA : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if (s1[0] == '\0')
		break ;
	    s1[59] = '\0' ; /* just in case */
	    strpas(pdc->dasource, s1) ; /* Must be Pascal string */
	    pdc->dpmodname[0] = '\0' ;    /* Not used on Unix */
	    this->command = CSCM_DOWNLOAD ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pdr = (pvoid) &pcomm->moreinfo ;
		pc1 = NULL ;
		while (1)
		{
		    err = wait_finished (pcomm) ;
		    if (err != CSCS_INPROGRESS)
			break ;
		    if (pdr->fsize == 0)
			printf ("No data transferred yet, continue to wait (Y/N) : ") ;
		    else
			printf ("%d%% percent transferred, continue to wait (Y/N) : ",
				(int32_t) ((pdr->byte_count * 100.0) / (unsigned int) pdr->fsize)) ;
		    gets_buf(s1, BUF_SIZE) ;
		    if ((s1[0] != 'Y') && (s1[0] != 'y'))
		    {
			this->command = CSCM_DOWNLOAD_ABORT ;
			err = cs_svc(me, 0) ;
		    }
		}
		if (err == CSCS_FINISHED)
		{
		    printf ("Is this a text file (Y/N) : ") ;
		    gets_buf (s1, BUF_SIZE) ;
		    pcomm->completion_status = CSCS_IDLE ;
		    pc1 = (pvoid) shmat(pdr->dpshmid, NULL, 0) ; /* attach to data module */
		    pc2 = pc1 ;
		    if ((uintptr_t) pc1 == ERROR)
		    {
			printf ("Could not attach to shared memory segment\n") ;
			break ;
		    }
		    if ((s1[0] == 'Y') || (s1[0] == 'y'))
		    { /* On Unix convert CR to LF */
			cnt = pdr->fsize ;
			while (cnt-- > 0)
			    if (*pc2 == 0xd)
				*pc2++ = 0xa ;
			    else
				pc2++ ;
		    }
		    while (1)
		    {
			printf ("File name on host (CR to abort) : ") ;
			gets_buf(s1, BUF_SIZE) ;
			if (s1[0] == '\0')
			    break ;
			path = fopen (s1, "w") ;
			if (path == NULL)
			    printf ("Could not open %s\n", s1) ;
			else
			{
			    fwrite (pc1, 1, pdr->fsize, path) ;
			    fclose (path) ;
			    printf ("File transferred\n") ;
			    break ;
			}
		    } 
		}
		else
		    showerr (err) ;
		shmdt(pc1) ;
		if (pdr->dpshmid != NOCLIENT)  /* If exists, delete it */
		    shmctl(pdr->dpshmid, IPC_RMID, NULL) ;
		pcomm->completion_status = CSCS_IDLE ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 26 :
	{
	    puc = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    printf ("File name on DA : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if (s1[0] == '\0')
		break ;
	    s1[59] = '\0' ; /* just in case */
	    strpas(puc->dadest, s1) ; /* Must be Pascal string */
	    puc->dpmodname[0] = '\0' ;    /* Not used on Unix */
	    found = FALSE ;
	    while (1)
	    {
		printf ("File name on host (CR to abort) : ") ;
		gets_buf(s1, BUF_SIZE) ;
		if (s1[0] == '\0')
		    break ;
		path = fopen (s1, "r") ;
		if (path == NULL)
		    printf ("Could not open %s\n", s1) ;
		else
		{
#ifdef SOLARIS2
		    if (stat (s1, &statbuf) == ERROR)
#else
			if (stat ((const char *)&s1, &statbuf) == ERROR)
#endif
			{
			    printf ("Could not get file size\n") ;
			    break ;
			}
		    puc->fsize = statbuf.st_size ;
		    upshmid = shmget(IPC_PRIVATE, puc->fsize, IPC_CREAT | PERM) ;
		    if (upshmid == ERROR)
		    {
			showerr (CSCR_PRIVATE) ;
			break ;
		    }
		    pupbuf = (pvoid) shmat (upshmid, NULL, 0) ;
		    if ((uintptr_t) pupbuf == ERROR)
		    {
			showerr (CSCR_PRIVATE) ;
			break ;
		    }
		    fread (pupbuf, 1, puc->fsize, path) ;
		    fclose (path) ;
		    found = TRUE ;
		    break ;
		}
	    } 
	    if (! found)
		break ;
	    printf ("Is this a text file (Y/N) : ") ;
	    gets_buf (s1, BUF_SIZE) ;
	    if ((s1[0] == 'Y') || (s1[0] == 'y'))
	    { /* On Unix convert LF to CR */
		cnt = puc->fsize ;
		pc2 = (pvoid) pupbuf ;
		while (cnt-- > 0)
		    if (*pc2 == 0xa)
			*pc2++ = 0xd ;
		    else
			pc2++ ;
	    }
	    puc->dpshmid = upshmid ;
	    shmdt ((pchar) pupbuf) ; /* don't really need to access it anymore */
	    this->command = CSCM_UPLOAD ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pupres = (pvoid) &pcomm->moreinfo ;
		while (1)
		{
		    err = wait_finished (pcomm) ;
		    if (err != CSCS_INPROGRESS)
			break ;
		    if (pupres->bytecount == 0)
			printf ("No data transferred yet, continue to wait (Y/N) : ") ;
		    else if (pupres->retries > 0)
			printf ("%d Missed packets resent, continue to wait (Y/N) : ",
				pupres->retries) ;
		    else
			printf ("%d%% percent transferred, continue to wait (Y/N) : ",
				(int32_t) ((pupres->bytecount * 100.0) / (unsigned int) puc->fsize)) ;
		    gets_buf(s1, BUF_SIZE) ;
		    if ((s1[0] != 'Y') && (s1[0] != 'y'))
		    {
			this->command = CSCM_UPLOAD_ABORT ;
			err = cs_svc(me, 0) ;
		    }
		}
		if (err == CSCS_FINISHED)
		{
		    printf ("File Transferred\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    shmctl(upshmid, IPC_RMID, NULL) ;
	    break ;
	}
	case 27 :
	{
	    this->command = CSCM_LINKSTAT ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		pls = (pvoid) &pcomm->moreinfo ;
		plsc = (pvoid) ((uintptr_t) me + this->cominoffset) ;
		plsc->pollusecs = replace ("Server polling delay in us", pls->pollusecs) ;
		plsc->reconcnt = replace ("Number of sequence errors before reconfigure", pls->reconcnt) ;
		plsc->net_idle_to = replace ("Network packet reception timeout in seconds", pls->net_idle_to) ;
		plsc->net_conn_dly = replace ("Network connection polling interval", pls->net_conn_dly) ;
		plsc->grpsize = replace ("ACK packet grouping size", pls->grpsize) ;
		plsc->grptime = replace ("ACK packet grouping timeout", pls->grptime) ;
		pcomm->completion_status = CSCS_IDLE ;
		this->command = CSCM_LINKSET ;
		err = cs_svc(me, 0) ;
		if (err == CSCR_GOOD)
		{
		    printf ("Server link settings changed\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	case 28 :
	{
	    this->command = CSCM_FLOOD_CTRL ;
	    pflood = (pvoid) ((uintptr_t) me + this->cominoffset) ;
	    printf ("Enable Flooding (Y/N) : ") ;
	    gets_buf(s1, BUF_SIZE) ;
	    *pflood = ((s1[0] == 'Y') || (s1[0] == 'y')) ;
	    err = cs_svc(me, 0) ;
	    if (err == CSCR_GOOD)
	    {
		err = wait_finished (pcomm) ;
		if (err == CSCS_FINISHED)
		{
		    printf ("Flood Command Sent\n") ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
		else
		    showerr (err) ;
	    }
	    else
		showerr (err) ;
	    break ;
	}
	}
    }
    while (1) ;

    cs_off (me) ;
    return 0;
}
