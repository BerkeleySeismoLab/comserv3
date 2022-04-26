



/*   Lib660 Time Series data routines
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
    0 2017       rdr Created
    1 2018-09-24 rdr In process_compressed check channel and frequency fields
                     to make sure they are in range.
    2 2019-07-11 rdr Add support for one second callback.
    3 2019-08-21 rdr Set sample_count for one second callback.
    4 2019-11-12 rdr In process_compressed look for lowish-latency data and if so
                     process it.
    5 2019-11-12 jld Mod per Joe to allow 0.1 SPS data from 1-sec callback.
    6 2021-12-11 jms various temporary debugging prints.
    7 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/

#undef LINUXDEBUGPRINT

#include "libsample.h"
#include "libsampglob.h"
#include "libsampcfg.h"
#include "libcompress.h"
#include "libmsgs.h"
#include "libsupport.h"
#include "libfilters.h"
#include "libdetect.h"
#include "liblogs.h"
#include "libarchive.h"
#include "libopaque.h"
#include "libdataserv.h"

typedef U8 tmasks[4] ;
const tmasks DMASKS = {0x0, 0xC0, 0xF0, 0xFC} ;
const tmasks SMASKS = {0xFF, 0x3F, 0x0F, 0x03} ;

I32 sex (I32 l)
{

    if (l & 0x8000)
        l = l | (I32)0xFFFF0000 ;

    return l ;
}

I32 seqspread (U32 new_, U32 last)
{
    I32 d ;

    d = new_ - last ; /* returns a signed value, which takes care of 32-bit wrapping */
    return d ;
}

static BOOLEAN detect_record (pq660 q660, pdet_packet det, plcq q)
{
    BOOLEAN have_detection, result ;
    tonset_mh onset_save ;
    con_common *pcc ;

    pcc = det->cont ;
    have_detection = FALSE ;
    result = FALSE ;

    if (pcc->detector_enabled) {
        do {
            if (FALSE /*((det->detector_def->dtype == MURDOCK_HUTT) land (E_detect (det))) lor
              ((det->detector_def->dtype == THRESHOLD) land (Te_detect (det))) */)

            {
                result = TRUE ;

                if ((pcc->new_onset) && (! have_detection)) {
                    have_detection = TRUE ;
                    memcpy(&(onset_save), &(det->onset), sizeof(tonset_mh)) ;
                }
            }
        } while (! (! (det->remaining))) ;

        if (have_detection) {
            result = TRUE ;
            (pcc->total_detections)++ ;
            onset_save.signal_onset_time.seed_fpt = onset_save.signal_onset_time.seed_fpt - q->def->delay ;
            (q->detections_session)++ ;
            logevent (q660, det, &(onset_save)) ;
        }
    }

    return result ;
}

void run_detector_chain (pq660 q660, plcq q, double rtime)
{
    pdet_packet det ;
    con_common *pcc ;

    det = q->det ;

    while (det) {
        pcc = det->cont ;
        pcc->startt = rtime ;

        if (pcc->detector_on) {
            pcc->detector_on = detect_record (q660, det, q) ;

            if (! (pcc->detector_on))
                pcc->first_detection = FALSE ;
        } else
            pcc->detector_on = detect_record (q660, det, q) ;

        pcc->detection_declared = ((pcc->detector_on) && (! (pcc->first_detection))) ;
        det = det->link ;
    }
}

void set_timetag (plcq q, double *tt)
{
    double correction, timetag_ ;
    double dprate, dpmark ;

    if (q->timetag < 1)
        timetag_ = q->backup_tag ;
    else
        timetag_ = q->timetag ;

    dprate = q->rate ;
    dpmark = q->com->time_mark_sample ;

    if (q->rate > 0)
        correction = - q->delay - (dpmark - 1) / dprate ;
    else
        correction = - q->delay - (dpmark - 1) * fabs(dprate) ;

    *tt = timetag_ + correction ;
}

void send_to_client (pq660 q660, plcq q, pcompressed_buffer_ring pbuf, U8 dest)
{
#define OCT_1_2016 23673600 /* first possible valid data */
#define MAX_DATE 0x7FFF0000 /* above this just has to be nonsense */

    q660->miniseed_call.context = q660 ;
    memcpy(&(q660->miniseed_call.station_name), &(q660->station_ident), sizeof(string9)) ;
    strcpy(q660->miniseed_call.location, q->slocation) ;
    strcpy(q660->miniseed_call.channel, q->sseedname) ;
    q660->miniseed_call.chan_number = q->lcq_num ;
    q660->miniseed_call.rate = q->rate ;
    q660->miniseed_call.timestamp = pbuf->hdr_buf.starting_time.seed_fpt ;

    if ((q660->miniseed_call.timestamp < OCT_1_2016) || (q660->miniseed_call.timestamp > MAX_DATE))
        return ; /* not possible */

    q660->miniseed_call.filter_bits = q->mini_filter ;
    q660->miniseed_call.packet_class = q->pack_class ;
    q660->miniseed_call.miniseed_action = MSA_512 ;
    q660->miniseed_call.data_size = LIB_REC_SIZE ;
    q660->miniseed_call.data_address = &(pbuf->rec) ;

    if ((dest & SCD_512) && (q->mini_filter) && (q660->par_create.call_minidata))
        q660->par_create.call_minidata (&(q660->miniseed_call)) ;

    if ((dest & SCD_ARCH) && (q->arc.amini_filter) && (q->pack_class != PKC_EVENT) &&
            (q->pack_class != PKC_CALIBRATE) && (q660->par_create.call_aminidata))

        archive_512_record (q660, q, pbuf) ;
}

void install_header (pq660 q660, plcq q, pcom_packet pcom)
{
    PU8 p ;
    double dprate, dpsamps ;
    seed_header *phdr ;

    phdr = &(pcom->ring->hdr_buf) ;
    p = (pointer)((PNTRINT)&(pcom->ring->rec) + (pcom->blockette_count + 1) * FRAME_SIZE + 8) ; /* ending sample address */
    storelongint (&(p), pcom->last_sample) ;
    phdr->samples_in_record = pcom->next_compressed_sample - 1 ;
    phdr->activity_flags = 0 ; /* Activity Flags set in finish/ringman */

    if (q->timetag < 1) {
        q->timetag = q->backup_tag ;
        q->timequal = q->backup_qual ;
    }

    if (q->timequal >= Q_OFF)
        phdr->io_flags = SIF_LOCKED ;
    else
        phdr->io_flags = 0 ;

    if (q->timequal < Q_LOW)
        phdr->data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
    else
        phdr->data_quality_flags = 0 ;

    /* THIS DOESN'T SEEM TO MAKE SENSE
      if (((q->raw_data_source and DCM) == DC_D32) land
         (q660->calerr_bitmap and (1 shl (q->raw_data_source and 7))))
        then
          phdr->data_quality_flags = phdr->data_quality_flags or SQF_AMPERR ;
    */
    if (pcom->charging) {
        phdr->data_quality_flags = phdr->data_quality_flags | SQF_CHARGING ;
        pcom->charging = FALSE ;
    }

    phdr->seed_record_type = 'D' ;
    phdr->continuation_record = ' ' ;
    phdr->sequence.seed_num = pcom->records_written + 1 ;
    memcpy(&(phdr->location_id), &(q->location), sizeof(tlocation)) ;
    memcpy(&(phdr->channel_id), &(q->seedname), sizeof(tseed_name)) ;
    memcpy(&(phdr->station_id_call_letters), &(q660->station), sizeof(tseed_stn)) ;
    memcpy(&(phdr->seednet), &(q660->network), sizeof(tseed_net)) ;
    phdr->sample_rate_factor = q->rate ;
    phdr->sample_rate_multiplier = 1 ;
    phdr->number_of_following_blockettes = pcom->blockette_count + 2 ;
    phdr->tenth_msec_correction = 0 ; /* this is a delta, which we don't */
    phdr->first_data_byte = (pcom->blockette_count + 1) * FRAME_SIZE ;
    phdr->first_blockette_byte = 48 ;
    set_timetag (q, &(phdr->starting_time.seed_fpt)) ;
    phdr->dob.blockette_type = 1000 ;
    phdr->dob.word_order = 1 ;
    phdr->dob.rec_length = RECORD_EXP ;
    phdr->dob.dob_reserved = 0 ;
    phdr->dob.next_blockette = 56 ;
    phdr->dob.encoding_format = 11 ;
    phdr->deb.blockette_type = 1001 ;

    if (pcom->blockette_count)
        phdr->deb.next_blockette = 64 ;
    else
        phdr->deb.next_blockette = 0 ;

    phdr->deb.deb_flags = q->gain_bits ;

    if (q->event_only)
        phdr->deb.deb_flags = phdr->deb.deb_flags | DEB_EVENT_ONLY ;

    phdr->deb.qual = q->timequal ;
    phdr->deb.usec99 = 0 ; /* for now */
    phdr->deb.frame_count = pcom->frame - 1 - pcom->blockette_count ;
    dprate = q->rate ;
    dpsamps = pcom->next_compressed_sample - 1 ;

    if (q->rate >= 0.999)
        q->backup_tag = q->timetag + dpsamps / dprate ;
    else
        q->backup_tag = q->timetag + dpsamps * fabs(dprate) ;

    q->backup_qual = q->timequal ;
    pcom->blockette_count = 0 ; /* no detection/cal blockettes */
    q->timequal = 0 ;
    q->timetag = 0 ; /* not set yet */
}

void ringman (pq660 q660, plcq q, BOOLEAN *now_on, BOOLEAN *was_on, U32 *next)
{
    pcompressed_buffer_ring lring ;
    BOOLEAN contig ;
    U8 sohsave ;
    PU8 p ;

    if ((*now_on) || (*was_on)) {
        lring = q->com->ring ;
        contig = *was_on ;

        do {
            if (! (contig))
                lring = lring->link ; /* the first will be the oldest */

            if ((lring->full) && (lring->hdr_buf.sequence.seed_num >= *next)) {
                sohsave = lring->hdr_buf.activity_flags ;

                if (! (*was_on)) /* if not time-contiguous */
                    lring->hdr_buf.activity_flags = lring->hdr_buf.activity_flags | (SAF_EVENT_IN_PROGRESS + SAF_BEGIN_EVENT) ;
                else
                    lring->hdr_buf.activity_flags = lring->hdr_buf.activity_flags | SAF_EVENT_IN_PROGRESS ;

                *was_on = TRUE ; /* next are contiguous until event done */
                *next = lring->hdr_buf.sequence.seed_num + 1 ;
                /* write the event blocks to the output files */
                p = (PU8)&(lring->rec) ;
                storeseedhdr (&(p), (pvoid)&(lring->hdr_buf), TRUE) ;

                if (q->scd_evt & SCD_ARCH) {
                    /* archival output only */
                    (q->records_generated_session)++ ;
                    q->last_record_generated = secsince () ;
                }

                send_to_client (q660, q, lring, q->scd_evt) ;
                lring->hdr_buf.activity_flags = sohsave ;
            }
        } while (! ((contig) || (lring == q->com->ring))) ;
    }

    if (! (*now_on)) {
        if ((q->scd_evt & SCD_ARCH) && (*was_on) && (q->arc.amini_filter))
            flush_archive (q660, q) ;

        *was_on = FALSE ; /* contiguity broken */
    }
}

void finish_record (pq660 q660, plcq q, pcom_packet pcom)
{
    U8 sohsave ;
    PU8 p ;
    seed_header *phdr ;

    install_header (q660, q, pcom) ;
    (pcom->records_written)++ ;
#ifdef NOTIMPLEMENTEDYET

    if ((! (q->data_written)) && (q->lcq_num != 0xFF))
        evaluate_detector_stack (q660, q) ;

#endif
    pcom->frame = 1 ;
    pcom->next_compressed_sample = 1 ; /* start of record */
    /* turn on EVENT_IN_PROGRESS in current record if any detectors are on */
    phdr = &(pcom->ring->hdr_buf) ;

    if (q->gen_on)
        phdr->activity_flags = phdr->activity_flags | SAF_EVENT_IN_PROGRESS ;

    if ((! q->gen_on) && (q->gen_last_on))
        phdr->activity_flags = phdr->activity_flags | SAF_END_EVENT ;

    if (q->cal_on)
        phdr->activity_flags = phdr->activity_flags | SAF_CAL_IN_PROGRESS ;

    /* write continuous data this comm links */
    if (q->scd_cont) {
        sohsave = phdr->activity_flags ;

        if ((q->gen_on) && (! q->gen_last_on))
            phdr->activity_flags = phdr->activity_flags | (SAF_EVENT_IN_PROGRESS | SAF_BEGIN_EVENT) ;

        p = (pvoid)&(pcom->ring->rec) ;
        storeseedhdr (&(p), (pvoid)phdr, TRUE) ;

        if (q->scd_cont & SCD_ARCH) {
            /* archival output only */
            (q->records_generated_session)++ ;
            q->last_record_generated = secsince () ;
        }

        send_to_client (q660, q, pcom->ring, q->scd_cont) ;
        phdr->activity_flags = sohsave ;
    }

    q->data_written = TRUE ;
    /* flag the current record in the pre-event buffer ring as being full */
    pcom->ring->full = TRUE ;

    if (q->scd_evt)
        ringman (q660, q, &(q->gen_on), &(q->gen_last_on), &(q->gen_next)) ;

    pcom->ring = pcom->ring->link ;
    memset(&(pcom->ring->rec), 0, LIB_REC_SIZE) ;
    memset(&(pcom->ring->hdr_buf), 0, sizeof(seed_header)) ;
    pcom->ring->full = FALSE ;
    q->gen_on = FALSE ; /* done with record, unlatch */
}

void flush_lcq (pq660 q660, plcq q, pcom_packet pcom)
{
    int used, loops ;
    PU8 p ;
    string15 s ;

    if (pcom->peek_total > 0) {
        loops = 0 ;

        do {
            used = compress_block (q660, q, pcom) ;
            pcom->next_compressed_sample = pcom->next_compressed_sample + used ;

            if (pcom->frame >= (pcom->maxframes + 1))
                finish_record (q660, q, pcom) ;

            (loops)++ ;
        } while (! ((pcom->peek_total <= 0) || (loops > 20))) ;

        if (loops > 20) {
            seed2string(q->location, q->seedname, s) ;
            libmsgadd (q660, LIBMSG_LBFAIL, s) ;
        }
    }

    if (pcom->next_compressed_sample > 1) {
        if ((pcom->block > 1) && (pcom->block < WORDS_PER_FRAME)) {
            /* complete partial frame */
            pcom->flag_word = pcom->flag_word << ((WORDS_PER_FRAME - pcom->block) << 1) ;
            pcom->frame_buffer[0] = pcom->flag_word ;
            p = (pointer)((PNTRINT)&(pcom->ring->rec) + pcom->frame * FRAME_SIZE) ;
            storeframe (&(p), &(pcom->frame_buffer)) ;
            memset(&(pcom->frame_buffer), 0, sizeof(compressed_frame)) ;
            (pcom->frame)++ ;
            pcom->block = 1 ;
            pcom->flag_word = 0 ;
        }

        finish_record (q660, q, pcom) ;
    }

    if ((q->arc.amini_filter) && (q->arc.total_frames > 1))
        flush_archive (q660, q) ;
}

void add_blockette (pq660 q660, plcq q, PU16 pw, double time)
{
    PU8 psrc, pdest, plink ;
    tcom_packet *pcom ;

    pcom = q->com ;

    if (pcom->frame >= pcom->maxframes) /* working on last one */
        flush_lcq (q660, q, pcom) ; /* need to start a new record */

    if (pcom->frame == 1)
        pcom->ring->hdr_buf.starting_time.seed_fpt = time ; /* make sure there is a time */

    if (pcom->frame > (pcom->blockette_count + 1)) {
        psrc = (pointer)((PNTRINT)&(pcom->ring->rec) + FRAME_SIZE * (pcom->blockette_count + 1)) ;
        pdest = (pointer)((PNTRINT)&(pcom->ring->rec) + FRAME_SIZE * (pcom->blockette_count + 2)) ;
        memmove (pdest, psrc, FRAME_SIZE * (pcom->frame - pcom->blockette_count - 1)) ;
    }

    pdest = (pointer)((PNTRINT)&(pcom->ring->rec) + FRAME_SIZE * (pcom->blockette_count + 1)) ;
    memcpy (pdest, pw, FRAME_SIZE) ;

    if (pcom->blockette_count) {
        /* extended link from previous blockette */
        plink = (pointer)((PNTRINT)&(pcom->ring->rec) + FRAME_SIZE * pcom->blockette_count + 2) ;
        storeword (&(plink), FRAME_SIZE * (pcom->blockette_count + 1)) ;
    }

    (pcom->blockette_count)++ ;
    (pcom->frame)++ ;

    if (pcom->frame >= (pcom->maxframes + 1))
        finish_record (q660, q, pcom) ;
}

void build_separate_record (pq660 q660, plcq q, PU16 pw,
                            double time, enum tpacket_class pclass)
{
    PU8 pdest, p ;
    seed_header *phdr ;

    memset (&(q660->detcal_buf), 0, sizeof(tcompressed_buffer_ring)) ;
    phdr = &(q660->detcal_buf.hdr_buf) ;
    phdr->starting_time.seed_fpt = time ;
    pdest = (pointer)((PNTRINT)&(q660->detcal_buf.rec) + FRAME_SIZE) ;
    memcpy (pdest, pw, FRAME_SIZE) ;
    phdr->activity_flags = SAF_BEGIN_EVENT ;
    phdr->seed_record_type = 'D' ;
    phdr->continuation_record = ' ' ;
    phdr->sequence.seed_num = 1 ;
    memcpy(&(phdr->location_id), &(q->location), sizeof(tlocation)) ;
    memcpy(&(phdr->channel_id), &(q->seedname), sizeof(tseed_name)) ;
    memcpy(&(phdr->station_id_call_letters), &(q660->station), sizeof(tseed_stn)) ;
    memcpy(&(phdr->seednet), &(q660->network), sizeof(tseed_net)) ;
    phdr->sample_rate_factor = q->rate ;
    phdr->sample_rate_multiplier = 1 ;
    phdr->number_of_following_blockettes = 2 ;
    phdr->tenth_msec_correction = 0 ; /* this is a delta, which we don't */
    phdr->first_blockette_byte = 48 ;
    phdr->dob.blockette_type = 1000 ;
    phdr->dob.word_order = 1 ;
    phdr->dob.rec_length = RECORD_EXP ;
    phdr->dob.dob_reserved = 0 ;
    phdr->dob.next_blockette = 64 ;
    phdr->dob.encoding_format = 11 ;
    p = (PU8)&(q660->detcal_buf.rec) ;
    storeseedhdr (&(p), (pvoid)phdr, FALSE) ;
    q->pack_class = pclass ; /* override normal data class */
    send_to_client (q660, q, &(q660->detcal_buf), SCD_BOTH) ;
    q->pack_class = PKC_DATA ; /* restore to normal */
}

void set_slip (pq660 q660, plcq q)
{
    pdownstream_packet down ;
    plcq p ;

    down = q->downstream_link ;

    while (down) {
        p = down->derived_q ;

        if (p->slip_modulus) {
            flush_lcq (q660, p, p->com) ;
            p->slipping = TRUE ;

            /* must restore fir filter to waiting to start state */
            if (p->fir) {
                p->fir->fcount = p->fir->flen - 1 ;
                p->fir->f = (pointer)((PNTRINT)p->fir->fbuf + (p->fir->flen - 1) * sizeof(tfloat)) ;
            }
        }

        set_slip (q660, p) ; /* recursive */
        down = down->link ;
    }
}

void process_lcq (pq660 q660, plcq q, int src_samp, tfloat dv)
{
    string95 s ;
    string31 s1, s2 ;
    volatile int samples ; /* gcc kept cloberring this without volatile */
    int i ;
    I32 dsamp ;        /* temp integer sample */
    tfloat sf ;
    PI32 p1 ;
    int used ;
    I32 int_time ; /* integer equivalent of sample time */
    piirfilter pi ;
    pdownstream_packet down ;
    pfloat p2 ;
    tfir_packet *pfir ;

    q->data_written = FALSE ;

    if (q->slipping) {
        /* for derived, see if reached modulus */
        int_time = lib_round((src_samp - 1) * q->input_sample_rate + q660->data_timetag) ;

        if ((int_time % q->slip_modulus) == (q->slip_modulus - 1))
            q->slipping = FALSE ;
        else
            return ;
    }

    if (q660->data_timetag < 1)
        q->com->charging = TRUE ;

    if (abs(src_samp) <= 1) {
        q->timemark_occurred = TRUE ; /* fist sample */

        if ((q->last_timetag > 1) && (fabs(q660->data_timetag - q->last_timetag - q->gap_offset) > q->gap_secs)) {
            if (q660->cur_verbosity & VERB_LOGEXTRA) {
                sprintf(s, "%s %s", seed2string(q->location, q->seedname, s1),
                        realtostr(q660->data_timetag - q->last_timetag - q->gap_offset, 6, s2)) ;
                libdatamsg (q660, LIBMSG_TIMEDISC, s) ;
            }

            flush_lcq (q660, q, q->com) ; /* gap in the data */
            set_slip (q660, q) ;
        }

        q->last_timetag = q660->data_timetag ;
        q->dtsequence = q660->dt_data_sequence ;
    }

    dsamp = 0 ;
    sf = 0.0 ;
    pi = q->stream_iir ;

    if (src_samp > 0) { /* only done due to decimation from a higher rate */
        samples = 1 ;
        pfir = q->fir ;

        if (pfir) {
            /*this only processes one sample at a time, it's probably <=1hz anyway*/
            *(pfir->f) = dv ;
            (pfir->f)++ ;
            (pfir->fcount)++ ;

            if (pfir->fcount < pfir->flen)
                return ;

            /*--------------------------------------------------------------------------------------
             This convolution may appear backwards, but for non-symetrical filters, the coefficients
             are defined in the reverse order, to match the reverse order of the input values
            ---------------------------------------------------------------------------------------*/
            sf = mac_and_shift (pfir) ;
            pfir->f = pfir->f - (pfir->fdec) ;
            pfir->fcount = pfir->fcount - (pfir->fdec) ; ;
            sf = sf * q->firfixing_gain ;
            q->processed_stream = sf ;
            dsamp = lib_round(sf) ;

            while (pi) {
                p2 = &(pi->out) ;
                *p2 = multi_section_filter (pi, sf) ;
                pi = pi->link ;
            }
        } else
            return ; /* nothing to */
    } else if (src_samp == 0) {
        /* 1hz or lower */
        samples = 1 ;

        while (pi) {
            p1 = (pointer)q->databuf ;
            p2 = &(pi->out) ;
            sf = *p1 ; /* convert to floating point */
            *p2 = multi_section_filter (pi, sf) ;
            pi = pi->link ;
        }
    } else {
        /* pre-compressed data */
        samples = decompress_blockette (q660, q) ;

        while (pi) {
            p1 = (pointer)q->databuf ;
            p2 = &(pi->out) ;

            for (i = 1 ; i <= samples ; i++) {
                sf = *p1 ; /* convert to floating point */
                *p2 = multi_section_filter (pi, sf) ;
                (p2)++ ;
                (p1)++ ;
            }

            pi = pi->link ;
        }
    }

    run_detector_chain (q660, q, q660->data_timetag) ;

    if (q->calstat) {
        q->cal_on = q->calstat ;
        q->calinc = 0 ;
    } else if ((q->cal_on) && (! q->calstat)) {
        (q->calinc)++ ;

        if (q->calinc > q->caldly)
            q->cal_on = q->calstat ;
    }

    /* Process misc. flags that must be processed a sample at a time */
    if (q->downstream_link) {
        p1 = (pointer)q->databuf ;
        p2 = NIL ;

        for (i = 1 ; i <= samples ; i++) {
            if (src_samp <= 0) {
                dsamp = *p1 ;
                sf = dsamp ;
            }

            down = q->downstream_link ;

            while (down) {
                process_lcq (q660, down->derived_q, i, sf) ;
                down = down->link ;
            }

            (p1)++ ;
        }
    }

    if (q->def->no_output)
        return ; /* not generating any data */

    if (q->timemark_occurred) {
        q->timemark_occurred = FALSE ;

        if ((q660->data_qual > q->timequal) || (q->timetag == 0)) {
            q->timetag = q660->data_timetag ;
            q->timequal = q660->data_qual ;
            q->com->time_mark_sample = q->com->peek_total + q->com->next_compressed_sample ;
        }
    }

    p1 = (pointer)q->databuf ;

    if (q660->par_create.call_secdata) {
        q660->onesec_call.total_size = sizeof(tonesec_call) - ((MAX_RATE - samples) * sizeof(I32)) ;
        q660->onesec_call.context = q660 ;
        memcpy(&(q660->onesec_call.station_name), &(q660->station_ident), sizeof(string9)) ;
        strcpy(q660->onesec_call.location, q->slocation) ;
        strcpy(q660->onesec_call.channel, q->sseedname) ;
        q660->onesec_call.timestamp = q660->data_timetag - q->delay ;
        q660->onesec_call.qual_perc = q660->data_qual ;
        q660->onesec_call.rate = q->rate ;
        q660->onesec_call.sample_count = samples ;
        q660->onesec_call.activity_flags = 0 ;

        if (q->gen_on)
            q660->onesec_call.activity_flags = q660->onesec_call.activity_flags | SAF_EVENT_IN_PROGRESS ;

        if (q->cal_on)
            q660->onesec_call.activity_flags = q660->onesec_call.activity_flags | SAF_CAL_IN_PROGRESS ;

        if (q->timequal >= Q_OFF)
            q660->onesec_call.io_flags = SIF_LOCKED ;
        else
            q660->onesec_call.io_flags = 0 ;

        if (q->timequal < Q_LOW)
            q660->onesec_call.data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
        else
            q660->onesec_call.data_quality_flags = 0 ;

        q660->onesec_call.src_gen = q->gen_src ;
        q660->onesec_call.src_subchan = q->sub_field ;

        if ((samples > 0) && (src_samp <= 0))
            memcpy (&(q660->onesec_call.samples), q->databuf, samples * sizeof(I32)) ;
    }

    if (src_samp >= 0) {
        /* data hasn't been pre-compressed */
        if (src_samp)
            q->com->peeks[q->com->next_in] = dsamp ;
        else
            q->com->peeks[q->com->next_in] = *p1 ;

        q->com->next_in = (q->com->next_in + 1) & (PEEKELEMS - 1) ;
        (q->com->peek_total)++ ;

        if (q660->par_create.call_secdata)
            q660->par_create.call_secdata (&(q660->onesec_call)) ;

        if (q->com->peek_total < MAXSAMPPERWORD)
            return ;

        used = compress_block (q660, q, q->com) ;
        q->com->next_compressed_sample = q->com->next_compressed_sample + used ;

        if (q->com->frame >= (q->com->maxframes + 1))
            finish_record (q660, q, q->com) ;
    } else {
        while (samples > 0) {
            used = build_blocks (q660, q, q->com) ;

            if (used == 0) {
                sprintf(s, "%s, %d samples left", seed2string(q->location, q->seedname, s1), samples) ;
                libdatamsg (q660, LIBMSG_RECOMP, s) ;
                samples = 0 ;
            } else {
                q->com->next_compressed_sample = q->com->next_compressed_sample + used ;

                if (q->com->frame >= (q->com->maxframes + 1))
                    finish_record (q660, q, q->com) ;
            }

            samples = samples - used ;
        }

        if (q660->par_create.call_secdata)
            q660->par_create.call_secdata (&(q660->onesec_call)) ;
    }
}

void flush_lcqs (pq660 q660)
{
    plcq q ;

    q = q660->lcqs ;

    while (q) {
        if ((q->def) && (! q->def->no_output))
            flush_lcq (q660, q, q->com) ;

        q = q->link ;
    }
}

void flush_dplcqs (pq660 q660)
{
    plcq q ;

    q = q660->dplcqs ;

    while (q) {
        switch (q->pack_class) {
        case PKC_DATA :
            flush_lcq (q660, q, q->com) ;
            break ;

        case PKC_TIMING :
            flush_timing (q660) ;
            break ;

        case PKC_MESSAGE :
            flush_messages (q660) ;
            break ;

        default :
            break ;
        }

        q = q->link ;
    }
}

void process_one (pq660 q660, plcq q, I32 data)
{
    string15 s ;

    if (q660->data_timetag < 1) {
        libdatamsg (q660, LIBMSG_DISCARD, seed2string(q->location, q->seedname, s)) ;
        return ; /* don't know what time it is yet */
    }

    if (seqspread (q660->dt_data_sequence, q->dtsequence) <= 0) { /* has this second of data already been processed? */
        if (abs(seqspread(q660->dt_data_sequence, q->dtsequence)) > MAXSPREAD)
            q->dtsequence = q660->dt_data_sequence ; /* if we're not within continuity distance, forget it */

        return ;
    }

    (*(q->databuf))[0] = data ; /* just one data point */
    process_lcq (q660, q, 0, 0.0) ;
}

/* On entry points to the start of the blockette and chan_offset represents the
  offset to add to the sub_field channel to get the mdispatch index */
void process_compressed (pq660 q660, PU8 p, int chan_offset,
                         U32 seq)
{
    PU8 psave ;
    tdatahdr datahdr ;
    int cnt, shift, size, offset, dsize, msize, used, src_cntr ;
    PU8 pd ;
    PU8 pm ;
    plcq q ;
    tprecomp *pcmp ;
    U8 dmask, smask, dest, src  ;
    string15 s ;

    psave = p ;
    loaddatahdr (&(p), &(datahdr)) ;

    if (((datahdr.chan >> 4) >= TOTAL_CHANNELS) || ((datahdr.chan & 0xF) >= FREQS))
        return ;

    if (datahdr.offset_flag & DHOF_OFF)
#ifdef LINUXDEBUGPRINT
    {
        static int P=0;
        fprintf(stderr,"P%d\n",P++);
        fflush(stderr);
#endif
        process_ll (q660, psave, chan_offset, seq) ; /* New lowish-latency handling */
#ifdef LINUXDEBUGPRINT
    }

#endif
    q = q660->mdispatch[(datahdr.chan >> 4) + chan_offset][datahdr.chan & 0xF] ;

    if (q == NIL)
        return ;

    pcmp = &(q->precomp) ;

    if (q660->data_timetag < 1) {
        libdatamsg (q660, LIBMSG_DISCARD, seed2string(q->location, q->seedname, s)) ;
        return ; /* don't know what time it is yet */
    }

    if (seqspread (q660->dt_data_sequence, q->dtsequence) <= 0) { /* has this second of data already been processed? */
        if (abs(seqspread(q660->dt_data_sequence, q->dtsequence)) > MAXSPREAD)
            q->dtsequence = q660->dt_data_sequence ; /* if we're not within continuity distance, forget it */

        return ;
    }

    if (seq != q->seg_seq) {
        /* New Second */
        pcmp->data_ptr = pcmp->data_buf ;
        pcmp->map_ptr = pcmp->map_buf ;
        pcmp->map_cntr = 0 ;
        memclr (pcmp->map_buf, pcmp->map_size) ;
        pcmp->blocks = 0 ;
    }

    q->seg_seq = seq ;
    pm = p ; /* Start of map */
    size = ((U16)datahdr.blk_size + 1) << 2 ; /* Total blockette size */
    offset = (datahdr.offset_flag & DHOF_MASK) << 2 ; /* Offset to start of data */
    dsize = size - offset ; /* Number of data bytes */
    pd = (PU8)((PNTRINT)psave + offset) ; /* Start of data blocks */

    if ((pcmp->map_cntr == 0) && (pcmp->map_ptr == pcmp->map_buf)) {
        /* First or only segment */
        if (datahdr.offset_flag & DHOF_PREV)
            pcmp->prev_sample = datahdr.prev_offset ;

        if ((datahdr.offset_flag & DHOF_MORE) == 0) {
            /* Only one segment */
            pcmp->pmap = p ;
            pcmp->pdata = pd ;
            pcmp->mapidx = 0 ;
            pcmp->blocks = dsize >> 2 ; /* Number of 32 bit blocks */
            process_lcq (q660, q, -1, 0.0) ;
            pcmp->map_cntr = 0 ;
            pcmp->map_ptr = pcmp->map_buf ;
            pcmp->data_ptr = pcmp->data_buf ;
            memclr (pcmp->map_buf, pcmp->map_size) ;
            return ;
        }
    }

    memcpy (pcmp->data_ptr, pd, dsize) ; /* move in data to merged buffer */
    pcmp->data_ptr = pcmp->data_ptr + (dsize) ;
    msize = dsize >> 1 ; /* Number of bits in map */
    src_cntr = 0 ; /* Start of new blockette map */
    pcmp->blocks = pcmp->blocks + (dsize >> 2) ; /* keep track of 32 bit blocks */

    if ((pcmp->map_cntr == 0) && (src_cntr == 0)) {
        /* expedited map copy */
        cnt = (msize + 7) >> 3 ; /* number of bytes in source, to byte size */
        memcpy (pcmp->map_ptr, pm, cnt) ;
        pcmp->map_cntr = msize ;

        while (pcmp->map_cntr >= 8) {
            /* Need to reduce to within one byte */
            pcmp->map_cntr = pcmp->map_cntr - (8) ;
            (pcmp->map_ptr)++ ;
        }
    } else
        while (msize > 0) {
            dmask = DMASKS[pcmp->map_cntr >> 1] ; /* the valid bits within the current dest. */
            dest = *(pcmp->map_ptr) & dmask ;
            smask = SMASKS[src_cntr >> 1] ; /* same for source */
            src = *pm & smask ;
            shift = pcmp->map_cntr - src_cntr ; /* result may be negative! */

            if (shift > 0)
                src = src >> shift ;
            else if (shift < 0)
                src = src << (-shift) ;

            dest = dest | src ; /* merge existing bits with new ones */
            *(pcmp->map_ptr) = dest ; /* and update */
            used = pcmp->map_cntr ;

            if (src_cntr > used)
                used = src_cntr ; /* find higher of the two */

            used = 8 - used ; /* this is actual number of bits used */

            if (used > msize)
                used = msize ; /* but no larger than we have in src */

            src_cntr = src_cntr + (used) ;
            pcmp->map_cntr = pcmp->map_cntr + (used) ;
            msize = msize - (used) ;

            while (pcmp->map_cntr >= 8) {
                /* Need to reduce to within one word */
                pcmp->map_cntr = pcmp->map_cntr - (8) ;
                (pcmp->map_ptr)++ ;
            }

            while (src_cntr >= 8) {
                /* Need to reduce to within one word */
                src_cntr = src_cntr - (8) ;
                (pm)++ ;
            }
        }

    if ((datahdr.offset_flag & DHOF_MORE) == 0) {
        /* last segment */
        pcmp->pmap = (PU8)pcmp->map_buf ;
        pcmp->pdata = (PU8)pcmp->data_buf ;
        pcmp->mapidx = 0 ;
        process_lcq (q660, q, -1, 0.0) ;
        pcmp->map_cntr = 0 ;
        pcmp->map_ptr = pcmp->map_buf ;
        pcmp->data_ptr = pcmp->data_buf ;
        memclr (pcmp->map_buf, pcmp->map_size) ;
    }
}

