/*$Id: config.c,v 1.2 2005/06/16 20:52:10 isti Exp $*/
/*   Client Test Program 2. Does local comserv commands.
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0  7 Apr 94 WHO Derived from test.c
    1  6 Jun 94 WHO One new command status string added.
    2  9 Jun 94 WHO Cleanup to avoid warnings.
    3 17 Jun 95 WHO Display new entries in linkrecord.
    4 20 Jun 95 WHO Display new entries in linkstat.
    5 29 May 96 WHO Start of conversion to run on OS9.
    6 29 Sep 2020 DSN Updated for comserv3.
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
#include <sys/time.h>
#include <signal.h>

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"

/* #define RESTRICT uncomment to restrict channel request */

tclientname name = "CONF" ;
tservername sname = "*" ;
tstations_struc stations ;

typedef char char23[24] ;
typedef char char15[16] ;
typedef char char7[8] ;

char23 stats[13] = { "Good", "Enqueue Timeout", "Service Timeout", "Init Error",
		     "Attach Refused", "No Data", "Server Busy", "Invalid Command",
		     "Server Dead", "Server Changed", "Segment Error",
		     "Command Size", "Privileged Command" } ;

char15 waves[4] = { "Sine", "Step", "Red Noise", "White Noise" } ;

char7 freqs[Hz0_0005+1] = { "DC", "25.000", "10.000", "8.0000", "6.6667", "5.0000", "4.0000", "3.3333",
			    "2.5000", "2.0000", "1.6667", "1.2500", "1.0000", "0.8000", "0.6667", "0.5000", "0.2000",
			    "0.1000", "0.0667", "0.0500", "0.0400", "0.0333", "0.0250", "0.0200", "0.0167", "0.0125",
			    "0.0100", "0.0050", "0.0020", "0.0010", "0.0005" } ;

char15 outpath[6] = { "Cont-Comlink", "Event-Comlink", "Cont-Tape", "Event-Tape", "Cont-Disk", "Event-Disk" } ;

char15 formats[2] = { "QSL", "Q512" } ;

pchar seednamestring (seed_name_type *sd, location_type *loc) ;

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
 
int main (int argc, char *argv[], char **envp)
{
    pclient_struc me ;
    pclient_station this ;
    short i, j, k ;
    int32_t done, good, goodr, ultra, link ;
    typedef void *pvoid ;
    comstat_rec *pcomm ;
    link_record *plink ;
    linkstat_rec *pls ;
    digi_record *pdigi ;
    ultra_rec *pur ;
    pchar pc1 ;
    cal_record *pcal ;
    eachcal *pe ;
    chan_record *pcr ;
    chan_struc *pcs ;
    char s1[200], s2[200], s3[200]  ;

/* Allow override of station name on command line */
    if (argc >= 2)
    {
	strncpy(sname, argv[1], SERVER_NAME_SIZE) ;
	sname[SERVER_NAME_SIZE-1] = '\0' ;
    }
    upshift(sname) ;

/* Generate an entry for all available stations */      
    cs_setup (&stations, name, sname, TRUE, FALSE, 0, 2, 0, 6000) ;

/* Create my segment and attach to all stations */      
    me = cs_gen (&stations) ;

#ifdef RESTRICT
/* For selector test, only accept data from ??BH? */
    this = (pclient_station) ((uintptr_t) me + me->offsets[0]) ;
    psa = (pselarray) ((uintptr_t) me + this->seloffset) ;
    strcpy (&((*psa)[1]), "??BH?") ;
    this->sels[CHAN].first = 1 ;
    this->sels[CHAN].last = 1 ;
#endif

    good = 0 ;
    ultra = 0 ;
    link = 0 ;
/* Show beginning status of all stations */
    for (j = 0 ; j < me->maxstation ; j++)
    {
	this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
	printf ("[%s] Status=%s\n", sname_str_cs(this->name),
		(char *)stats[this->status]) ;
	if (this->status == CSCR_GOOD)
	    set_bit (&good, j) ;
    }
      
/* For all stations, with good status, get link status packet */
    done = good ;
    goodr = 0 ;
    while (done != 0)
    {
	for (j = 0 ; j < me->maxstation ; j++)
            if (test_bit(done, j))
	    {
		this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
		this->command = CSCM_LINKSTAT ;
		if (cs_svc(me, j) == CSCR_GOOD)
		{
		    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		    pls = (pvoid) &pcomm->moreinfo ;
		    printf ("CSIM_LINKSTAT for [%s]\n", sname_str_cs(this->name)) ;
		    printf ("  Ultra=%d, Link Recv=%d, Ultra Recv=%d, Suspended=%d, Data Format=%s\n",
			    pls->ultraon, pls->linkrecv, pls->ultrarecv, pls->suspended,
			    (char *)formats[pls->data_format]) ;
		    printf ("  Total Packets=%d, Sync Packets=%d, Sequence Errors=%d\n",
			    pls->total_packets, pls->sync_packets, pls->seq_errors) ;
		    printf ("  Checksum Errors=%d, IO Errors=%d, Last IO Error=%d\n",
			    pls->check_errors, pls->io_errors, pls->lastio_error) ;
		    printf ("  Blocked Packets=%d, Seed Format=%5.5s, Seconds in Operation=%d\n",
			    pls->blocked_packets, pls->seedformat, pls->seconds_inop) ;
		    strpcopy (s1, pls->description) ;
		    printf ("  Station Description=%s\n", (char *)s1) ;
		    printf ("  Polling=%dus, Reconfig=%d, Net Timeout=%d, Net Polling=%d\n",
			    pls->pollusecs, pls->reconcnt, pls->net_idle_to, pls->net_conn_dly) ;
		    printf ("  Group Size=%d, Group Timeout=%d\n", pls->grpsize, pls->grptime) ;
		    clr_bit (&done, j) ;
		    if (pls->ultraon)
			set_bit (&ultra, j) ;
		    if (pls->linkrecv)
			set_bit (&link, j) ;
		    if (pls->ultrarecv)
			set_bit (&goodr, j) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
    }

/* For all stations, with link status, get link packet */
    done = link ;
    while (done != 0)
    {
	for (j = 0 ; j < me->maxstation ; j++)
            if (test_bit(done, j))
	    {
		this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
		this->command = CSCM_LINK ;
		if (cs_svc(me, j) == CSCR_GOOD)
		{
		    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		    plink = (pvoid) &pcomm->moreinfo ;
		    printf ("CSIM_LINK for [%s]\n", sname_str_cs(this->name)) ;
		    printf ("  Window Size=%d, Total priority levels=%d, Message Priority=%d\n",
			    plink->window_size, plink->total_prio, plink->msg_prio) ;
		    printf ("  Detection Priority=%d, Timing Priority=%d, Cal Priority=%d\n",
			    plink->det_prio, plink->time_prio, plink->cal_prio) ;
		    printf ("  Resend Time=%d  Sync Time=%d  Resend Pkts=%d\n",
			    plink->resendtime, plink->synctime, plink->resendpkts) ;
		    printf ("  Net Restart Dly=%d  Net Conn. Time=%d  Net Packet Limit=%d\n",
			    plink->netdelay, plink->nettime, plink->netmax) ;
		    printf ("  Group Pkt Size=%d  Group Timeout=%d\n",
			    plink->groupsize, plink->grouptime) ;
		    clr_bit (&done, j) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
    }

/* For all stations, with ultra received status, get digitizer packet */
    done = goodr & ultra ;
    while (done != 0)
    {
	for (j = 0 ; j < me->maxstation ; j++)
            if (test_bit(done, j))
	    {
		this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
		this->command = CSCM_DIGI ;
		if (cs_svc(me, j) == CSCR_GOOD)
		{
		    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		    pdigi = (pvoid) &pcomm->moreinfo ;
		    printf ("CSIM_DIGI for [%s]\n", sname_str_cs(this->name)) ;
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
		    clr_bit (&done, j) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
    }

/* For all stations, with goodr status, get ultra packet */
    done = goodr & ultra ;
    while (done != 0)
    {
	for (j = 0 ; j < me->maxstation ; j++)
            if (test_bit(done, j))
	    {
		this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
		this->command = CSCM_ULTRA ;
		if (cs_svc(me, j) == CSCR_GOOD)
		{
		    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		    pur = (pvoid) &pcomm->moreinfo ;
		    printf ("CSIM_ULTRA for [%s]\n", sname_str_cs(this->name)) ;
		    printf ("  VCO Value=%d, PLL ON=%d, Mass Rec. OK=%d\n",
			    pur->vcovalue, pur->pllon, pur->umass_ok) ;
		    s1[0] = '\0' ;
		    pc1 = (pchar) &pur->commnames ;
		    for (i = 0 ; i < CE_MAX ; i++)
		    {
			strpcopy (s2, pc1) ;
			pc1 = (pchar) ((uintptr_t) pc1 + *pc1 + 1) ;
			strcat (s1, s2) ;
			strcat (s1, " ") ;
			if (strlen(s1) > 70)
			{
			    printf ("  %s\n", s1) ;
			    s1[0] = '\0' ;
			}
		    }
		    if (s1[0] != '\0')
			printf ("  %s\n", s1) ;
		    clr_bit (&done, j) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
    }

/* For all stations, with goodr status, get cal packet */
    done = goodr ;
    while (done != 0)
    {
	for (j = 0 ; j < me->maxstation ; j++)
            if (test_bit(done, j))
	    {
		this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
		this->command = CSCM_CAL ;
		if (cs_svc(me, j) == CSCR_GOOD)
		{
		    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		    pcal = (pvoid) &pcomm->moreinfo ;
		    printf ("CSIM_CAL for [%s]\n", sname_str_cs(this->name)) ;
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
		    clr_bit (&done, j) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
    }

/* For all stations, with good status, get chan packet */
    done = goodr ;
    while (done != 0)
    {
	for (j = 0 ; j < me->maxstation ; j++)
            if (test_bit(done, j))
	    {
		this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
		this->command = CSCM_CHAN ;
		if (cs_svc(me, j) == CSCR_GOOD)
		{
		    pcomm = (pvoid) ((uintptr_t) me + this->comoutoffset) ;
		    pcs = (pvoid) &pcomm->moreinfo ;
		    printf ("CSIM_CHAN for [%s]\n", sname_str_cs(this->name)) ;
		    for (i = 0 ; i < pcs->chancount ; i++)
		    {
			pcr = &pcs->chans[i] ;
			printf ("    %s Stream=%d, Physical=%d, Detectors=%d, CPrio=%d, Eprio=%d",
				seednamestring (&pcr->seedname, &pcr->seedloc), pcr->stream, 
				pcr->physical, pcr->det_count, pcr->c_prio, pcr->e_prio) ;
			printf (" Rate=") ;
			if (pcr->rate > 0)
			    printf ("%4.3f\n", 1.0 * pcr->rate) ;
			else
			    printf ("%4.3f\n", 1.0 / (0 - pcr->rate)) ;
			printf ("      Available Outputs=%s\n", showout(pcr->available)) ;
			printf ("      Enabled   Outputs=%s\n", showout(pcr->enabled)) ;
		    }
		    clr_bit (&done, j) ;
		    pcomm->completion_status = CSCS_IDLE ;
		}
	    }
    }

    cs_off (me) ;
    return 0;
}
