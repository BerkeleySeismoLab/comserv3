



/*   Lib660 Message Log Routines
     Copyright 2017-2021 by
     Kinemetrics, Inc.
     Pasadena, CA 91107 USA.

    This file is part of Lib660

    Lib660 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Lib660 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2017-06-08 rdr Created
    1 2017-07-15 rdr Fix clock type string processing.
    2 2017-07-16 rdr Use correct pointer for loadcalstart.
    3 2018-09-04 rdr Handle sine calibrations of 2Hz or greater. Fix cal abort
                     blockette handling.
    4 2021-01-06 jms fix incorrect use of "addr" operator that resulted in stack
                        corruption when time jump > 250us occurred.
    5 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef liblogs_h
#include "liblogs.h"
#endif

#include "libmsgs.h"
#include "libdetect.h"
#include "libsample.h"
#include "libclient.h"
#include "libsampcfg.h"
#include "libsupport.h"
#include "libarchive.h"
#include "libseed.h"
#include "readpackets.h"

/* copy string "s" to fixed width right padded field "b" */
void lib660_padright (pchar s, pchar b, int fld)
{
    int i, j ;
    string s1 ;

    strcpy (s1, s) ;
    j = (int)strlen(s1) ;

    if (j > fld)
        j = fld ;

    for (i = 0 ; i < j ; i++)
        *b++ = s1[i] ;

    for (i = j ; i < fld ; i++)
        *b++ = ' ' ;
}

/* return two digit string, padded on left with zero */
pchar two (U16 w, pchar s)
{
    int v ;

    v = w ;
    sprintf(s, "%d", v % 100) ;
    zpad (s, 2) ;
    return s ;
}

#ifdef adsfadsfasdf
/* same as zpad but pads on left with spaces */
static pchar spad (pchar s, int lth)
{
    int len, diff ;

    len = strlen(s) ;
    diff = lth - len ;

    if (diff > 0) {
        memmove (&(s[diff]), &(s[0]), len + 1) ; /* shift existing string right */
        memset (&(s[0]), ' ', diff) ; /* add spaces at front */
    }

    return s ;
}

static pchar string_time (tsystemtime *nt, pchar result)
{
    string3 smth, sday, shour, smin, ssec ;

    sprintf(result, "%d-%s-%s %s:%s:%s", nt->wyear, two(nt->wmonth, smth),
            two(nt->wday, sday), two(nt->whour, shour),
            two(nt->wminute, smin), two(nt->wsecond, ssec)) ;
    return result ;
}
#endif

static void fix_time (tseed_time *st)
{
    tsystemtime greg ;
    I32 usec ;

    convert_time (st->seed_fpt, &(greg), &(usec)) ;
    lib660_seed_time (st, &(greg), usec) ;
}

void log_clock (pq660 q660, enum tclock_exception clock_exception, pchar jump_amount)
{
    string s ;
    tsystemtime newtime ;
    I32 newusec ;
    PU8 p ;
    seed_header *phdr ;
    tcom_packet *pcom ;
    plcq q ;
    timing *ptim ;

    q = q660->tim_lcq ;

    if ((q == NIL) || (q->com->ring == NIL))
        return ;

    pcom = q->com ;
    phdr = &(pcom->ring->hdr_buf) ;

    if (q660->need_sats)
        finish_log_clock (q660) ; /* Already one pending, just use current satellite info */

    q660->last_update = q660->data_timetag ;
    memset(&(pcom->ring->rec), 0, LIB_REC_SIZE) ;
    phdr->samples_in_record = 0 ;
    phdr->seed_record_type = 'D' ;
    phdr->continuation_record = ' ' ;
    phdr->sequence.seed_num = pcom->records_written + 1 ;
    (pcom->records_written)++ ;
    memcpy(&(phdr->location_id), &(q->location), sizeof(tlocation)) ;
    memcpy(&(phdr->channel_id), &(q->seedname), sizeof(tseed_name)) ;
    memcpy(&(phdr->station_id_call_letters), &(q660->station), sizeof(tseed_stn)) ;
    memcpy(&(phdr->seednet), &(q660->network), sizeof(tseed_net)) ;
    phdr->starting_time.seed_fpt = q660->data_timetag ;
    phdr->sample_rate_factor = 0 ;
    phdr->sample_rate_multiplier = 1 ;
    phdr->number_of_following_blockettes = 2 ;
    phdr->tenth_msec_correction = 0 ; /* this is a delta, which we don't */
    phdr->first_data_byte = 0 ;
    phdr->first_blockette_byte = 48 ;
    phdr->dob.blockette_type = 1000 ;
    phdr->dob.word_order = 1 ;
    phdr->dob.rec_length = RECORD_EXP ;
    phdr->dob.dob_reserved = 0 ;
    phdr->dob.next_blockette = 56 ;
    phdr->dob.encoding_format = 0 ;
    ptim = &(q660->timing_buf) ;
    ptim->blockette_type = 500 ;
    ptim->next_blockette = 0 ;
    ptim->vco_correction = q660->share.stat_pll.cur_vco / 50.0 ;
    convert_time (q660->data_timetag, &(newtime), &(newusec)) ;
    lib660_seed_time (&(ptim->time_of_exception), &(newtime), newusec) ;
    ptim->usec99 = newusec % 100 ;
    ptim->reception_quality = q660->data_qual ;
    ptim->exception_count = q660->except_count ;
    q660->except_count = 0 ;

    switch (clock_exception) {
    case CE_DAILY :
        lib660_padright ("Daily Timemark", ptim->exception_type, 16) ;
        break ;

    case CE_VALID :
        lib660_padright ("Valid Timemark", ptim->exception_type, 16) ;
        break ;

    case CE_JUMP :
        lib660_padright ("UnExp Timemark", ptim->exception_type, 16) ;
        sprintf (s, "Jump of %s Seconds", jump_amount) ;
        lib660_padright (s, ptim->clock_status, 128) ;
        break ;
    }

    lib660_padright (q660->share.sysinfo.clk_typ, ptim->clock_model, 32) ;

    if (clock_exception == CE_JUMP) {
        p = (PU8)&(pcom->ring->rec) ;
        storeseedhdr (&(p), phdr, FALSE) ;
        storetiming (&(p), ptim) ;
        (q->records_generated_session)++ ;
        q->last_record_generated = secsince () ;
        send_to_client (q660, q, pcom->ring, SCD_BOTH) ;
    } else
        q660->need_sats = TRUE ;
}

void finish_log_clock (pq660 q660)
{
    string s ;
    string7 s1 ;
    U16 w ;
    PU8 p ;
    tcom_packet *pcom ;
    plcq q ;
    timing *ptim ;
    tstat_gpssat *pone ;

    q = q660->tim_lcq ;

    if ((q == NIL) || (q->com->ring == NIL))
        return ;

    pcom = q->com ;
    q660->need_sats = FALSE ;
    ptim = &(q660->timing_buf) ;

    if (q660->share.stat_gps.sat_count) {
        strcpy(s, "SNR=") ;

        for (w = 0 ; w <= q660->share.stat_gps.sat_count - 1 ; w++) {
            pone = &(q660->share.stat_gps.gps_sats[w]) ;

            if ((pone->num) && (pone->snr >= 20)) {
                if (strlen(s) > 4)
                    strcat(s, ",") ;

                sprintf(s1, "%d", pone->snr) ;
                strcat(s, s1) ;
            }
        }
    } else
        s[0] = 0 ;

    lib660_padright (s, ptim->clock_status, 128) ;
    p = (PU8)&(pcom->ring->rec) ;
    storeseedhdr (&(p), (pvoid)&(pcom->ring->hdr_buf), FALSE) ;
    storetiming (&(p), ptim) ;
    (q->records_generated_session)++ ;
    q->last_record_generated = secsince () ;
    send_to_client (q660, q660->tim_lcq, pcom->ring, SCD_BOTH) ;
}

void flush_timing (pq660 q660)
{
    plcq q ;

    q = q660->tim_lcq ;

    if ((q == NIL) || (q->com->ring == NIL))
        return ;

    if ((q->arc.amini_filter) && (q->arc.total_frames > 0))
        flush_archive (q660, q) ;
}

void logevent (pq660 q660, pdet_packet det, tonset_mh *onset)
{
#ifdef dasfadsfsdf
    string on_, s ;
    string63 w ;
    string31 s1, s2 ;
    BOOLEAN mh ;
    tsystemtime newtime ;
    I32 newusec ;
    murdock_detect eblk ;
    threshold_detect *pt ;
    double ts ;
    U8 buffer[FRAME_SIZE] ; /* for creating "blockette image" */
    PU8 p ;
    plcq ppar ;
    pdetect pdef ;

    q660 = paqs->owner ;
    ppar = det->parent ;
    pdef = det->detector_def ;
    mh = (pdef->dtype == MURDOCK_HUTT) ;

    if (mh)
        sprintf(on_, "%c %c %c %c%c%c%c%c", onset->event_detection_flags + 0x63,
                onset->pick_algorithm + 0x41, onset->lookback_value + 0x30,
                onset->snr[0] + 0x30, onset->snr[1] + 0x30, onset->snr[2] + 0x30,
                onset->snr[3] + 0x30, onset->snr[4] + 0x30) ;
    else
        strcpy(on_, "           ") ;

    ts = onset->signal_onset_time.seed_fpt ;
    convert_time (onset->signal_onset_time.seed_fpt, &(newtime), &(newusec)) ;
    sprintf(s2, "%d", newusec) ;
    zpad(s2, 6) ;
    sprintf(w, " %s.%s ", string_time (&(newtime), s1), s2) ;
    strcat(on_, w) ;

    if ((! mh) || (onset->signal_amplitude >= 0)) {
        sprintf(s1, "%d", lib_round(onset->signal_amplitude)) ;
        spad(s1, 10) ;
        strcat(on_, s1) ;
        strcat(on_, " ") ;
    } else
        strcat(on_, "?????????? ") ;

    sprintf(s2, "%d", lib_round(onset->background_estimate)) ;

    if (mh) {
        if ((onset->signal_period > 0) && (onset->signal_period < 1000)) {
            sprintf(s1, "%6.2f ", onset->signal_period) ;
            strcat(on_, s1) ;
        } else
            strcat(on_, "???.?? ") ;

        if (onset->background_estimate >= 0) {
            spad(s2, 7) ;
            strcat(on_, s2) ;
        } else
            strcat(on_, "???????") ;
    } else {
        zpad(s2, 14) ;
        strcat(on_, s2) ;
    }

    strcat(on_, " ") ;
    sprintf(w, "%s:%s", seed2string(ppar->location, ppar->seedname, s2), pdef->detname) ;

    while (strlen(w) < 18)
        strcat(w, " ") ;

    if (det->det_options & DO_MSG) {
        sprintf(s, "%s-%s", w, on_) ;
        libmsgadd(q660, LIBMSG_DETECT, s) ;
    }

    lib660_seed_time (&(eblk.mh_onset.signal_onset_time), &(newtime), newusec) ;
    eblk.mh_onset.signal_amplitude = onset->signal_amplitude ;
    eblk.mh_onset.signal_period = onset->signal_period ;
    eblk.mh_onset.background_estimate = onset->background_estimate ;
    eblk.mh_onset.event_detection_flags = onset->event_detection_flags ;
    eblk.mh_onset.reserved_byte = 0 ;
    pt = (pointer)&(eblk) ;
    p = &(buffer) ;

    if (mh) {
        eblk.blockette_type = 201 ;
        eblk.next_blockette = 0 ;
        memcpy(&(eblk.mh_onset.snr), &(onset->snr), 6) ;
        eblk.mh_onset.lookback_value = onset->lookback_value ;
        eblk.mh_onset.pick_algorithm = onset->pick_algorithm ;
        lib660_padright (&(pdef->detname), &(eblk.s_detname), 24) ;
        storemurdock (&(p), &(eblk)) ;
    } else {
        pt->blockette_type = 200 ;
        pt->next_blockette = 0 ;
        lib660_padright (&(pdef->detname), &(pt->s_detname), 24) ;
        storethreshold (&(p), pt) ;
    }

    if (ppar->lcq_opt & LO_DETP) {
        if (q660->par_create.mini_embed)
            add_blockette (paqs, ppar, (PU16)&(buffer), ts) ;

        if (q660->par_create.mini_separate)
            build_separate_record (paqs, ppar, (PU16)&(buffer), ts, PKC_EVENT) ;
    }

#endif
}

static void set_cal2 (pcal2 pc2, tcalstart *cals, pq660 q660)
{
    int sub ;
    plcq pq ;

    pc2->calibration_amplitude = (cals->amplitude + 1) * -6 ;
    pc2->cal2_res = 0 ;
    pc2->ref_amp = 0 ;
    lib660_padright ("Resistive", pc2->coupling, 12) ;
    lib660_padright ("3DB@10Hz", pc2->rolloff, 12) ;
    memset(&(pc2->calibration_input_channel), ' ', 3) ;

    for (sub = FREQS - 1 ; sub >= 0 ; sub--) {
        pq = q660->mdispatch[CAL_CHANNEL][sub] ;

        if (pq) {
            memcpy (&(pc2->calibration_input_channel), &(pq->seedname), 3) ;
            break ;
        }
    }
}

void log_cal (pq660 q660, PU8 pb, BOOLEAN start)
{
    tcalstart cals ;
    plcq q ;
    int idx, sub ;
    U16 map ;
    step_calibration *pstep ;
    sine_calibration *psine ;
    random_calibration *prand ;
    abort_calibration *pabort ;
    random_calibration cblk ;
    U8 buffer[FRAME_SIZE] ; /* for creating "blockette image" */
    PU8 p ;

    memclr(&(cblk), sizeof(random_calibration)) ;
    p = (PU8)&(buffer) ;

    if (start) {
        loadcalstart (&(pb), &(cals)) ;
        map = cals.calbit_map ;
        psine = (pvoid)&(cblk) ;
        /* common for all waveforms */
        psine->next_blockette = 0 ;
        psine->calibration_time.seed_fpt = q660->data_timetag ;
        fix_time (&(psine->calibration_time)) ;

        if (cals.waveform & 0x80)
            psine->calibration_flags = 4 ; /* automatic */
        else
            psine->calibration_flags = 0 ;

        psine->calibration_duration = (I32)cals.duration * 10000 ;

        switch ((U8)(cals.waveform & 7)) {
        case 0 : /* sine */
            psine->blockette_type = 310 ;
            psine->calibration_flags = psine->calibration_flags | 0x10 ;

            if (cals.freqdiv & 0x8000)
                psine->sine_period = 1.0 / (cals.freqdiv & 0xFF) ; /* 2Hz to 20Hz */
            else
                psine->sine_period = cals.freqdiv ;

            set_cal2 (&(psine->sine2), &(cals), q660) ;
            storesine (&(p), psine) ;
            break ;

        case 1 :
        case 2 :
        case 4 : /* random */
            prand = &(cblk) ;
            prand->blockette_type = 320 ;

            /* white uses random amplitudes, red and telegraph don't */
            if ((U8)(cals.waveform & 7) == 2)
                prand->calibration_flags = prand->calibration_flags | 0x10 ;

            set_cal2 (&(prand->random2), &(cals), q660) ;

            switch ((U8)(cals.waveform & 7)) {
            case 1 :
                lib660_padright("Red",prand->noise_type, 8) ;
                break ;

            case 2 :
                lib660_padright("White", prand->noise_type, 8) ;
                break ;

            case 4 :
                lib660_padright("Telegraf", prand->noise_type, 8) ;
                break ;
            }

            storerandom (&(p), prand) ;
            break ;

        case 3 : /* step */
            pstep = (pvoid)&(cblk) ;
            pstep->blockette_type = 300 ;
            pstep->number_of_steps = 1 ;

            if ((cals.waveform & 0x40) == 0)
                pstep->calibration_flags = pstep->calibration_flags | 1 ; /* positive pulse */

            pstep->interval_duration = 0 ;
            set_cal2 (&(pstep->step2), &(cals), q660) ;
            storestep (&(p), pstep) ;
            break ;
        }
    } else {
        loadbyte (&(pb)) ;
        loadbyte (&(pb)) ;
        map = loadword (&(pb)) ;
        pabort = (pvoid)&(cblk) ;
        pabort->blockette_type = 395 ;
        pabort->next_blockette = 0 ;
        pabort->calibration_time.seed_fpt = q660->data_timetag ;
        fix_time (&(pabort->calibration_time)) ;
        storeabort (&(p), pabort) ;
    } ;

    /* need a record for each channel in the map */
    for (idx = 0 ; idx <= TOTAL_CHANNELS - 1 ; idx++)
        if (map & (1 << idx))
            for (sub = 0 ; sub <= FREQS - 1 ; sub++) {
                q = q660->mdispatch[idx][sub] ;

                if (q) {
                    (q->calibrations_session)++ ;

                    if (q660->par_create.mini_embed)
                        add_blockette (q660, q, (PU16)&(buffer), q660->data_timetag) ;

                    if (q660->par_create.mini_separate)
                        build_separate_record (q660, q, (PU16)&(buffer), q660->data_timetag, PKC_CALIBRATE) ;
                }
            }
}

void log_message (pq660 q660, pchar msg)
{
    int i ;
    double ts ;
    PU8 p ;
    pchar pc ;
    plcq q ;
    tcom_packet *pcom ;
    seed_header *phdr ;

    q = q660->msg_lcq ;

    if ((q == NIL) || (q->com->ring == NIL))
        return ; /* not initialized */

    ts = now() ;
    pcom = q->com ;
    phdr = &(pcom->ring->hdr_buf) ;
    i = (int)strlen(msg) + 2 ;

    if ((pcom->frame >= 2) &&
            (((pcom->frame + i) > NONDATA_AREA) || (ts > (phdr->starting_time.seed_fpt + 60))))

    { /* won't fit in current frame or was too long ago */
        phdr->samples_in_record = pcom->frame ;
        q660->nested_log = TRUE ;
        p = (PU8)&(pcom->ring->rec) ;
        storeseedhdr (&(p), (pvoid)phdr, FALSE) ;
        (q->records_generated_session)++ ;
        q->last_record_generated = secsince () ;
        send_to_client (q660, q, pcom->ring, SCD_BOTH) ;
        q660->nested_log = FALSE ;
        pcom->frame = 0 ;
    }

    if (pcom->frame < 2) {
        /* at least a blank line */
        memset(&(pcom->ring->rec), 0, LIB_REC_SIZE) ;
        phdr->samples_in_record = 0 ;
        pcom->frame = 0 ;
        phdr->starting_time.seed_fpt = ts ;
        phdr->seed_record_type = 'D' ;
        phdr->continuation_record = ' ' ;
        phdr->sequence.seed_num = pcom->records_written + 1 ;
        (pcom->records_written)++ ;
        memcpy(&(phdr->location_id), &(q->location), sizeof(tlocation)) ;
        memcpy(&(phdr->channel_id), &(q->seedname), sizeof(tseed_name)) ;
        memcpy(&(phdr->station_id_call_letters), &(q660->station), sizeof(tseed_stn)) ;
        memcpy(&(phdr->seednet), &(q660->network), sizeof(tseed_net)) ;
        phdr->sample_rate_factor = 0 ;
        phdr->sample_rate_multiplier = 1 ;
        phdr->number_of_following_blockettes = 1 ;
        phdr->tenth_msec_correction = 0 ; /* this is a delta, which we don't */
        phdr->first_data_byte = 56 ;
        phdr->first_blockette_byte = 48 ;
        phdr->dob.blockette_type = 1000 ;
        phdr->dob.word_order = 1 ;
        phdr->dob.rec_length = RECORD_EXP ;
        phdr->dob.dob_reserved = 0 ;
        phdr->dob.next_blockette = 0 ;
        phdr->dob.encoding_format = 0 ;
    }

    strcat(msg, "\x0D\x0A") ;
    pc = (pointer)((PNTRINT)&(pcom->ring->rec) + 56 + pcom->frame) ; /* add text to data area */
    memcpy(pc, msg, strlen(msg)) ;
    pcom->frame = pcom->frame + (strlen(msg)) ;
    q660->log_timer = LOG_TIMEOUT ;
}

void flush_messages (pq660 q660)
{
    PU8 p ;
    plcq q ;
    tcom_packet *pcom ;
    seed_header *phdr ;

    q = q660->msg_lcq ;

    if ((q == NIL) || (q->com->ring == NIL))
        return ; /* not initialized */

    pcom = q->com ;
    phdr = &(pcom->ring->hdr_buf) ;

    if (pcom->frame >= 2) {
        phdr->samples_in_record = pcom->frame ;
        q660->nested_log = TRUE ;
        p = (PU8)&(pcom->ring->rec) ;
        storeseedhdr (&(p), (pvoid)phdr, FALSE) ;
        (q->records_generated_session)++ ;
        q->last_record_generated = secsince () ;
        send_to_client (q660, q, pcom->ring, SCD_BOTH) ;
        q660->nested_log = FALSE ;
        pcom->frame = 0 ;
    }

    if ((q->arc.amini_filter) && (q->arc.hdr_buf.samples_in_record))
        flush_archive (q660, q) ;

    q660->log_timer = 0 ;
}


