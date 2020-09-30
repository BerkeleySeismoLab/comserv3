/*$Id: dataread.c,v 1.2 2005/06/16 20:52:10 isti Exp $*/
/*   Client Test Program. Gets data and blockettes from servers.
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 11 Mar 94 WHO First created.
    1 30 Mar 94 WHO Uses new service calls.
    2  6 Jun 94 WHO Two new status strings added.
    3  9 Jun 94 WHO Cleanup to avoid warnings.
    4 13 Dec 94 WHO Add verbose flag to show more contents of packets.
    5 27 Feb 95 WHO Start of conversion to run on OS9.
    6  3 Dec 96 WHO Add support for Blockette Records.
    7 12 Jun 97 WHO Show seed sequence number for each record.
    8  3 Feb 2012 DSN Only disallow verbose option on little endian systems.
    9 29 Sep 2020 DSN Updated for comserv3.
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
#include <unistd.h>

#include "cslimits.h"
#include "cstypes.h"
#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "seedutil.h"

tclientname name = "DATA" ;
tservername sname = "*" ;
tstations_struc stations ;
typedef char char23[24] ;
boolean verbose = FALSE ;

char23 stats[13] = { "Good", "Enqueue Timeout", "Service Timeout", "Init Error",
                       "Attach Refused", "No Data", "Server Busy", "Invalid Command",
                       "Server Dead", "Server Changed", "Segment Error",
                       "Command Size", "Privileged Command" } ;

extern pchar seednamestring (seed_name_type *sd, location_type *loc) ;

double seed_jul (seed_time_struc *st)
{
    double t ;
      
    t = jconv (st->yr - 1900, st->jday) ;
    t = t + (double) st->hr * 3600.0 + (double) st->minute * 60.0 + 
	(double) st->seconds + (double) st->tenth_millisec * 0.0001 ;
    return t ;
}
    
void depad (pchar ain, pchar sout, short cnt)
{
    short n ;
      
    for (n = 0 ; n < cnt ; n++)
        sout[n] = ain[n] ;
    sout[cnt] = '\0' ;
    for (n = cnt - 1 ; n > 0 ; n--)
        if (sout[n] != ' ')
	{
	    sout[n + 1] = '\0' ;
	    break ;
	}
}

int main (int argc, char *argv[])
{
    typedef seed_fixed_data_record_header *tpdh ;
    typedef data_only_blockette *tpdob ;
    typedef data_extension_blockette *tpdeb ;
    typedef timing *tptim ;
    typedef murdock_detect *tpmd ;
    typedef threshold_detect *tptd ;
    typedef cal2 *tpcal2 ;
    typedef step_calibration *tpstep ;
    typedef sine_calibration *tpsine ;
    typedef random_calibration *tprand ;
    typedef opaque_hdr *tophdr ;

    pclient_struc me ;
    pclient_station this ;
    short j, k ;
    boolean alert, mh ;
    pdata_user pdat ;
    seed_record_header *pseed ;
    tophdr popaque ;
    char s[80], s2[130] ;
    char af[10], ifl[10], df[10] ;
    float a ;
    double b ;
    tpdh pdh ;
    tpdob pdob ;
    tpdeb pdeb ;
    tptim ptim ;
    tpmd pmd ;
    tptd ptd ;
    tpstep pstep ;
    tpsine psine ;
    tprand prand ;
    tpcal2 pcal2 ;
    pchar pc1, pc2 ;

/* Allow override of station name on command line and -V option */
    for (j = 1 ; j < argc ; j++)
    {
	strcpy (s, argv[j]) ;
	upshift(s) ;
	if (strcmp(s, "-V") == 0)
	    verbose = TRUE ;
	else
	{
	    strncpy(sname, s, SERVER_NAME_SIZE) ;
	    sname[SERVER_NAME_SIZE-1] = '\0' ;
	}
    }

/* For non-OS9 disallow verbose option on little endian systems. */
#ifdef	ENDIAN_LITTLE
    verbose = FALSE;
#endif

/* Generate an entry for all available stations */      
    cs_setup (&stations, name, sname, TRUE, TRUE, 10, 5, CSIM_MSG |  CSIM_DATA | CSIM_CAL |
              CSIM_EVENT | CSIM_TIMING | CSIM_BLK, 6000) ;

/* Create my segment and attach to all stations */      
    me = cs_gen (&stations) ;

/* Show beginning status of all stations */
    for (j = 0 ; j < me->maxstation ; j++)
    {
	this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
	printf ("[%s] Status=%s\n", sname_str_cs(this->name),
		(char *)stats[this->status]) ;
    }
      
    do
    {
	j = cs_scan (me, &alert) ;
	if (j != NOCLIENT)
	{
	    this = (pclient_station) ((uintptr_t) me + me->offsets[j]) ;
	    if (alert)
	    printf("New status on station %s is %s\n", sname_str_cs(this->name),
		   (char *)stats[this->status]) ;
	    if (this->valdbuf)
	    {
		pdat = (pdata_user) ((uintptr_t) me + this->dbufoffset) ;
		for (k = 0 ; k < this->valdbuf ; k++)
		{
		    pseed = (pvoid) &pdat->data_bytes ;
		    printf("[%s] <%2d> Station=%s Network=%s Channel=%s Seq=%6.6s Received at=%s\n",
			   sname_str_cs(this->name), k, 
			   (char *)seedstationidstring(pseed->station_ID_call_letters), 
			   (char *)seednetstring(pseed->seednet),
			   (char *)seednamestring(&pseed->channel_id, &pseed->location_id),
			   (char *)pseed->sequence,
			   (char *)time_string(pdat->reception_time)) ;
		    if (verbose)
		    {
			b = seed_jul(&pseed->starting_time) ;
			pdh = (tpdh) pseed ;
			pdob = &pdh->dob ;
			if ((pseed->samples_in_record) && (pseed->sample_rate_factor))
			{ /* data record */
			    pdeb = &pdh->deb ;
			    b = b + (double) pdeb->usec99 / 1.0E6 ;
			}
			else if ((pdob->blockette_type == 1000) && (pdob->next_offset))
			{ /* might be timing blockette */
			    ptim = (tptim) ((uintptr_t) pdh + pdob->next_offset) ;
			    if (ptim->blockette_type == 500)
				b = b + (double) ptim->usec99 / 1.0E6 ; /* it is */
			}
			printf ("     Time=%s   Blks=%d   Samples=%d\n",
				time_string(b), 
				pseed->number_of_following_blockettes,
				pseed->samples_in_record) ;
			if (pseed->activity_flags == SEED_ACTIVITY_FLAG_END_EVENT)
			{ /* end of detection */
			    printf ("     End of Detection\n") ;
			}
			else if (pseed->samples_in_record)
			    if (pseed->sample_rate_factor)
			    { /* data */
				a = pseed->sample_rate_factor ;
				if (a < 0.0)
				    a = 1.0 / (-a) ;
				strcpy (af, "--------") ;
				strcpy (ifl, "--------") ;
				strcpy (df, "--------") ;
				if (pseed->activity_flags & SEED_ACTIVITY_FLAG_CAL_IN_PROGRESS)
				    af[7] = 'C' ;
				if (pseed->activity_flags & SEED_ACTIVITY_FLAG_BEGIN_EVENT)
				    af[5] = 'B' ;
				if (pseed->activity_flags & SEED_ACTIVITY_FLAG_EVENT_IN_PROGRESS)
				    af[1] = 'P' ;
				if (pseed->IO_flags & SEED_IO_CLOCK_LOCKED)
				    ifl[2] = 'L' ;
				if (pseed->data_quality_flags & SEED_QUALITY_FLAG_MISSING_DATA)
				    df[3] = 'M' ;
				if (pseed->data_quality_flags & SEED_QUALITY_FLAG_QUESTIONABLE_TIMETAG)
				    df[0] = 'Q' ;
				printf ("     Activity Flags=%s   IO Flags=%s   Data Quality Flags=%s\n",
					af, ifl, df) ;
				switch (pdob->encoding_format)
				{
				case 10 : 
				{
				    strcpy (s2, "Steim1") ;
				    break ;
				}
				case 11 : 
				{
				    strcpy (s2, "Steim2") ;
				    break ;
				}
				case 19 : 
				{
				    strcpy (s2, "Steim3") ;
				    break ;
				}
				default : strcpy (s2, "Unknown") ;
				}
				printf ("     Encoding Format=%s   Frequency=%5.2fHz  Clock Quality=%d%%\n",
					s2, a, (short) pdeb->qual) ;
			    }
			    else
			    { /* messages */
				pc1 = (pchar) ((uintptr_t) pseed + pseed->first_data_byte) ;
				pc2 = (pchar) ((uintptr_t) pc1 + pseed->samples_in_record - 2) ;
				*pc2 = '\0' ;
				printf ("     %s\n", pc1) ;
			    }
			else if (pdob->blockette_type == 1000)
			{ /* must be timing, detection, or calibration */
			    ptim = (tptim) ((uintptr_t) pdh + pdob->next_offset) ;
			    switch (ptim->blockette_type)
			    {
			    case 500 :
			    {
				depad (ptim->exception_type, s, 16) ;
				printf ("     %s Timemark   VCO Correction=%5.2f%%   Reception=%d%%   Count=%d\n",
					s, ptim->vco_correction, 
					ptim->reception_quality, ptim->exception_count) ;
				depad (ptim->clock_model, s, 32) ;
				depad (ptim->clock_status, s2, 128) ;
				printf ("     Model=%s\n", s) ;
				printf ("     Status:%s\n", s2) ;
				break ;
			    }
			    case 200 : ;
			    case 201 :
			    {
				mh = (ptim->blockette_type == 201) ;
				pmd = (tpmd) ptim ;
				ptd = (tptd) ptim ;
				if (mh)
				    printf ("     Murdock-Hutt") ;
				else
				    printf ("     Threshold") ;
				printf (" Detection  Counts=%3.0f  Period=%5.3f  Background=%3.0f\n",
					pmd->signal_amplitude, pmd->signal_period, pmd->background_estimate) ;
				if (mh)
				    depad (pmd->s_detname, s, 24) ;
				else
				    depad (ptd->s_detname, s, 24) ;
				printf ("     Flags=%d  Detector Name=%s", pmd->event_detection_flags, s) ;
				if (mh)
				    printf ("  SNR=%d%d%d%d%d  Lookback=%d  Pick=%d\n",
					    (short) pmd->snr[0], (short) pmd->snr[1],
					    (short) pmd->snr[2], (short) pmd->snr[3],
					    (short) pmd->snr[4], (short) pmd->lookback_value,
					    (short) pmd->pick_algorithm) ;
				else
				    printf ("\n") ;
				break ;
			    }
			    case 300 : ;
			    case 310 : ;
			    case 320 : ;
			    case 395 :
			    {
				pstep = (tpstep) ptim ;
				psine = (tpsine) ptim ;
				prand = (tprand) ptim ;
				switch (ptim->blockette_type)
				{
				case 300 :
				{
				    pcal2 = (tpcal2) &pstep->step2 ;
				    strcpy (s, "Step") ;
				    break ;
				}
				case 310 :
				{
				    pcal2 = (tpcal2) &psine->sine2 ;
				    strcpy (s, "Sine") ;
				    break ;
				}
				case 320 :
				{
				    pcal2 = (tpcal2) &prand->random2 ;
				    depad (prand->noise_type, s, 8) ;
				    strcat (s, " Noise") ;
				    break ;
				}
				default :
				{
				    pcal2 = NULL ;
				    strcpy (s, "Abort") ;
				    break ;
				}
				}
				printf ("     %s Calibration", s) ;
				if (pcal2)
				{
				    strcpy (s2, "-----M--") ;
				    if (pstep->calibration_flags & 0x40)
					s2[1] = 'R' ;
				    if (pstep->calibration_flags & 0x20)
					s2[2] = 'Z' ;
				    if (pstep->calibration_flags & 0x10)
					s2[3] = 'P' ;
				    if (pstep->calibration_flags & 0x04)
					s2[5] = 'A' ;
				    if (pstep->calibration_flags & 0x01)
					s2[7] = '+' ;
				    printf ("   Duration=%6.4f    Flags=%s\n",
					    pstep->calibration_duration * 0.0001, s2) ;
				    if (ptim->blockette_type == 310)
					printf ("     Sine Period=%6.4f  ", psine->sine_period) ;
				    else
					printf ("     ") ;
				    printf ("Amplitude=%5.3f   Ref. Amp.=%5.3f\n", 
					    pcal2->calibration_amplitude, pcal2->ref_amp) ;
				    depad (pcal2->calibration_input_channel, s, 3) ;
				    if (s[0] > ' ')
					printf ("     Monitor Ch=%s  ", s) ;
				    else
					printf ("     ") ;
				    depad (pcal2->coupling, s, 12) ;
				    depad (pcal2->rolloff, s2, 12) ;
				    printf ("Coupling=%s  Rolloff=%s\n", s, s2) ;
				}
				else
				    printf ("\n") ;
			    }
			    case 2000 :
			    { /* Must be Blockette */
				popaque = (tophdr) ptim ;
				do
				{
				    printf ("        Blockette Length=%d    Record Number %d\n", 
					    popaque->blockette_lth, popaque->record_num) ;
				    if (popaque->next_blockette)
					popaque = (tophdr)((uintptr_t) pseed + popaque->next_blockette) ;
				    else
					popaque = NULL ;
				}
				while (popaque) ;
			    }
			    } 
			}
		    }
		    pdat = (pdata_user) ((uintptr_t) pdat + this->dbufsize) ;
		}
	    }
	}
	else
	sleep (1) ; /* Bother the server once every second */
    }
    while (1) ;
}
