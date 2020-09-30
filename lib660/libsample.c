/*   Lib660 Time Series data routines
     Copyright 2017 Certified Software Corporation

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
*/
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

typedef byte tmasks[4] ;
const tmasks DMASKS = {0x0, 0xC0, 0xF0, 0xFC} ;
const tmasks SMASKS = {0xFF, 0x3F, 0x0F, 0x03} ;

longint sex (longint l)
begin

  if (l and 0x8000)
    then
      l = l or (longint)0xFFFF0000 ;
  return l ;
end

longint seqspread (longword new_, longword last)
begin
  longint d ;

  d = new_ - last ; /* returns a signed value, which takes care of 32-bit wrapping */
  return d ;
end

static boolean detect_record (pq660 q660, pdet_packet det, plcq q)
begin
  boolean have_detection, result ;
  tonset_mh onset_save ;
  con_common *pcc ;

  pcc = det->cont ;
  have_detection = FALSE ;
  result = FALSE ;
  if (pcc->detector_enabled)
    then
      begin
        repeat
          if (FALSE /*((det->detector_def->dtype == MURDOCK_HUTT) land (E_detect (det))) lor
              ((det->detector_def->dtype == THRESHOLD) land (Te_detect (det))) */)
            then
              begin
                result = TRUE ;
                if ((pcc->new_onset) land (lnot have_detection))
                  then
                    begin
                      have_detection = TRUE ;
                      memcpy(addr(onset_save), addr(det->onset), sizeof(tonset_mh)) ;
                    end
              end
        until (lnot (det->remaining))) ;
        if (have_detection)
          then
            begin
              result = TRUE ;
              inc(pcc->total_detections) ;
              onset_save.signal_onset_time.seed_fpt = onset_save.signal_onset_time.seed_fpt - q->def->delay ;
              inc(q->detections_session) ;
              logevent (q660, det, addr(onset_save)) ;
            end
      end
  return result ;
end

void run_detector_chain (pq660 q660, plcq q, double rtime)
begin
  pdet_packet det ;
  con_common *pcc ;

  det = q->det ;
  while (det)
    begin
      pcc = det->cont ;
      pcc->startt = rtime ;
      if (pcc->detector_on)
        then
          begin
            pcc->detector_on = detect_record (q660, det, q) ;
            if (lnot (pcc->detector_on))
              then
                pcc->first_detection = FALSE ;
          end
        else
          pcc->detector_on = detect_record (q660, det, q) ;
      pcc->detection_declared = ((pcc->detector_on) land (lnot (pcc->first_detection))) ;
      det = det->link ;
    end
end

void set_timetag (plcq q, double *tt)
begin
  double correction, timetag_ ;
  double dprate, dpmark ;

  if (q->timetag < 1)
    then
      timetag_ = q->backup_tag ;
    else
      timetag_ = q->timetag ;
  dprate = q->rate ;
  dpmark = q->com->time_mark_sample ;
  if (q->rate > 0)
    then
      correction = - q->delay - (dpmark - 1) / dprate ;
    else
      correction = - q->delay - (dpmark - 1) * fabs(dprate) ;
  *tt = timetag_ + correction ;
end

void send_to_client (pq660 q660, plcq q, pcompressed_buffer_ring pbuf, byte dest)
begin
#define OCT_1_2016 23673600 /* first possible valid data */
#define MAX_DATE 0x7FFF0000 /* above this just has to be nonsense */

  q660->miniseed_call.context = q660 ;
  memcpy(addr(q660->miniseed_call.station_name), addr(q660->station_ident), sizeof(string9)) ;
  strcpy(q660->miniseed_call.location, q->slocation) ;
  strcpy(q660->miniseed_call.channel, q->sseedname) ;
  q660->miniseed_call.chan_number = q->lcq_num ;
  q660->miniseed_call.rate = q->rate ;
  q660->miniseed_call.timestamp = pbuf->hdr_buf.starting_time.seed_fpt ;
  if ((q660->miniseed_call.timestamp < OCT_1_2016) lor (q660->miniseed_call.timestamp > MAX_DATE))
    then
      return ; /* not possible */
  q660->miniseed_call.filter_bits = q->mini_filter ;
  q660->miniseed_call.packet_class = q->pack_class ;
  q660->miniseed_call.miniseed_action = MSA_512 ;
  q660->miniseed_call.data_size = LIB_REC_SIZE ;
  q660->miniseed_call.data_address = addr(pbuf->rec) ;
  if ((dest and SCD_512) land (q->mini_filter) land (q660->par_create.call_minidata))
    then
      q660->par_create.call_minidata (addr(q660->miniseed_call)) ;
  if ((dest and SCD_ARCH) land (q->arc.amini_filter) land (q->pack_class != PKC_EVENT) land
      (q->pack_class != PKC_CALIBRATE) land (q660->par_create.call_aminidata))
    then
      archive_512_record (q660, q, pbuf) ;
end

void install_header (pq660 q660, plcq q, pcom_packet pcom)
begin
  pbyte p ;
  double dprate, dpsamps ;
  seed_header *phdr ;

  phdr = addr(pcom->ring->hdr_buf) ;
  p = (pointer)((pntrint)addr(pcom->ring->rec) + (pcom->blockette_count + 1) * FRAME_SIZE + 8) ; /* ending sample address */
  storelongint (addr(p), pcom->last_sample) ;
  phdr->samples_in_record = pcom->next_compressed_sample - 1 ;
  phdr->activity_flags = 0 ; /* Activity Flags set in finish/ringman */
  if (q->timetag < 1)
    then
      begin
        q->timetag = q->backup_tag ;
        q->timequal = q->backup_qual ;
      end
  if (q->timequal >= Q_OFF)
   then
     phdr->io_flags = SIF_LOCKED ;
   else
     phdr->io_flags = 0 ;
  if (q->timequal < Q_LOW)
    then
      phdr->data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
    else
      phdr->data_quality_flags = 0 ;
/* THIS DOESN'T SEEM TO MAKE SENSE
  if (((q->raw_data_source and DCM) == DC_D32) land
     (q660->calerr_bitmap and (1 shl (q->raw_data_source and 7))))
    then
      phdr->data_quality_flags = phdr->data_quality_flags or SQF_AMPERR ;
*/
  if (pcom->charging)
    then
      begin
        phdr->data_quality_flags = phdr->data_quality_flags or SQF_CHARGING ;
        pcom->charging = FALSE ;
      end
  phdr->seed_record_type = 'D' ;
  phdr->continuation_record = ' ' ;
  phdr->sequence.seed_num = pcom->records_written + 1 ;
  memcpy(addr(phdr->location_id), addr(q->location), sizeof(tlocation)) ;
  memcpy(addr(phdr->channel_id), addr(q->seedname), sizeof(tseed_name)) ;
  memcpy(addr(phdr->station_id_call_letters), addr(q660->station), sizeof(tseed_stn)) ;
  memcpy(addr(phdr->seednet), addr(q660->network), sizeof(tseed_net)) ;
  phdr->sample_rate_factor = q->rate ;
  phdr->sample_rate_multiplier = 1 ;
  phdr->number_of_following_blockettes = pcom->blockette_count + 2 ;
  phdr->tenth_msec_correction = 0 ; /* this is a delta, which we don't */
  phdr->first_data_byte = (pcom->blockette_count + 1) * FRAME_SIZE ;
  phdr->first_blockette_byte = 48 ;
  set_timetag (q, addr(phdr->starting_time.seed_fpt)) ;
  phdr->dob.blockette_type = 1000 ;
  phdr->dob.word_order = 1 ;
  phdr->dob.rec_length = RECORD_EXP ;
  phdr->dob.dob_reserved = 0 ;
  phdr->dob.next_blockette = 56 ;
  phdr->dob.encoding_format = 11 ;
  phdr->deb.blockette_type = 1001 ;
  if (pcom->blockette_count)
    then
      phdr->deb.next_blockette = 64 ;
    else
      phdr->deb.next_blockette = 0 ;
  phdr->deb.deb_flags = q->gain_bits ;
  if (q->event_only)
    then
      phdr->deb.deb_flags = phdr->deb.deb_flags or DEB_EVENT_ONLY ;
  phdr->deb.qual = q->timequal ;
  phdr->deb.usec99 = 0 ; /* for now */
  phdr->deb.frame_count = pcom->frame - 1 - pcom->blockette_count ;
  dprate = q->rate ;
  dpsamps = pcom->next_compressed_sample - 1 ;
  if (q->rate >= 0.999)
    then
      q->backup_tag = q->timetag + dpsamps / dprate ;
    else
      q->backup_tag = q->timetag + dpsamps * fabs(dprate) ;
  q->backup_qual = q->timequal ;
  pcom->blockette_count = 0 ; /* no detection/cal blockettes */
  q->timequal = 0 ;
  q->timetag = 0 ; /* not set yet */
end

void ringman (pq660 q660, plcq q, boolean *now_on, boolean *was_on, longword *next)
begin
  pcompressed_buffer_ring lring ;
  boolean contig ;
  byte sohsave ;
  pbyte p ;

  if ((*now_on) lor (*was_on))
    then
      begin
        lring = q->com->ring ;
        contig = *was_on ;
        repeat
          if (lnot (contig))
            then
              lring = lring->link ; /* the first will be the oldest */
          if ((lring->full) land (lring->hdr_buf.sequence.seed_num >= *next))
            then
              begin
                sohsave = lring->hdr_buf.activity_flags ;
                if (lnot (*was_on)) /* if not time-contiguous */
                  then
                    lring->hdr_buf.activity_flags = lring->hdr_buf.activity_flags or (SAF_EVENT_IN_PROGRESS + SAF_BEGIN_EVENT) ;
                  else
                    lring->hdr_buf.activity_flags = lring->hdr_buf.activity_flags or SAF_EVENT_IN_PROGRESS ;
                *was_on = TRUE ; /* next are contiguous until event done */
                *next = lring->hdr_buf.sequence.seed_num + 1 ;
/* write the event blocks to the output files */
                p = (pbyte)addr(lring->rec) ;
                storeseedhdr (addr(p), (pvoid)addr(lring->hdr_buf), TRUE) ;
                if (q->scd_evt and SCD_ARCH)
                  then
                    begin /* archival output only */
                      inc(q->records_generated_session) ;
                      q->last_record_generated = secsince () ;
                    end
                send_to_client (q660, q, lring, q->scd_evt) ;
                lring->hdr_buf.activity_flags = sohsave ;
              end
        until ((contig) lor (lring == q->com->ring))) ;
      end
  if (lnot (*now_on))
    then
      begin
        if ((q->scd_evt and SCD_ARCH) land (*was_on) land (q->arc.amini_filter))
          then
            flush_archive (q660, q) ;
        *was_on = FALSE ; /* contiguity broken */
      end
end

void finish_record (pq660 q660, plcq q, pcom_packet pcom)
begin
  byte sohsave ;
  pbyte p ;
  seed_header *phdr ;

  install_header (q660, q, pcom) ;
  inc(pcom->records_written) ;
#ifdef NOTIMPLEMENTEDYET
  if ((lnot (q->data_written)) land (q->lcq_num != 0xFF))
    then
      evaluate_detector_stack (q660, q) ;
#endif
  pcom->frame = 1 ;
  pcom->next_compressed_sample = 1 ; /* start of record */
/* turn on EVENT_IN_PROGRESS in current record if any detectors are on */
  phdr = addr(pcom->ring->hdr_buf) ;
  if (q->gen_on)
    then
      phdr->activity_flags = phdr->activity_flags or SAF_EVENT_IN_PROGRESS ;
  if ((lnot q->gen_on) land (q->gen_last_on))
    then
      phdr->activity_flags = phdr->activity_flags or SAF_END_EVENT ;
  if (q->cal_on)
    then
      phdr->activity_flags = phdr->activity_flags or SAF_CAL_IN_PROGRESS ;
/* write continuous data this comm links */
  if (q->scd_cont)
    then
      begin
        sohsave = phdr->activity_flags ;
        if ((q->gen_on) land (lnot q->gen_last_on))
          then
            phdr->activity_flags = phdr->activity_flags or (SAF_EVENT_IN_PROGRESS or SAF_BEGIN_EVENT) ;
        p = (pvoid)addr(pcom->ring->rec) ;
        storeseedhdr (addr(p), (pvoid)phdr, TRUE) ;
        if (q->scd_cont and SCD_ARCH)
          then
            begin /* archival output only */
              inc(q->records_generated_session) ;
              q->last_record_generated = secsince () ;
            end
        send_to_client (q660, q, pcom->ring, q->scd_cont) ;
        phdr->activity_flags = sohsave ;
      end
  q->data_written = TRUE ;
/* flag the current record in the pre-event buffer ring as being full */
  pcom->ring->full = TRUE ;
  if (q->scd_evt)
    then
      ringman (q660, q, addr(q->gen_on), addr(q->gen_last_on), addr(q->gen_next)) ;
  pcom->ring = pcom->ring->link ;
  memset(addr(pcom->ring->rec), 0, LIB_REC_SIZE) ;
  memset(addr(pcom->ring->hdr_buf), 0, sizeof(seed_header)) ;
  pcom->ring->full = FALSE ;
  q->gen_on = FALSE ; /* done with record, unlatch */
end

void flush_lcq (pq660 q660, plcq q, pcom_packet pcom)
begin
  integer used, loops ;
  pbyte p ;
  string15 s ;

  if (pcom->peek_total > 0)
    then
      begin
        loops = 0 ;
        repeat
          used = compress_block (q660, q, pcom) ;
          pcom->next_compressed_sample = pcom->next_compressed_sample + used ;
          if (pcom->frame >= (pcom->maxframes + 1))
            then
              finish_record (q660, q, pcom) ;
          inc(loops) ;
        until ((pcom->peek_total <= 0) lor (loops > 20))) ;
        if (loops > 20)
          then
            begin
              seed2string(q->location, q->seedname, s) ;
              libmsgadd (q660, LIBMSG_LBFAIL, s) ;
            end
      end
  if (pcom->next_compressed_sample > 1)
    then
      begin
        if ((pcom->block > 1) land (pcom->block < WORDS_PER_FRAME))
          then
            begin /* complete partial frame */
              pcom->flag_word = pcom->flag_word shl ((WORDS_PER_FRAME - pcom->block) shl 1) ;
              pcom->frame_buffer[0] = pcom->flag_word ;
              p = (pointer)((pntrint)addr(pcom->ring->rec) + pcom->frame * FRAME_SIZE) ;
              storeframe (addr(p), addr(pcom->frame_buffer)) ;
              memset(addr(pcom->frame_buffer), 0, sizeof(compressed_frame)) ;
              inc(pcom->frame) ;
              pcom->block = 1 ;
              pcom->flag_word = 0 ;
            end
        finish_record (q660, q, pcom) ;
      end
  if ((q->arc.amini_filter) land (q->arc.total_frames > 1))
    then
      flush_archive (q660, q) ;
end

void add_blockette (pq660 q660, plcq q, pword pw, double time)
begin
  pbyte psrc, pdest, plink ;
  tcom_packet *pcom ;

  pcom = q->com ;
  if (pcom->frame >= pcom->maxframes) /* working on last one */
    then
      flush_lcq (q660, q, pcom) ; /* need to start a new record */
  if (pcom->frame == 1)
    then
      pcom->ring->hdr_buf.starting_time.seed_fpt = time ; /* make sure there is a time */
  if (pcom->frame > (pcom->blockette_count + 1))
    then
      begin
        psrc = (pointer)((pntrint)addr(pcom->ring->rec) + FRAME_SIZE * (pcom->blockette_count + 1)) ;
        pdest = (pointer)((pntrint)addr(pcom->ring->rec) + FRAME_SIZE * (pcom->blockette_count + 2)) ;
        memmove (pdest, psrc, FRAME_SIZE * (pcom->frame - pcom->blockette_count - 1)) ;
      end
  pdest = (pointer)((pntrint)addr(pcom->ring->rec) + FRAME_SIZE * (pcom->blockette_count + 1)) ;
  memcpy (pdest, pw, FRAME_SIZE) ;
  if (pcom->blockette_count)
    then
      begin /* extended link from previous blockette */
        plink = (pointer)((pntrint)addr(pcom->ring->rec) + FRAME_SIZE * pcom->blockette_count + 2) ;
        storeword (addr(plink), FRAME_SIZE * (pcom->blockette_count + 1)) ;
      end
  inc(pcom->blockette_count) ;
  inc(pcom->frame) ;
  if (pcom->frame >= (pcom->maxframes + 1))
    then
      finish_record (q660, q, pcom) ;
end

void build_separate_record (pq660 q660, plcq q, pword pw,
                            double time, enum tpacket_class pclass)
begin
  pbyte pdest, p ;
  seed_header *phdr ;

  memset (addr(q660->detcal_buf), 0, sizeof(tcompressed_buffer_ring)) ;
  phdr = addr(q660->detcal_buf.hdr_buf) ;
  phdr->starting_time.seed_fpt = time ;
  pdest = (pointer)((pntrint)addr(q660->detcal_buf.rec) + FRAME_SIZE) ;
  memcpy (pdest, pw, FRAME_SIZE) ;
  phdr->activity_flags = SAF_BEGIN_EVENT ;
  phdr->seed_record_type = 'D' ;
  phdr->continuation_record = ' ' ;
  phdr->sequence.seed_num = 1 ;
  memcpy(addr(phdr->location_id), addr(q->location), sizeof(tlocation)) ;
  memcpy(addr(phdr->channel_id), addr(q->seedname), sizeof(tseed_name)) ;
  memcpy(addr(phdr->station_id_call_letters), addr(q660->station), sizeof(tseed_stn)) ;
  memcpy(addr(phdr->seednet), addr(q660->network), sizeof(tseed_net)) ;
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
  p = (pbyte)addr(q660->detcal_buf.rec) ;
  storeseedhdr (addr(p), (pvoid)phdr, FALSE) ;
  q->pack_class = pclass ; /* override normal data class */
  send_to_client (q660, q, addr(q660->detcal_buf), SCD_BOTH) ;
  q->pack_class = PKC_DATA ; /* restore to normal */
end

void set_slip (pq660 q660, plcq q)
begin
  pdownstream_packet down ;
  plcq p ;

  down = q->downstream_link ;
  while (down)
    begin
      p = down->derived_q ;
      if (p->slip_modulus)
        then
          begin
            flush_lcq (q660, p, p->com) ;
            p->slipping = TRUE ;
            /* must restore fir filter to waiting to start state */
            if (p->fir)
              then
                begin
                  p->fir->fcount = p->fir->flen - 1 ;
                  p->fir->f = (pointer)((pntrint)p->fir->fbuf + (p->fir->flen - 1) * sizeof(tfloat)) ;
                end
          end
      set_slip (q660, p) ; /* recursive */
      down = down->link ;
    end
end

void process_lcq (pq660 q660, plcq q, integer src_samp, tfloat dv)
begin
  string95 s ;
  string31 s1, s2 ;
  volatile integer samples ; /* gcc kept cloberring this without volatile */
  integer i ;
  longint dsamp ;        /* temp integer sample */
  tfloat sf ;
  plong p1 ;
  integer used ;
  longint int_time ; /* integer equivalent of sample time */
  piirfilter pi ;
  pdownstream_packet down ;
  pfloat p2 ;
  tfir_packet *pfir ;

  q->data_written = FALSE ;
  if (q->slipping)
    then
      begin /* for derived, see if reached modulus */
        int_time = lib_round((src_samp - 1) * q->input_sample_rate + q660->data_timetag) ;
        if ((int_time mod q->slip_modulus) == (q->slip_modulus - 1))
          then
            q->slipping = FALSE ;
          else
            return ;
      end
  if (q660->data_timetag < 1)
    then
      q->com->charging = TRUE ;
  if (abs(src_samp) <= 1)
    then
      begin
        q->timemark_occurred = TRUE ; /* fist sample */
        if ((q->last_timetag > 1) land (fabs(q660->data_timetag - q->last_timetag - q->gap_offset) > q->gap_secs))
          then
            begin
              if (q660->cur_verbosity and VERB_LOGEXTRA)
                then
                  begin
                    sprintf(s, "%s %s", seed2string(q->location, q->seedname, s1),
                            realtostr(q660->data_timetag - q->last_timetag - q->gap_offset, 6, s2)) ;
                    libdatamsg (q660, LIBMSG_TIMEDISC, s) ;
                  end
              flush_lcq (q660, q, q->com) ; /* gap in the data */
              set_slip (q660, q) ;
            end
        q->last_timetag = q660->data_timetag ;
        q->dtsequence = q660->dt_data_sequence ;
      end
  dsamp = 0 ;
  sf = 0.0 ;
  pi = q->stream_iir ;
  if (src_samp > 0) /* only done due to decimation from a higher rate */
    then
      begin
        samples = 1 ;
        pfir = q->fir ;
        if (pfir)
          then
            begin /*this only processes one sample at a time, it's probably <=1hz anyway*/
              *(pfir->f) = dv ;
              inc(pfir->f) ;
              inc(pfir->fcount) ;
              if (pfir->fcount < pfir->flen)
                then
                  return ;
/*--------------------------------------------------------------------------------------
 This convolution may appear backwards, but for non-symetrical filters, the coefficients
 are defined in the reverse order, to match the reverse order of the input values
---------------------------------------------------------------------------------------*/
              sf = mac_and_shift (pfir) ;
              decn(pfir->f, pfir->fdec) ;
              decn(pfir->fcount, pfir->fdec) ; ;
              sf = sf * q->firfixing_gain ;
              q->processed_stream = sf ;
              dsamp = lib_round(sf) ;
              while (pi)
                begin
                  p2 = addr(pi->out) ;
                  *p2 = multi_section_filter (pi, sf) ;
                  pi = pi->link ;
                end
            end
          else
            return ; /* nothing to */
      end
  else if (src_samp == 0)
    then
      begin /* 1hz or lower */
        samples = 1 ;
        while (pi)
          begin
            p1 = (pointer)q->databuf ;
            p2 = addr(pi->out) ;
            sf = *p1 ; /* convert to floating point */
            *p2 = multi_section_filter (pi, sf) ;
            pi = pi->link ;
          end
      end
    else
      begin /* pre-compressed data */
        samples = decompress_blockette (q660, q) ;
        while (pi)
          begin
            p1 = (pointer)q->databuf ;
            p2 = addr(pi->out) ;
            for (i = 1 ; i <= samples ; i++)
              begin
                sf = *p1 ; /* convert to floating point */
                *p2 = multi_section_filter (pi, sf) ;
                inc(p2) ;
                inc(p1) ;
              end
            pi = pi->link ;
          end
      end
  run_detector_chain (q660, q, q660->data_timetag) ;
  if (q->calstat)
    then
      begin
        q->cal_on = q->calstat ;
        q->calinc = 0 ;
      end
  else if ((q->cal_on) land (lnot q->calstat))
    then
      begin
        inc(q->calinc) ;
        if (q->calinc > q->caldly)
          then
            q->cal_on = q->calstat ;
      end
/* Process misc. flags that must be processed a sample at a time */
  if (q->downstream_link)
    then
      begin
        p1 = (pointer)q->databuf ;
        p2 = NIL ;
        for (i = 1 ; i <= samples ; i++)
          begin
            if (src_samp <= 0)
              then
                begin
                  dsamp = *p1 ;
                  sf = dsamp ;
                end
            down = q->downstream_link ;
            while (down)
              begin
                process_lcq (q660, down->derived_q, i, sf) ;
                down = down->link ;
              end
            inc(p1) ;
          end
      end
  if (q->def->no_output)
    then
      return ; /* not generating any data */
  if (q->timemark_occurred)
    then
      begin
        q->timemark_occurred = FALSE ;
        if ((q660->data_qual > q->timequal) lor (q->timetag == 0))
          then
            begin
              q->timetag = q660->data_timetag ;
              q->timequal = q660->data_qual ;
              q->com->time_mark_sample = q->com->peek_total + q->com->next_compressed_sample ;
            end
      end
  p1 = (pointer)q->databuf ;
  if (q660->par_create.call_secdata)
    then
      begin
        q660->onesec_call.total_size = sizeof(tonesec_call) - ((MAX_RATE - samples) * sizeof(longint)) ;
        q660->onesec_call.context = q660 ;
        memcpy(addr(q660->onesec_call.station_name), addr(q660->station_ident), sizeof(string9)) ;
        strcpy(q660->onesec_call.location, q->slocation) ;
        strcpy(q660->onesec_call.channel, q->sseedname) ;
        q660->onesec_call.timestamp = q660->data_timetag - q->delay ;
        q660->onesec_call.qual_perc = q660->data_qual ;
        q660->onesec_call.rate = q->rate ;
        q660->onesec_call.sample_count = samples ;
        q660->onesec_call.activity_flags = 0 ;
        if (q->gen_on)
          then
            q660->onesec_call.activity_flags = q660->onesec_call.activity_flags or SAF_EVENT_IN_PROGRESS ;
        if (q->cal_on)
          then
            q660->onesec_call.activity_flags = q660->onesec_call.activity_flags or SAF_CAL_IN_PROGRESS ;
        if (q->timequal >= Q_OFF)
         then
           q660->onesec_call.io_flags = SIF_LOCKED ;
         else
           q660->onesec_call.io_flags = 0 ;
        if (q->timequal < Q_LOW)
          then
            q660->onesec_call.data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
          else
            q660->onesec_call.data_quality_flags = 0 ;
        q660->onesec_call.src_gen = q->gen_src ;
        q660->onesec_call.src_subchan = q->sub_field ;
        if ((samples > 0) land (src_samp <= 0))
          then
            memcpy (addr(q660->onesec_call.samples), q->databuf, samples * sizeof(longint)) ;
      end
  if (src_samp >= 0)
    then
      begin /* data hasn't been pre-compressed */
        if (src_samp)
          then
            q->com->peeks[q->com->next_in] = dsamp ;
          else
            q->com->peeks[q->com->next_in] = *p1 ;
        q->com->next_in = (q->com->next_in + 1) and (PEEKELEMS - 1) ;
        inc(q->com->peek_total) ;
        if (q660->par_create.call_secdata)
          then
            q660->par_create.call_secdata (addr(q660->onesec_call)) ;
        if (q->com->peek_total < MAXSAMPPERWORD)
          then
            return ;
        used = compress_block (q660, q, q->com) ;
        q->com->next_compressed_sample = q->com->next_compressed_sample + used ;
        if (q->com->frame >= (q->com->maxframes + 1))
          then
            finish_record (q660, q, q->com) ;
      end
    else
      begin
        while (samples > 0)
          begin
            used = build_blocks (q660, q, q->com) ;
            if (used == 0)
              then
                begin
                  sprintf(s, "%s, %d samples left", seed2string(q->location, q->seedname, s1), samples) ;
                  libdatamsg (q660, LIBMSG_RECOMP, s) ;
                  samples = 0 ;
                end
              else
                begin
                  q->com->next_compressed_sample = q->com->next_compressed_sample + used ;
                  if (q->com->frame >= (q->com->maxframes + 1))
                    then
                      finish_record (q660, q, q->com) ;
                end
            samples = samples - used ;
          end
        if (q660->par_create.call_secdata)
          then
            q660->par_create.call_secdata (addr(q660->onesec_call)) ;
      end
end

void flush_lcqs (pq660 q660)
begin
  plcq q ;

  q = q660->lcqs ;
  while (q)
    begin
      if ((q->def) land (lnot q->def->no_output))
        then
          flush_lcq (q660, q, q->com) ;
      q = q->link ;
    end
end

void flush_dplcqs (pq660 q660)
begin
  plcq q ;

  q = q660->dplcqs ;
  while (q)
    begin
      switch (q->pack_class) begin
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
      end
      q = q->link ;
    end
end

void process_one (pq660 q660, plcq q, longint data)
begin
  string15 s ;

  if (q660->data_timetag < 1)
    then
      begin
        libdatamsg (q660, LIBMSG_DISCARD, seed2string(q->location, q->seedname, s)) ;
        return ; /* don't know what time it is yet */
      end
  if (seqspread (q660->dt_data_sequence, q->dtsequence) <= 0) /* has this second of data already been processed? */
    then
      begin
        if (abs(seqspread(q660->dt_data_sequence, q->dtsequence)) > MAXSPREAD)
          then
            q->dtsequence = q660->dt_data_sequence ; /* if we're not within continuity distance, forget it */
        return ;
      end
  (*(q->databuf))[0] = data ; /* just one data point */
  process_lcq (q660, q, 0, 0.0) ;
end

/* On entry points to the start of the blockette and chan_offset represents the
  offset to add to the sub_field channel to get the mdispatch index */
void process_compressed (pq660 q660, pbyte p, integer chan_offset,
            longword seq)
begin
  pbyte psave ;
  tdatahdr datahdr ;
  integer cnt, shift, size, offset, dsize, msize, used, src_cntr ;
  pbyte pd ;
  pbyte pm ;
  plcq q ;
  tprecomp *pcmp ;
  byte dmask, smask, dest, src  ;
  string15 s ;

  psave = p ;
  loaddatahdr (addr(p), addr(datahdr)) ;
  if (((datahdr.chan shr 4) >= TOTAL_CHANNELS) lor ((datahdr.chan and 0xF) >= FREQS))
    then
      return ;

  /* Per Joe, disable these 3 lines:
  if (datahdr.offset_flag and DHOF_OFF)
    then
      process_ll (q660, psave, chan_offset, seq) ; /* New lowish-latency handling */
  
  q = q660->mdispatch[(datahdr.chan shr 4) + chan_offset][datahdr.chan and 0xF] ;
  if (q == NIL)
    then
      return ;
  pcmp = addr(q->precomp) ;
  if (q660->data_timetag < 1)
    then
      begin
        libdatamsg (q660, LIBMSG_DISCARD, seed2string(q->location, q->seedname, s)) ;
        return ; /* don't know what time it is yet */
      end
  if (seqspread (q660->dt_data_sequence, q->dtsequence) <= 0) /* has this second of data already been processed? */
    then
      begin
        if (abs(seqspread(q660->dt_data_sequence, q->dtsequence)) > MAXSPREAD)
          then
            q->dtsequence = q660->dt_data_sequence ; /* if we're not within continuity distance, forget it */
        return ;
      end
  if (seq != q->seg_seq)
    then
      begin /* New Second */
        pcmp->data_ptr = pcmp->data_buf ;
        pcmp->map_ptr = pcmp->map_buf ;
        pcmp->map_cntr = 0 ;
        memclr (pcmp->map_buf, pcmp->map_size) ;
        pcmp->blocks = 0 ;
      end
  q->seg_seq = seq ;
  pm = p ; /* Start of map */
  size = ((word)datahdr.blk_size + 1) shl 2 ; /* Total blockette size */
  offset = (datahdr.offset_flag and DHOF_MASK) shl 2 ; /* Offset to start of data */
  dsize = size - offset ; /* Number of data bytes */
  pd = (pbyte)((pntrint)psave + offset) ; /* Start of data blocks */
  if ((pcmp->map_cntr == 0) land (pcmp->map_ptr == pcmp->map_buf))
    then
      begin /* First or only segment */
        if (datahdr.offset_flag and DHOF_PREV)
          then
            pcmp->prev_sample = datahdr.prev_offset ;
        if ((datahdr.offset_flag and DHOF_MORE) == 0)
          then
            begin /* Only one segment */
              pcmp->pmap = p ;
              pcmp->pdata = pd ;
              pcmp->mapidx = 0 ;
              pcmp->blocks = dsize shr 2 ; /* Number of 32 bit blocks */
              process_lcq (q660, q, -1, 0.0) ;
              pcmp->map_cntr = 0 ;
              pcmp->map_ptr = pcmp->map_buf ;
              pcmp->data_ptr = pcmp->data_buf ;
              memclr (pcmp->map_buf, pcmp->map_size) ;
              return ;
            end
      end
  memcpy (pcmp->data_ptr, pd, dsize) ; /* move in data to merged buffer */
  incn (pcmp->data_ptr, dsize) ;
  msize = dsize shr 1 ; /* Number of bits in map */
  src_cntr = 0 ; /* Start of new blockette map */
  pcmp->blocks = pcmp->blocks + (dsize shr 2) ; /* keep track of 32 bit blocks */
  if ((pcmp->map_cntr == 0) land (src_cntr == 0))
    then
      begin /* expedited map copy */
        cnt = (msize + 7) shr 3 ; /* number of bytes in source, to byte size */
        memcpy (pcmp->map_ptr, pm, cnt) ;
        pcmp->map_cntr = msize ;
        while (pcmp->map_cntr >= 8)
          begin /* Need to reduce to within one byte */
            decn (pcmp->map_cntr, 8) ;
            inc (pcmp->map_ptr) ;
          end
      end
    else
      while (msize > 0)
        begin
          dmask = DMASKS[pcmp->map_cntr shr 1] ; /* the valid bits within the current dest. */
          dest = *(pcmp->map_ptr) and dmask ;
          smask = SMASKS[src_cntr shr 1] ; /* same for source */
          src = *pm and smask ;
          shift = pcmp->map_cntr - src_cntr ; /* result may be negative! */
          if (shift > 0)
            then
              src = src shr shift ;
          else if (shift < 0)
            then
              src = src shl (-shift) ;
          dest = dest or src ; /* merge existing bits with new ones */
          *(pcmp->map_ptr) = dest ; /* and update */
          used = pcmp->map_cntr ;
          if (src_cntr > used)
            then
              used = src_cntr ; /* find higher of the two */
          used = 8 - used ; /* this is actual number of bits used */
          if (used > msize)
            then
              used = msize ; /* but no larger than we have in src */
          incn (src_cntr, used) ;
          incn (pcmp->map_cntr, used) ;
          decn (msize, used) ;
          while (pcmp->map_cntr >= 8)
            begin /* Need to reduce to within one word */
              decn (pcmp->map_cntr, 8) ;
              inc (pcmp->map_ptr) ;
            end
          while (src_cntr >= 8)
            begin /* Need to reduce to within one word */
              decn (src_cntr, 8) ;
              inc (pm) ;
            end
        end
  if ((datahdr.offset_flag and DHOF_MORE) == 0)
    then
      begin /* last segment */
        pcmp->pmap = (pbyte)pcmp->map_buf ;
        pcmp->pdata = (pbyte)pcmp->data_buf ;
        pcmp->mapidx = 0 ;
        process_lcq (q660, q, -1, 0.0) ;
        pcmp->map_cntr = 0 ;
        pcmp->map_ptr = pcmp->map_buf ;
        pcmp->data_ptr = pcmp->data_buf ;
        memclr (pcmp->map_buf, pcmp->map_size) ;
      end
end

