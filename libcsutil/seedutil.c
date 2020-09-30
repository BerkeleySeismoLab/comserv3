/*$Id: seedutil.c,v 1.2 2005/03/30 22:27:56 isti Exp $*/
/*   Quanterra to Seed format utility module.
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 27 Mar 94 WHO Translated from seedutil.p
    1  6 Jun 94 WHO In seedheader, make sequence number out of packet_seq (DSN).
                    Since longinteger station name is right justified,
                    use routine to make left justified Seed station name.
    2  9 Jun 94 WHO Cleanup to avoid warnings.
    3 25 Sep 94 WHO Add conversion of RECORD_HEADER_3 to compression 19.
    4 12 Dec 94 WHO Add support for SEED version 2.3B.
    5 28 Feb 95 WHO Start of conversion to run on OS9.
    6 26 Jun 95 DSN Added external symbols to allow change to LOG and ACE
                    SEED channel names and locations.
    7 13 Jun 96 WHO Add copying of seedname and location in comment and
                    clock correction packet if new "SEEDIN" flag is set.
    8 11 Jun 97 WHO In seedblocks set seed sequence number based on
                    value in header_flag.
    9 17 Oct 97 WHO Add VER_SEEDUTIL.
   10 26 Nov 97 WHO Set deb_flags based on header_revision.
   11  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
  17 Nov 99 IGD Used the old porting code to idetify byte-swapping and applied
                byte-swapping routines flip2() and flip4() to the following lines
                175: seed_time (&h->time_of_sample, ((int32_t) h->millisec) * 1000L + ((int32_t) flip2(h->microsec)),
                176: &sh->header.starting_time) ;
                231: return julian(&h->time_of_sample) + h->millisec / 1000.0 + flip2(h->microsec) / 1000000.0 ;
                382: tim->vco_correction = flip2(pce->vco) / 40.96 ;
                403: sprintf (s2, "%d", flip4(pce->clock_drift) ) ;
                445: usec = ((int32_t) (flip4 (ptqd->msec_correction))) * 1000 + ((int32_t) (flip2(pce->usec_correction)) );
                447: tim->usec99 = flip2 (pce->usec_correction) % 100 ;
                448: headtime = julian(&ta) + (int32_t) (flip4(ptqd->msec_correction)) / 1000.0 + flip2(pce->usec_correction) / 1000000.0 ;
                465: det->signal_amplitude = (float) flip4(psedr->peak_amplitude) ;
                468: det->signal_period = ((float) flip4 (psedr->period_x_100) ) * 0.01 ;
                469: det->background_estimate = (float) flip4 (psedr->background_amplitude) ;
                476: gregorian((int32_t) flip4(psedr->jdate), &ta) ;
                477: usec = ((int32_t) flip2(psedr->millisec)) * 1000 ;
                493: headtime = (double) flip4 ( psedr->jdate) + SECCOR + flip2(psedr->millisec) / 1000.0 ;
                512: calstep->calibration_duration = (int32_t)flip4 (ltemp.l) * 10000 ; -------------> not in old porting
                529: calsine->sine_period = (int32_t) flip4 (ltemp.l) / 1000.0 ;
  29 Noc 99 IGD Set wordorder to 0 (little-endian) if LINUX flag is set in set_dob
   6 Dec 99 IGD sh->deb.usec99 = flip2(h ->microsec) % 100 ;
		IGD flip2 microsecs before sending them to the sheared memory
  21 Mar 00 IGD Put the word_order flag to 1 SUN even for Linux version
		We do not swap data in comlink anymore -> Steim1 problem
  17 Mar 00 IGD Few byte-swapping operations are moved here from comserv/comlink.c
  29 Sep 2020 DSN Updated for comserv3.
*/   

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "quanstrc.h"
#include "stuff.h"

short VER_SEEDUTIL = 11 ;

/************************************************************************
 * External symbols for SEED channel_id and location_id names for the
 * channels derived from COMMENT and CLOCK_EXCEPTION packets.
 * These can be customized by comserv on a per-station basis by
 * changing the contents of these variables.
 ************************************************************************/
char log_channel_id[4] = "LOG" ;
char log_location_id[3] = "  " ;
char clock_channel_id[4] = "ACE" ;
char clock_location_id[3] = "  " ;
boolean seedin = FALSE ;

typedef data_only_blockette *tpdob ;
  
void seedsequence (seed_record_header *seed, int32_t seq) ;
short julian_day (time_array *gt) ;
short decode_rate (byte rate) ;
int32_t julian (time_array *gt) ;
void gregorian (int32_t jdate, time_array *ret) ;

void seed_time (time_array *t, int32_t usec, seed_time_struc *st)
{
    st->yr = ord(t->year) + 1900 ;
    st->jday = julian_day(t) ;
    st->hr = t->hour ;
    st->minute = t->min ;
    st->seconds = t->sec ;
    st->unused = 0 ;
    st->tenth_millisec = usec / 100 ;
}

/* Converts bits from the quanterra SOH byte to SEED activitiy flags */
byte translate_to_seed_activity_flags (byte soh)
{
    byte act ;

    act = 0 ;
    if (soh & SOH_BEGINNING_OF_EVENT)
	act = act | SEED_ACTIVITY_FLAG_BEGIN_EVENT ;
    if (soh & SOH_CAL_IN_PROGRESS)
	act = act | SEED_ACTIVITY_FLAG_CAL_IN_PROGRESS ;
    if (soh & SOH_EVENT_IN_PROGRESS)
	act = act | SEED_ACTIVITY_FLAG_EVENT_IN_PROGRESS ;
    return act ;
}

/* Converts bits from the quanterra SOH byte to SEED quality flags */
byte translate_to_seed_quality_flags (byte soh)
{
    byte qual ;

    qual = 0 ;
    if (soh & SOH_GAP)
	qual = qual | SEED_QUALITY_FLAG_MISSING_DATA ;
    if (((soh & SOH_INACCURATE) != 0) ||
	(((soh ^ SOH_EXTRANEOUS_TIMEMARKS) &
	  (SOH_EXTRANEOUS_TIMEMARKS | SOH_EXTERNAL_TIMEMARK_TAG)) == 0))
	qual = qual | SEED_QUALITY_FLAG_QUESTIONABLE_TIMETAG ;
    return qual ;
}

/* Get percentage of clock quality */
short clk_perc (signed char b)
{
    short perc[7] = {0, 40, 10, 20, 60, 80, 100} ;
      
    return perc[b + 1] ;
}

/* Setup common area of data only blockette */
void set_dob (tpdob pdob)
{
    pdob->blockette_type = 1000 ;
# if defined (LINUX) /*IGD Make sure that we mention to the client that this is little-endian*/
    pdob->word_order = 1 ;  /*IGD March 2000 - keep data in SUNs order */
#else
    pdob->word_order = 1 ;
#endif
    pdob->rec_length = 9 ;
    pdob->dob_reserved = 0 ;
}

/* Take a C string, move it into a fixed length array, and pad
   on the right with spaces to fill the array */
void cpadright (pchar s, pchar b, short fld)
{
    short i, j ;
      
    j = strlen(s) ;
    if (j > fld)
	j = fld ;
    for (i = 0 ; i < j ; i++)
        b[i] = s[i] ;
    for (i = j ; i < fld ; i++)
        b[i] = ' ' ;
}

/* Take a Pascal string, move it into a fixed length array, and pad
   on the right with spaces to fill the array */
void padright (pchar s, pchar b, short fld)
{
    short i, j ;
      
    j = s[0] ;
    if (j > fld)
	j = fld ;
    for (i = 0 ; i < j ; i++)
        b[i] = s[i + 1] ;
    for (i = j ; i < fld ; i++)
        b[i] = ' ' ;
}

/* Passed the address of a quanterra header, and the record size, will convert the
   header to SEED format.
*/
double seedheader (header_type *h, seed_fixed_data_record_header *sh)
{
    short i ;
    pchar ps ;

    seedsequence (&sh->header, flip4(h->packet_seq) % 1000000) ;    /* IGD 03/03/01 flip4 */
    sh->header.seed_record_type = 'D' ;
    sh->header.continuation_record = ' ' ;
    /* station name converted from right justified 4 byte to left justified 5 byte */
    ps = long_str(h->station.l) ; /* get string version of station */
    memset(sh->header.station_ID_call_letters, ' ', 5) ; /* initialize to spaces */
    memcpy(sh->header.station_ID_call_letters, ps, strlen(ps)) ; /* copy valid part in */
    /* channel+id, location_id, and seednet are just copied directly */
    memcpy(sh->header.channel_id, h->seedname, 3) ;
    memcpy(sh->header.location_id, h->location, 2) ;
    memcpy(sh->header.seednet, h->seednet, 2) ;
    /* convert to seed time */
    seed_time (&h->time_of_sample, ((int32_t) flip2(h->millisec)) * 1000L + ((int32_t) flip2(h->microsec)),
	       &sh->header.starting_time) ;   		/*IGD flip2 and flip4 here */
    sh->header.samples_in_record = flip2(h->number_of_samples) ;    /* IGD 03/03/01 flip4 */
    sh->header.sample_rate_factor = decode_rate(h->rate) ;
    sh->header.sample_rate_multiplier = 1 ;
    sh->header.activity_flags = translate_to_seed_activity_flags (h->soh) ;
    sh->header.IO_flags = 0 ;
    sh->header.data_quality_flags = translate_to_seed_quality_flags (h->soh) ;
    sh->header.number_of_following_blockettes = h->blockette_count + 2 ;
    sh->header.tenth_msec_correction = 0 ; /* this is a delta, which we don't do */
    sh->header.first_data_byte = (ord(h->blockette_count) + 1) * 64 ;
    sh->header.first_blockette_byte = 48 ;
    sh->deb.blockette_type = 1001 ;
    if (h->blockette_count != 0)
	sh->deb.next_offset = 64 ;
    else
	sh->deb.next_offset = 0 ;
    i = clk_perc (h->clkq) ;
    if (h->soh & SOH_INACCURATE)
	i = 15 + (short) h->clkq ;
    else
    {
	if (h->soh & SOH_EXTRANEOUS_TIMEMARKS)
	    i = i - 30 ;
	if ((h ->soh & SOH_EXTERNAL_TIMEMARK_TAG) == 0)
	    i = i - 15 ;
    }
    if (i < 0)
	sh->deb.qual = 0 ;
    else
	sh->deb.qual = (byte) i ;
    if (h->clkq >= 3)
	sh->header.IO_flags = 0x20 ;
    sh->deb.usec99 = flip2(h ->microsec) % 100 ; /*IGD flip2 microsecs before sending them to the sheared memory */
    if (h->header_revision >= 5)
	sh->deb.deb_flags = DEB_NEWTIME ;
    else
	sh->deb.deb_flags = 0 ;
    sh->deb.frame_count = h->frame_count ;
    set_dob (&sh->dob) ;
    sh->dob.next_offset = 56 ;
    if (h->frame_type == RECORD_HEADER_1)
	sh->dob.encoding_format = 10 ;
    else if (h->frame_type == RECORD_HEADER_2)
	sh->dob.encoding_format = 11 ;
    else
	sh->dob.encoding_format = 19 ;

    return julian(&h->time_of_sample) + flip2(h->millisec) / 1000.0 + flip2(h->microsec) / 1000000.0 ;  /*IGD flip2 here */
}

/* Converts the sequence number into a string with leading zeroes */
void seedsequence (seed_record_header *seed, int32_t seq)
{
    char tmp[8] ;

    if ((seq < 0) | (seq > 999999))
	seq = 0 ;
    sprintf(tmp, "%-7d", seq + 1000000) ; /* convert to string */
    strncpy(seed->sequence, &tmp[1], 6) ; /* move 2nd through 7th characters */
}

/* given the seed channel name and location, returns a string in the format LL-CCC */
pchar seednamestring (seed_name_type *sd, location_type *loc)
{
    static char s2[7] ;
    short i, j ;

    i = 0 ;
    if ((*loc)[0] != ' ')
    {
	s2[i++] = (*loc)[0] ;
	if ((*loc)[1] != ' ')
	    s2[i++] = (*loc)[1] ;
	s2[i++] = '-' ;
    }
    for (j = 0 ; j < 3 ; j++)
        s2[i++] = (*sd)[j] ;
    s2[i] = '\0' ;
    return (pchar) &s2 ;
}

/* given the seed station_ID return a string in the format SSSSS */
pchar seedstationidstring (char  *in)
{
    static char s2[6] ;
    short i ;

    i = 0;
    while (i < 5 && in[i] != '\0' && in[i] != ' ') 
    {
	s2[i] = in[i] ;
	i++ ;
    }
    s2[i] = '\0' ;
    return (pchar) &s2 ;
}

/* given the seednet return a string in the format NN */
pchar seednetstring (char  *in)
{
    static char s2[3] ;
    short i ;

    i = 0;
    while (i < 2 && in[i] != '\0' && in[i] != ' ') 
    {
	s2[i] = in[i] ;
	i++ ;
    }
    s2[i] = '\0' ;
    return (pchar) &s2 ;
}

#define FIRSTDATA 56
#define FIRSTBLK 48

/* 
   sl is the buffer where the SEED header and blockette will go. log is the address
   of the commo_record that contains either a commo_event or commo_comment structure.
*/
double seedblocks (seed_record_header *sl, commo_record *log)
{
    typedef void *pvoid ;
    typedef char string9[10] ;
      
    pchar pc1, pc2 ;
    time_array ta ;
    short i, j, lth ;
    murdock_detect *det ;
    threshold_detect *dett ;
    timing *tim ;
    step_calibration *calstep ;
    sine_calibration *calsine ;
    random_calibration *calrand ;
    abort_calibration *calabort ;
    cal2 *calend ;
    time_quality_descriptor *ptqd ;
    squeezed_event_detection_report *psedr ;
    clock_exception *pce ;
    calibration_result *pcr ;
    boolean mh ;
    char mots[10] ;
    int32_t dbadj ;
    complong stat ;
    seed_net_type net ;
    complong ltemp ;
    double headtime ;
    int32_t usec ;
    pchar ps ;
    tpdob pdob ;
    char s[130] ;
    char s2[32] ;
    string9 filters[5] = {"None", "3dB@40Hz", "3dB@10Hz", "3dB@1Hz", "3dB@0.1Hz"} ;
    string15 mtypes[6] = {"Unknown", "Missing", "Expected",
			  "Valid", "Unexpected", "Daily"} ;
    string15 omeganames[8] = {"Norway", "Liberia", "Hawaii", "North Dakota",
			      "La Reunion", "Argentina", "Australia", "Japan"} ;
    string23 ctypes[11] = {"Unknown", "OS9 System", "Kinemetrics GOES",
			   "Kinemetrics Omega", "Kinemetrics DCF", "Meinburg DCF",
			   "Quanterra GPS1/QTS", "Quanterra GPS1/GOES", "Quanterra GPS1/UA31S",
			   "Quanterra GPS2/QTS2", "Altus K2 GPS"} ;
      
    sl->seed_record_type = 'D' ;
    sl->continuation_record = ' ' ;
    sl->location_id[0] = ' ' ;
    sl->location_id[1] = ' ' ;
    sl->first_blockette_byte = FIRSTBLK ;
    usec = 0 ;
    pce = &log->ce.header_elog.clk_exc ;
    pdob = (tpdob) ((uintptr_t) sl + FIRSTBLK) ;
    set_dob (pdob) ;
    pdob->encoding_format = 0 ;
    pdob->next_offset = 56 ; /* valid except for comments */
    sl->number_of_following_blockettes = 2 ; /* ditto */
    switch (pce->frame_type)
    {
    case COMMENTS :
    {
	if (seedin)
	    memcpy (sl->location_id, log->cc.cc_location, 5) ;
	else
	{
	    sl->channel_id[0] = log_channel_id[0] ;
	    sl->channel_id[1] = log_channel_id[1] ;
	    sl->channel_id[2] = log_channel_id[2] ;
	    sl->location_id[0] = log_location_id[0] ;
	    sl->location_id[1] = log_location_id[1] ;
	}
	pdob->next_offset = 0 ;
	sl->number_of_following_blockettes = 1 ;
	ta = log->cc.time_of_transmission ;
	memcpy(net, log->cc.cc_net, 2) ;
	memcpy((pvoid) &stat, (pvoid) &log->cc.cc_station, 4) ;
	sl->first_data_byte = FIRSTDATA ;
	pc1 = (pchar) ((uintptr_t) sl + FIRSTDATA) ;
	pc2 = log->cc.ct ;
	lth = ord(*pc2++) + 2 ; /* get dynamic length plus room for CR/LF */
	for (i = 3 ; i <= lth ; i++)
	    *pc1++ = *pc2++ ;
	*pc1++ = '\xd' ;
	*pc1++ = '\xa' ;
	sl->samples_in_record = lth ;
	headtime = julian(&ta) ;
	break ;
    }
    case CLOCK_CORRECTION :
    {
	if (seedin)
	    memcpy (sl->location_id, pce->cl_location, 5) ;
	else
	{
	    sl->channel_id[0] = clock_channel_id[0] ;
	    sl->channel_id[1] = clock_channel_id[1] ;
	    sl->channel_id[2] = clock_channel_id[2] ;
	    sl->location_id[0] = clock_location_id[0] ;
	    sl->location_id[1] = clock_location_id[1] ;
	}
	pce = &log->ce.header_elog.clk_exc ;
	ta = pce->time_of_mark ;
	memcpy(net, pce->cl_net, 2) ;
	memcpy((pvoid) &stat, (pvoid) &pce->cl_station, 4) ;
	tim = (pvoid) ((uintptr_t) sl + FIRSTDATA) ;
	tim->blockette_type = 500 ;
	tim->exception_count = flip4 (pce->count_of) ;  /* IGD 03/05/01 flip4 here*/
	tim->vco_correction = flip2(pce->vco) / 40.96 ;  /*IGD flip here */
	ptqd = (pvoid) &pce->correction_quality ;
	tim->reception_quality = (byte) clk_perc (ptqd->reception_quality_indicator) ;
	if (pce->clock_exception <= DAILY_TIME_CORRECTION_REPORT)
	    strcpy (s, mtypes[pce->clock_exception]) ;
	else
	    strcpy (s, "????") ;
	cpadright (s, tim->exception_type, 16) ;
	if (pce->cl_spec.model <= 10)
	    strcpy (s, ctypes[pce->cl_spec.model]) ;
	else
	{
	    i = (short) pce->cl_spec.model ;
	    sprintf (s2, "%d", i) ;
	    strcpy (s, "Model ") ;
	    strcat (s, s2) ;
	}
	cpadright (s, tim->clock_model, 32) ;
	strcpy (s, "Drift=") ;
	sprintf (s2, "%d", flip4(pce->clock_drift) ) ;  /*IGD flip4 here */
	strcat (s, s2) ;
	strcat (s, "usec") ;
	switch (pce->cl_spec.model)
	{
	case 3 :
	{
	    strcat (s, ", Station=") ;
	    if ((pce->cl_spec.cl_status[0] >= 'A') &&
		(pce->cl_spec.cl_status[0] <= 'H'))
		strcat (s, omeganames[pce->cl_spec.cl_status[0] - (short) 'A']) ;
	    else if ((pce->cl_spec.cl_status[0] >= 'I') &&
		     (pce->cl_spec.cl_status[0] <= 'Z'))
		strncat (s, (pchar) &pce->cl_spec.cl_status[0], 1) ;
	    else
	    {
		i = (short) pce->cl_spec.cl_status[0] ;
		sprintf (s2, "%d", i) ;
		strcat (s, s2) ;
	    }
	    break ;
	}
	case 6 : ;
	case 9 :
	{
	    strcat (s, ", Satellite SNR in dB=") ;
	    for (i = 0 ; i <= 5 ; i++)
	    {
		if (i)
		    strcat (s, ", ") ;
		j = (short) pce->cl_spec.cl_status[i] ;
		sprintf (s2, "%d", j) ;
		strcat (s, s2) ;
	    }
	    break ;
	}
	default : strcpy (s, "") ;
	}
	cpadright (s, tim->clock_status, 128) ;
	usec = ((int32_t) (flip4 (ptqd->msec_correction))) * 1000 + ((int32_t) (flip2(pce->usec_correction)) ); /*IGD flip2 here */
	seed_time (&pce->time_of_mark, usec, &tim->time_of_exception) ;
	tim->usec99 = flip2 (pce->usec_correction) % 100 ;
	headtime = julian(&ta) + (int32_t) (flip4(ptqd->msec_correction)) / 1000.0 + flip2(pce->usec_correction) / 1000000.0 ; /* IGD two flip here */
	break ;
    }
    case DETECTION_RESULT :
    {
	psedr = &log->ce.header_elog.det_res.pick ;
	memcpy (sl->channel_id, psedr->seedname, 3) ;
	memcpy (sl->location_id, psedr->location, 2) ;
	memcpy (net, psedr->ev_network, 2) ;
	memcpy ((pvoid) &stat, (pvoid) &psedr->ev_station, 4) ;
	mh = (log->ce.header_elog.det_res.detection_type == MURDOCK_HUTT) ;
	det = (pvoid) ((uintptr_t) sl + FIRSTDATA) ;
	if (mh)
	    det->blockette_type = 201 ;
	else
	    det->blockette_type = 200 ;
	det->signal_amplitude = (float) flip4(psedr->peak_amplitude) ;  /*IGD flip4 here */
	if (mh)
	    det->signal_period = ((float) flip4 (psedr->period_x_100) ) * 0.01 ;  /*IGD flip4 here */
	det->background_estimate = (float) flip4 (psedr->background_amplitude) ; /*IGD flip4 here */
	sprintf (mots, "%8X", psedr->motion_pick_lookback_quality) ;
	if (mh)
	    det->event_detection_flags = mots[0] - 'C' ;
	else
	    det->event_detection_flags = 4 ;
	gregorian((int32_t) flip4(psedr->jdate), &ta) ;   /*IGD flip4 here*/
	usec = ((int32_t) flip2(psedr->millisec)) * 1000 ;   /*IGD flip2 here */
	seed_time(&ta, usec, &det->signal_onset_time) ;
	if (mh)
	{
	    for (i = 0 ; i <= 4 ; i++)
		det->snr[i] = mots[i + 3] - '0' ;
	    det->lookback_value = mots[2] - '0' ;
	    det->pick_algorithm = mots[1] - 'A' ;
	    padright (psedr->detname, det->s_detname, 24) ;
	}
	else
	{
	    dett = (pvoid) det ;
	    padright (psedr->detname, dett->s_detname, 24) ;
	}
	headtime = (double) flip4 ( psedr->jdate) + SECCOR + flip2(psedr->millisec) / 1000.0 ;  /*IGD flip4 and flip2 here */
	break;
    }
    case CALIBRATION :
    {
	pcr = &log->ce.header_elog.cal_res ;
	memcpy(sl->channel_id, pcr->cr_seedname, 3) ;
	memcpy(sl->location_id, pcr->cr_location, 2) ;
	ta = pcr->cr_time ;
	memcpy(net, pcr->cr_network, 2) ;
	memcpy((pvoid) &stat, (pvoid) &pcr->cr_station, 4) ;
	calstep = (pvoid) ((uintptr_t) sl + FIRSTDATA) ;
	calsine = (pvoid) calstep ;
	calrand = (pvoid) calstep ;
	calabort = (pvoid) calstep ;
	seed_time (&pcr->cr_time, 0, &calstep->fixed.calibration_time) ;
	calstep->calibration_flags = pcr->cr_flags ;
	ltemp.s[0] = pcr->cr_duration ;
	ltemp.s[1] = pcr->cr_duration_low ;
	calstep->calibration_duration = (int32_t)flip4 (ltemp.l) * 10000 ; /*IGD flip4 here -> not in  old porting */
	switch (pcr->cr_type)
	{
	case STEP_CAL :
	{
	    calstep->fixed.blockette_type = 300 ;
	    calstep->number_of_steps = pcr->cr_stepnum ;
	    calstep->interval_duration = 0 ;
	    calend = &calstep->step2 ;
	    break ;
	}
	case SINE_CAL :
	{
	    calsine->fixed.blockette_type = 310 ;
	    calsine->res = 0 ;
	    ltemp.s[0] = pcr->cr_period ;
	    ltemp.s[1] = pcr->cr_period_low ;
	    calsine->sine_period = (int32_t) flip4 (ltemp.l) / 1000.0 ;  /*IGD flip4 here */
	    calend = &calsine->sine2 ;
	    break ;
	}
	case RANDOM_CAL :
	{
	    calrand->fixed.blockette_type = 320 ;
	    calrand->res = 0 ;
	    calend = &calrand->random2 ;
	    if (pcr->cr_flags2 & 2)
		cpadright ("White", calrand->noise_type, 8) ;
	    else
		cpadright ("Red", calrand->noise_type, 8) ;
	    break ;
	} ;
	case ABORT_CAL :
	{
	    calabort->fixed.blockette_type = 395 ;
	    calabort->res = 0 ;
	    calend = NULL ;
	    break ;
	}
	}
	if (calend)
	{
	    ltemp.s[0] = pcr->cr_0dB ;
	    ltemp.s[1] = pcr->cr_0dB_low ;
	    ltemp.f = (float) flip4( ltemp.l );  /* IGD 03/07/01 flip4 */
	    if (ltemp.f == 0.0)
		calend->calibration_amplitude = (float) flip2 (pcr->cr_amplitude) ; /* IGD 03/07/01 flip2 */
	    else
	    {
		dbadj = 1.0 ;
		i = flip2(pcr->cr_amplitude) ; /* IGD 03/07/01 flip2 */
		while (i < 0)
		{
		    dbadj = dbadj * 2.0 ;
		    i = i + 6 ;
		}
		calend->calibration_amplitude = ltemp.f / dbadj ;
	    }
	    memcpy (calend->calibration_input_channel, pcr->cr_input_seedname, 3) ;
	    calend->cal2_res = 0 ;
	    calend->ref_amp = ltemp.f ;
	    if (pcr->cr_flags2 & 1)
		cpadright ("Capacitive", calend->coupling, 12) ;
	    else
		cpadright ("Resistive", calend->coupling, 12) ;
	    if (pcr->cr_filt <= 4)
		strcpy (s, filters[pcr->cr_filt]) ;
	    else
		strcpy (s, "Unknown") ;
	    cpadright (s, calend->rolloff, 12) ;
	}
	headtime = julian (&ta) ;
	break ;
    }
    case END_OF_DETECTION :
    {
	sl->number_of_following_blockettes = 1 ;
	pdob->next_offset = 0 ;
	psedr = &log->ce.header_elog.det_res.pick ;
	memcpy(sl->channel_id, psedr->seedname, 3) ;
	memcpy(sl->location_id, psedr->location, 2) ;
	memcpy(net, psedr->ev_network, 2) ;
	memcpy((pvoid) &stat, (pvoid) &psedr->ev_station, 4) ;
	ta = log->ce.header_elog.det_res.time_of_report ;
	sl->activity_flags = SEED_ACTIVITY_FLAG_END_EVENT ;
	headtime = julian(&ta) ;
	break ;
    }
    } /* switch */

    seedsequence (sl, log->ce.header_flag) ;
    /* station name converted from right justified 4 byte to left justified 5 byte */
    ps = long_str(stat.l) ; /* get string version of station */
    cpadright (ps, sl->station_ID_call_letters, 5) ;
    memcpy (sl->seednet, net, 2) ;
    seed_time (&ta, usec, &sl->starting_time) ;
    return headtime ;
}
