/*  Libmsmcast Seed Routines

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
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
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#include "libseed.h"
#include "libstrucs.h"
#include "libsupport.h"

/* convert seedname and location into string */
pchar seed2string(tlocation loc, tseed_name sn, pchar result)
{
    string15 s ;
    integer lth ;

    lth = 0 ;
    if (loc[0] != ' ')
    {
        s[lth++] = loc[0] ;
        if (loc[1] != ' ')
            s[lth++] = loc[1] ;
        s[lth++] = '-' ;
    }
    s[lth++] = sn[0] ;
    s[lth++] = sn[1] ;
    s[lth++] = sn[2] ;
    s[lth] = 0 ;
    strcpy(result, s) ;
    return result ;
}

/* convert C string to fixed width field */
void string2fixed(pointer p, pchar s)
{

    memcpy(p, s, strlen(s)) ;
}

void lib660_seed_time (tseed_time *st, tsystemtime *greg, longint usec)
{

    st->seed_yr = greg->wyear ;
    st->seed_hr = greg->whour ;
    st->seed_minute = greg->wminute ;
    st->seed_seconds = greg->wsecond ;
    st->seed_unused = 0 ;
    st->seed_tenth_millisec = usec / 100 ;
    st->seed_jday = day_julian (greg->wyear, greg->wmonth, greg->wday) ;
}

void fix_seed_header (seed_header *hdr, tsystemtime *greg,
		      longint usec, boolean setdeb)
{
    string7 s ;
    longword recnum ;

    recnum = ((hdr->sequence.seed_num - 1) % 999999) + 1 ; /* restrict to 1 .. 999999 */
    sprintf(s, "%d", (integer)recnum) ;
    zpad(s, 6) ;
    memcpy(addr(hdr->sequence.seed_num), addr(s), 6) ;
    lib660_seed_time (addr(hdr->starting_time), greg, usec) ;
    if (setdeb)
	hdr->deb.usec99 = usec % 100 ;
}

void convert_time (double fp, tsystemtime *greg, longint *usec)
{
    longint jul ;
    double fjul, fusec ;

    jul = lib_round (fp) ;
    fjul = jul ;
    fusec = (fp - fjul) * 1.0E6 ;
    *usec = lib_round(fusec) ;
    if (*usec < 0)
    {
        jul = jul - 1 ;
        *usec = *usec + 1000000 ;
    }
    if (*usec >= 1000000)
    {
        jul = jul + 1 ;
        *usec = *usec - 1000000 ;
    }
    lib660_gregorian (jul, greg) ;
}

double extract_time (tseed_time *st, byte usec)
{
    tsystemtime greg ;
    word j, dim ;
    double time ;

    j = st->seed_jday ;
    greg.wyear = st->seed_yr ;
    greg.whour = st->seed_hr ;
    greg.wminute = st->seed_minute ;
    greg.wsecond = st->seed_seconds ;
    greg.wmonth = 1 ;
    do
    {
	dim = days_mth[greg.wmonth] ;
	if (((greg.wyear % 4) == 0) && (greg.wmonth == 2))
	    dim = 29 ;
	if (j <= dim)
	    break ;
	j = j - dim ;
	inc(greg.wmonth) ;
    } while (! (j > 400)) ; /* just in case */
    greg.wday = j ;
    time = lib660_julian(addr(greg)) ; /* seconds since 2016 */
    return time + st->seed_tenth_millisec / 10000.0 + usec * 0.000001 ;
}

void storebyte (pbyte *p, byte b)
{

    **p = b ;
    inc(*p) ;
}

void storeword (pbyte *p, word w)
{

#ifdef ENDIAN_LITTLE
    w = htons(w) ;
#endif
    memcpy(*p, addr(w), 2) ;
    incn(*p, 2) ;
}

void storeint16 (pbyte *p, int16 i)
{

#ifdef ENDIAN_LITTLE
    i = htons(i) ;
#endif
    memcpy(*p, addr(i), 2) ;
    incn(*p, 2) ;
}

void storelongword (pbyte *p, longword lw)
{

#ifdef ENDIAN_LITTLE
    lw = htonl(lw) ;
#endif
    memcpy(*p, addr(lw), 4) ;
    incn(*p, 4) ;
}

void storelongint (pbyte *p, longint li)
{

#ifdef ENDIAN_LITTLE
    li = htonl(li) ;
#endif
    memcpy(*p, addr(li), 4) ;
    incn(*p, 4) ;
}

void storesingle (pbyte *p, single s)
{
    longint li ;

    memcpy(addr(li), addr(s), 4) ;
#ifdef ENDIAN_LITTLE
    li = htonl(li) ;
#endif
    memcpy(*p, addr(li), 4) ;
    incn(*p, 4) ;
}

void storeblock (pbyte *p, integer size, pointer psrc)
{

    memcpy(*p, psrc, size) ;
    incn(*p, size) ;
}

void storetime (pbyte *p, tseed_time *seedtime)
{

    storeword (p, seedtime->seed_yr) ;
    storeword (p, seedtime->seed_jday) ;
    storebyte (p, seedtime->seed_hr) ;
    storebyte (p, seedtime->seed_minute) ;
    storebyte (p, seedtime->seed_seconds) ;
    storebyte (p, seedtime->seed_unused) ;
    storeword (p, seedtime->seed_tenth_millisec) ;
}

void storeseedhdr (pbyte *pdest, seed_header *hdr, boolean hasdeb)
{
    tsystemtime newtime ;
    longint newusec ;
    double time_save ;
    longword seq_save ;

    time_save = hdr->starting_time.seed_fpt ;
    seq_save = hdr->sequence.seed_num ;
    convert_time (hdr->starting_time.seed_fpt, addr(newtime), addr(newusec)) ;
    fix_seed_header (hdr, addr(newtime), newusec, hasdeb) ;
    storeblock (pdest, 6, addr(hdr->sequence.seed_ch)) ;
    storebyte (pdest, (byte)hdr->seed_record_type) ;
    storebyte (pdest, (byte)hdr->continuation_record) ;
    storeblock (pdest, 5, addr(hdr->station_id_call_letters)) ;
    storeblock (pdest, 2, addr(hdr->location_id)) ;
    storeblock (pdest, 3, addr(hdr->channel_id)) ;
    storeblock (pdest, 2, addr(hdr->seednet)) ;
    storetime (pdest, addr(hdr->starting_time)) ;
    storeword (pdest, hdr->samples_in_record) ;
    storeint16 (pdest, hdr->sample_rate_factor) ;
    storeint16 (pdest, hdr->sample_rate_multiplier) ;
    storebyte (pdest, hdr->activity_flags) ;
    storebyte (pdest, hdr->io_flags) ;
    storebyte (pdest, hdr->data_quality_flags) ;
    storebyte (pdest, hdr->number_of_following_blockettes) ;
    storelongint (pdest, hdr->tenth_msec_correction) ;
    storeword (pdest, hdr->first_data_byte) ;
    storeword (pdest, hdr->first_blockette_byte) ;
    /* all seed records have this one */
    storeword (pdest, hdr->dob.blockette_type) ;
    storeword (pdest, hdr->dob.next_blockette) ;
    storebyte (pdest, hdr->dob.encoding_format) ;
    storebyte (pdest, hdr->dob.word_order) ;
    storebyte (pdest, hdr->dob.rec_length) ;
    storebyte (pdest, hdr->dob.dob_reserved) ;
    /* only data records have this one */
    if (hasdeb)
    {
        storeword (pdest, hdr->deb.blockette_type) ;
        storeword (pdest, hdr->deb.next_blockette) ;
        storebyte (pdest, hdr->deb.qual) ;
        storebyte (pdest, hdr->deb.usec99) ;
        storebyte (pdest, hdr->deb.deb_flags) ;
        storebyte (pdest, hdr->deb.frame_count) ;
    }
    hdr->starting_time.seed_fpt = time_save ;
    hdr->sequence.seed_num = seq_save ;
}

void storethreshold (pbyte *pdest, threshold_detect *tdet)
{

    storeword (pdest, tdet->blockette_type) ;
    storeword (pdest, tdet->next_blockette) ;
    storesingle (pdest, tdet->thr_onset.signal_amplitude) ;
    storesingle (pdest, tdet->thr_onset.signal_period) ;
    storesingle (pdest, tdet->thr_onset.background_estimate) ;
    storebyte (pdest, tdet->thr_onset.event_detection_flags) ;
    storebyte (pdest, tdet->thr_onset.reserved_byte) ;
    storetime (pdest, addr(tdet->thr_onset.signal_onset_time)) ;
    storeblock (pdest, 24, addr(tdet->s_detname)) ;
}

void storetiming (pbyte *pdest, timing *tim)
{

    storeword (pdest, tim->blockette_type) ;
    storeword (pdest, tim->next_blockette) ;
    storesingle (pdest, tim->vco_correction) ;
    storetime (pdest, addr(tim->time_of_exception)) ;
    storebyte (pdest, tim->usec99) ;
    storebyte (pdest, tim->reception_quality) ;
    storelongint (pdest, tim->exception_count) ;
    storeblock (pdest, 16, addr(tim->exception_type)) ;
    storeblock (pdest, 32, addr(tim->clock_model)) ;
    storeblock (pdest, 128, addr(tim->clock_status)) ;
}

void storecal2 (pbyte *pdest, cal2 *c2)
{

    storesingle (pdest, c2->calibration_amplitude) ;
    storeblock (pdest, 3, addr(c2->calibration_input_channel)) ;
    storebyte (pdest, c2->cal2_res) ;
    storesingle (pdest, c2->ref_amp) ;
    storeblock (pdest, 12, addr(c2->coupling)) ;
    storeblock (pdest, 12, addr(c2->rolloff)) ;
}

void storestep (pbyte *pdest, step_calibration *stepcal)
{

    storeword (pdest, stepcal->blockette_type) ;
    storeword (pdest, stepcal->next_blockette) ;
    storetime (pdest, addr(stepcal->calibration_time)) ;
    storebyte (pdest, stepcal->number_of_steps) ;
    storebyte (pdest, stepcal->calibration_flags) ;
    storelongword (pdest, stepcal->calibration_duration) ;
    storelongword (pdest, stepcal->interval_duration) ;
    storecal2 (pdest, addr(stepcal->step2)) ;
}

void storesine (pbyte *pdest, sine_calibration *sinecal)
{

    storeword (pdest, sinecal->blockette_type) ;
    storeword (pdest, sinecal->next_blockette) ;
    storetime (pdest, addr(sinecal->calibration_time)) ;
    storebyte (pdest, sinecal->res) ;
    storebyte (pdest, sinecal->calibration_flags) ;
    storelongword (pdest, sinecal->calibration_duration) ;
    storesingle (pdest, sinecal->sine_period) ;
    storecal2 (pdest, addr(sinecal->sine2)) ;
}

void storerandom (pbyte *pdest, random_calibration *randcal)
{

    storeword (pdest, randcal->blockette_type) ;
    storeword (pdest, randcal->next_blockette) ;
    storetime (pdest, addr(randcal->calibration_time)) ;
    storebyte (pdest, randcal->res) ;
    storebyte (pdest, randcal->calibration_flags) ;
    storelongword (pdest, randcal->calibration_duration) ;
    storecal2 (pdest, addr(randcal->random2)) ;
    storeblock (pdest, 8, addr(randcal->noise_type)) ;
}

void storeabort (pbyte *pdest, abort_calibration *abortcal)
{

    storeword (pdest, abortcal->blockette_type) ;
    storeword (pdest, abortcal->next_blockette) ;
    storetime (pdest, addr(abortcal->calibration_time)) ;
    storeword (pdest, abortcal->res) ;
}

void storeopaque (pbyte *pdest, topaque_hdr *ophdr,
		  pointer pbuf, integer psize)
{

    storeword (pdest, ophdr->blockette_type) ;
    storeword (pdest, ophdr->next_blockette) ;
    storeword (pdest, ophdr->blk_lth) ;
    storeword (pdest, ophdr->data_off) ;
    storelongword (pdest, ophdr->recnum) ;
    storebyte (pdest, ophdr->word_order) ;
    storebyte (pdest, ophdr->opaque_flags) ;
    storebyte (pdest, ophdr->hdr_fields) ;
    storeblock (pdest, 5, addr(ophdr->rec_type)) ;
    storeblock (pdest, psize, pbuf) ;
}

/* we this inline instead of calling storelongword to save the procedure call */
void storeframe (pbyte *pdest, compressed_frame *cf)
{
    integer i ;
    longword lw ;

    for (i = 0 ; i <= WORDS_PER_FRAME - 1 ; i++)
    {
	lw = (*cf)[i] ;
#ifdef ENDIAN_LITTLE
	lw = htonl(lw) ;
#endif
	memcpy(*pdest, addr(lw), 4) ;
	incn(*pdest, 4) ;
    }
}

void loadblkhdr (pbyte *p, blk_min *blk)
{

    blk->blockette_type = loadword (p) ;
    blk->next_blockette = loadword (p) ;
}

void loadtime (pbyte *p, tseed_time *seedtime)
{

    seedtime->seed_yr = loadword (p) ;
    seedtime->seed_jday = loadword (p) ;
    seedtime->seed_hr = loadbyte (p) ;
    seedtime->seed_minute = loadbyte (p) ;
    seedtime->seed_seconds = loadbyte (p) ;
    seedtime->seed_unused = loadbyte (p) ;
    seedtime->seed_tenth_millisec = loadword (p) ;
}

void loadseedhdr (pbyte *psrc, seed_header *hdr, boolean hasdeb)
{

    loadblock (psrc, 6, addr(hdr->sequence.seed_ch)) ;
    hdr->seed_record_type = (char)loadbyte (psrc) ;
    hdr->continuation_record = (char)loadbyte (psrc) ;
    loadblock (psrc, 5, addr(hdr->station_id_call_letters)) ;
    loadblock (psrc, 2, addr(hdr->location_id)) ;
    loadblock (psrc, 3, addr(hdr->channel_id)) ;
    loadblock (psrc, 2, addr(hdr->seednet)) ;
    loadtime (psrc, addr(hdr->starting_time)) ;
    hdr->samples_in_record = loadword (psrc) ;
    hdr->sample_rate_factor = loadint16 (psrc) ;
    hdr->sample_rate_multiplier = loadint16 (psrc) ;
    hdr->activity_flags = loadbyte (psrc) ;
    hdr->io_flags = loadbyte (psrc) ;
    hdr->data_quality_flags = loadbyte (psrc) ;
    hdr->number_of_following_blockettes = loadbyte (psrc) ;
    hdr->tenth_msec_correction = loadlongint (psrc) ;
    hdr->first_data_byte = loadword (psrc) ;
    hdr->first_blockette_byte = loadword (psrc) ;
    hdr->dob.blockette_type = loadword (psrc) ;
    hdr->dob.next_blockette = loadword (psrc) ;
    hdr->dob.encoding_format = loadbyte (psrc) ;
    hdr->dob.word_order = loadbyte (psrc) ;
    hdr->dob.rec_length = loadbyte (psrc) ;
    hdr->dob.dob_reserved = loadbyte (psrc) ;
    if (hasdeb)
    {
        hdr->deb.blockette_type = loadword (psrc) ;
        hdr->deb.next_blockette = loadword (psrc) ;
        hdr->deb.qual = loadbyte (psrc) ;
        hdr->deb.usec99 = loadbyte (psrc) ;
        hdr->deb.deb_flags = loadbyte (psrc) ;
        hdr->deb.frame_count = loadbyte (psrc) ;
    }
}

/* Return tpacket_class value on success, -1 on failure */
int loadseedhdr_classify (pbyte *psrc, seed_header *hdr, int recsize)
{
    /* psrc is ptr to ptr to start of miniseed record */
    int pkt_type = -1 ;
    int next_blockette ;
    pbyte pmsrec = *psrc ; /* save pointer to start of miniseed record */
    word blockette_type ;
    int rec_len;
    int dob_found = 0 ;
    int deb_found = 0 ;
    int ok = 0;

    loadblock (psrc, 6, addr(hdr->sequence.seed_ch)) ;
    hdr->seed_record_type = (char)loadbyte (psrc) ;
    hdr->continuation_record = (char)loadbyte (psrc) ;
    loadblock (psrc, 5, addr(hdr->station_id_call_letters)) ;
    loadblock (psrc, 2, addr(hdr->location_id)) ;
    loadblock (psrc, 3, addr(hdr->channel_id)) ;
    loadblock (psrc, 2, addr(hdr->seednet)) ;
    loadtime (psrc, addr(hdr->starting_time)) ;
    hdr->samples_in_record = loadword (psrc) ;
    hdr->sample_rate_factor = loadint16 (psrc) ;
    hdr->sample_rate_multiplier = loadint16 (psrc) ;
    hdr->activity_flags = loadbyte (psrc) ;
    hdr->io_flags = loadbyte (psrc) ;
    hdr->data_quality_flags = loadbyte (psrc) ;
    hdr->number_of_following_blockettes = loadbyte (psrc) ;
    hdr->tenth_msec_correction = loadlongint (psrc) ;
    hdr->first_data_byte = loadword (psrc) ;
    hdr->first_blockette_byte = loadword (psrc) ;

    /* Blockette offsets are from the beginning of the mseed hdr. */
    /* Basic sanity check for blockette offset */
    if (hdr->first_blockette_byte < 48 || hdr->first_blockette_byte >= recsize)
    {
	return -1 ;
    }

    /* Load dob (required) and deb (optional) blockettes. */
    /* Do not make assumptions about the order of blockettes. */

    /* Check for dob (Data Only Blockette = 1000) */
    next_blockette = hdr->first_blockette_byte;
    while (next_blockette != 0) {
	if (next_blockette == 0)
	{
	    break; /* No more blockettes */
	}
	/* Basic sanity check for blockette offset */
	if (hdr->first_blockette_byte < 48 || hdr->first_blockette_byte >= recsize)
	{
	    return -1;
	}
	/* Since next_blockette contains the byte offset of the blockette */
	/* from the start of the MiniSEED record, explicitly set *psrc to */
	/* point to the blockette. */
	/* Look-ahead at blockette type */
	*psrc = pmsrec + next_blockette;
	blockette_type = loadword (psrc);
	/* Reset pointer to point to blockette again. */
	*psrc = pmsrec + next_blockette;
	if (blockette_type == 1000)
	{
	    /* dob blockette */
	    hdr->dob.blockette_type = loadword (psrc) ;
	    hdr->dob.next_blockette = loadword (psrc) ;
	    hdr->dob.encoding_format = loadbyte (psrc) ;
	    hdr->dob.word_order = loadbyte (psrc) ;
	    hdr->dob.rec_length = loadbyte (psrc) ;
	    hdr->dob.dob_reserved = loadbyte (psrc) ;
	    next_blockette = hdr->dob.next_blockette;
	    dob_found = TRUE ;
	}
	else if (blockette_type == 1001) 
	{
	    /* deb blockette */
	    hdr->deb.blockette_type = loadword (psrc) ;
	    hdr->deb.next_blockette = loadword (psrc) ;
	    hdr->deb.qual = loadbyte (psrc) ;
	    hdr->deb.usec99 = loadbyte (psrc) ;
	    hdr->deb.deb_flags = loadbyte (psrc) ;
	    hdr->deb.frame_count = loadbyte (psrc) ;
	    next_blockette = hdr->deb.next_blockette;
	    deb_found = TRUE ;
	}
	else {
	    /* Read blockette header and skip over its contents. */
	    blockette_type = loadword (psrc) ;
	    next_blockette = loadword (psrc) ;
	    *psrc = pmsrec + next_blockette;	/* Skip over blockette contents. */
	    /* Check blockette type for classifying this record. */
	    if (blockette_type >= 200 && blockette_type <= 299)
		pkt_type = PKC_EVENT;
	    else if (blockette_type >= 300 && blockette_type <= 399)
		pkt_type = PKC_CALIBRATE;
	    else if (blockette_type == 500)
		pkt_type = PKC_TIMING;
	    else if (blockette_type = 2000)
		pkt_type = PKC_OPAQUE;
	}
    }

    /* Perform minimal mseed integrity checks. */
    if (dob_found) 
    {
	rec_len - hdr->dob.rec_length;
	if (rec_len >= 7 && rec_len <= 9 && exp2(rec_len) == recsize)
	{
	    ok = TRUE;  /* We have a mseed header */
	}
    }

    /* Finish packet classification for PKT_DATA and PKT_MESSAGE */
    if (pkt_type < 0) {
	if (hdr->sample_rate_factor == 0 && hdr->samples_in_record > 0)
	    /* Sample rate of 0 with samples implies MESSAGE record. */
	    pkt_type = PKC_MESSAGE;
	else if (hdr->sample_rate_factor != 0 && hdr->samples_in_record > 0)
	    /* Sampel rate != 0 and samples implies DATA record. */
	    pkt_type = PKC_DATA;
	else
	    /* Unknown or corrupt record. */
	    pkt_type = -1;
    }
    return pkt_type ;
}

void loadtiming (pbyte *psrc, timing *tim)
{

    tim->blockette_type = loadword (psrc) ;
    tim->next_blockette = loadword (psrc) ;
    tim->vco_correction = loadsingle (psrc) ;
    loadtime (psrc, addr(tim->time_of_exception)) ;
    tim->usec99 = loadbyte (psrc) ;
    tim->reception_quality = loadbyte (psrc) ;
    tim->exception_count = loadlongint (psrc) ;
    loadblock (psrc, 16, addr(tim->exception_type)) ;
    loadblock (psrc, 32, addr(tim->clock_model)) ;
    loadblock (psrc, 128, addr(tim->clock_status)) ;
}

void loadmurdock (pbyte *psrc, murdock_detect *mdet)
{

    mdet->blockette_type = loadword (psrc) ;
    mdet->next_blockette = loadword (psrc) ;
    mdet->mh_onset.signal_amplitude = loadsingle (psrc) ;
    mdet->mh_onset.signal_period = loadsingle (psrc) ;
    mdet->mh_onset.background_estimate = loadsingle (psrc) ;
    mdet->mh_onset.event_detection_flags = loadbyte (psrc) ;
    mdet->mh_onset.reserved_byte = loadbyte (psrc) ;
    loadtime (psrc, addr(mdet->mh_onset.signal_onset_time)) ;
    loadblock (psrc, 6, addr(mdet->mh_onset.snr)) ;
    mdet->mh_onset.lookback_value = loadbyte (psrc) ;
    mdet->mh_onset.pick_algorithm = loadbyte (psrc) ;
    loadblock (psrc, 24, addr(mdet->s_detname)) ;
}

void loadthreshold (pbyte *psrc, threshold_detect *tdet)
{

    tdet->blockette_type = loadword (psrc) ;
    tdet->next_blockette = loadword (psrc) ;
    tdet->thr_onset.signal_amplitude = loadsingle (psrc) ;
    tdet->thr_onset.signal_period = loadsingle (psrc) ;
    tdet->thr_onset.background_estimate = loadsingle (psrc) ;
    tdet->thr_onset.event_detection_flags = loadbyte (psrc) ;
    tdet->thr_onset.reserved_byte = loadbyte (psrc) ;
    loadtime (psrc, addr(tdet->thr_onset.signal_onset_time)) ;
    loadblock (psrc, 24, addr(tdet->s_detname)) ;
}

void loadcal2 (pbyte *psrc, cal2 *c2)
{

    c2->calibration_amplitude = loadsingle (psrc) ;
    loadblock (psrc, 3, addr(c2->calibration_input_channel)) ;
    c2->cal2_res = loadbyte (psrc) ;
    c2->ref_amp = loadsingle (psrc) ;
    loadblock (psrc, 12, addr(c2->coupling)) ;
    loadblock (psrc, 12, addr(c2->rolloff)) ;
}

void loadstep (pbyte *psrc, step_calibration *stepcal)
{

    stepcal->blockette_type = loadword (psrc) ;
    stepcal->next_blockette = loadword (psrc) ;
    loadtime (psrc, addr(stepcal->calibration_time)) ;
    stepcal->number_of_steps = loadbyte (psrc) ;
    stepcal->calibration_flags = loadbyte (psrc) ;
    stepcal->calibration_duration = loadlongword (psrc) ;
    stepcal->interval_duration = loadlongword (psrc) ;
    loadcal2 (psrc, addr(stepcal->step2)) ;
}

void loadsine (pbyte *psrc, sine_calibration *sinecal)
{

    sinecal->blockette_type = loadword (psrc) ;
    sinecal->next_blockette = loadword (psrc) ;
    loadtime (psrc, addr(sinecal->calibration_time)) ;
    sinecal->res = loadbyte (psrc) ;
    sinecal->calibration_flags = loadbyte (psrc) ;
    sinecal->calibration_duration = loadlongword (psrc) ;
    sinecal->sine_period = loadsingle (psrc) ;
    loadcal2 (psrc, addr(sinecal->sine2)) ;
}

void loadrandom (pbyte *psrc, random_calibration *randcal)
{

    randcal->blockette_type = loadword (psrc) ;
    randcal->next_blockette = loadword (psrc) ;
    loadtime (psrc, addr(randcal->calibration_time)) ;
    randcal->res = loadbyte (psrc) ;
    randcal->calibration_flags = loadbyte (psrc) ;
    randcal->calibration_duration = loadlongword (psrc) ;
    loadcal2 (psrc, addr(randcal->random2)) ;
    loadblock (psrc, 8, addr(randcal->noise_type)) ;
}

void loadabort (pbyte *psrc, abort_calibration *abortcal)
{

    abortcal->blockette_type = loadword (psrc) ;
    abortcal->next_blockette = loadword (psrc) ;
    loadtime (psrc, addr(abortcal->calibration_time)) ;
    abortcal->res = loadword (psrc) ;
}

void loadopaquehdr (pbyte *psrc, topaque_hdr *ophdr)
{

    ophdr->blockette_type = loadword (psrc) ;
    ophdr->next_blockette = loadword (psrc) ;
    ophdr->blk_lth = loadword (psrc) ;
    ophdr->data_off = loadword (psrc) ;
    ophdr->recnum = loadlongword (psrc) ;
    ophdr->word_order = loadbyte (psrc) ;
    ophdr->opaque_flags = loadbyte (psrc) ;
    ophdr->hdr_fields = loadbyte (psrc) ;
    loadblock (psrc, 5, addr(ophdr->rec_type)) ;
}
