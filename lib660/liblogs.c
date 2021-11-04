/*   Lib660 Message Log Routines
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
    0 2017-06-08 rdr Created
    1 2017-07-15 rdr Fix clock type string processing.
    2 2017-07-16 rdr Use correct pointer for loadcalstart.
    3 2018-09-04 rdr Handle sine calibrations of 2Hz or greater. Fix cal abort
                     blockette handling.
    4 2021-01-06 jms fix incorrect use of "addr" operator that resulted in stack
                        corruption when time jump > 250us occurred.
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
void lib660_padright (pchar s, pchar b, integer fld)
begin
  integer i, j ;
  string s1 ;

  strcpy (s1, s) ;
  j = (integer)strlen(s1) ;
  if (j > fld)
    then
      j = fld ;
  for (i = 0 ; i < j ; i++)
    *b++ = s1[i] ;
  for (i = j ; i < fld ; i++)
    *b++ = ' ' ;
end

/* return two digit string, padded on left with zero */
pchar two (word w, pchar s)
begin
  integer v ;

  v = w ;
  sprintf(s, "%d", v mod 100) ;
  zpad (s, 2) ;
  return s ;
end

#ifdef adsfadsfasdf
/* same as zpad but pads on left with spaces */
static pchar spad (pchar s, integer lth)
begin
  integer len, diff ;

  len = strlen(s) ;
  diff = lth - len ;
  if (diff > 0)
    then
      begin
        memmove (addr(s[diff]), addr(s[0]), len + 1) ; /* shift existing string right */
        memset (addr(s[0]), ' ', diff) ; /* add spaces at front */
      end
  return s ;
end

static pchar string_time (tsystemtime *nt, pchar result)
begin
  string3 smth, sday, shour, smin, ssec ;

  sprintf(result, "%d-%s-%s %s:%s:%s", nt->wyear, two(nt->wmonth, smth),
          two(nt->wday, sday), two(nt->whour, shour),
          two(nt->wminute, smin), two(nt->wsecond, ssec)) ;
  return result ;
end
#endif

static void fix_time (tseed_time *st)
begin
  tsystemtime greg ;
  longint usec ;

  convert_time (st->seed_fpt, addr(greg), addr(usec)) ;
  lib660_seed_time (st, addr(greg), usec) ;
end

void log_clock (pq660 q660, enum tclock_exception clock_exception, pchar jump_amount)
begin
  string s ;
  tsystemtime newtime ;
  longint newusec ;
  pbyte p ;
  seed_header *phdr ;
  tcom_packet *pcom ;
  plcq q ;
  timing *ptim ;

  q = q660->tim_lcq ;
  if ((q == NIL) lor (q->com->ring == NIL))
    then
      return ;
  pcom = q->com ;
  phdr = addr(pcom->ring->hdr_buf) ;
  if (q660->need_sats)
    then
      finish_log_clock (q660) ; /* Already one pending, just use current satellite info */
  q660->last_update = q660->data_timetag ;
  memset(addr(pcom->ring->rec), 0, LIB_REC_SIZE) ;
  phdr->samples_in_record = 0 ;
  phdr->seed_record_type = 'D' ;
  phdr->continuation_record = ' ' ;
  phdr->sequence.seed_num = pcom->records_written + 1 ;
  inc(pcom->records_written) ;
  memcpy(addr(phdr->location_id), addr(q->location), sizeof(tlocation)) ;
  memcpy(addr(phdr->channel_id), addr(q->seedname), sizeof(tseed_name)) ;
  memcpy(addr(phdr->station_id_call_letters), addr(q660->station), sizeof(tseed_stn)) ;
  memcpy(addr(phdr->seednet), addr(q660->network), sizeof(tseed_net)) ;
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
  ptim = addr(q660->timing_buf) ;
  ptim->blockette_type = 500 ;
  ptim->next_blockette = 0 ;
  ptim->vco_correction = q660->share.stat_pll.cur_vco / 50.0 ;
  convert_time (q660->data_timetag, addr(newtime), addr(newusec)) ;
  lib660_seed_time (addr(ptim->time_of_exception), addr(newtime), newusec) ;
  ptim->usec99 = newusec mod 100 ;
  ptim->reception_quality = q660->data_qual ;
  ptim->exception_count = q660->except_count ;
  q660->except_count = 0 ;
  switch (clock_exception) begin
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
  end
  lib660_padright (q660->share.sysinfo.clk_typ, ptim->clock_model, 32) ;
  if (clock_exception == CE_JUMP)
    then
      begin
        p = (pbyte)addr(pcom->ring->rec) ;
        storeseedhdr (addr(p), phdr, FALSE) ;
        storetiming (addr(p), ptim) ;
        inc(q->records_generated_session) ;
        q->last_record_generated = secsince () ;
        send_to_client (q660, q, pcom->ring, SCD_BOTH) ;
      end
    else
      q660->need_sats = TRUE ;
end

void finish_log_clock (pq660 q660)
begin
  string s ;
  string7 s1 ;
  word w ;
  pbyte p ;
  tcom_packet *pcom ;
  plcq q ;
  timing *ptim ;
  tstat_gpssat *pone ;

  q = q660->tim_lcq ;
  if ((q == NIL) lor (q->com->ring == NIL))
    then
      return ;
  pcom = q->com ;
  q660->need_sats = FALSE ;
  ptim = addr(q660->timing_buf) ;
  if (q660->share.stat_gps.sat_count)
    then
      begin
        strcpy(s, "SNR=") ;
        for (w = 0 ; w <= q660->share.stat_gps.sat_count - 1 ; w++)
          begin
            pone = addr(q660->share.stat_gps.gps_sats[w]) ;
            if ((pone->num) land (pone->snr >= 20))
              then
                begin
                  if (strlen(s) > 4)
                    then
                      strcat(s, ",") ;
                  sprintf(s1, "%d", pone->snr) ;
                  strcat(s, s1) ;
                end
          end
      end
    else
      s[0] = 0 ;
  lib660_padright (s, ptim->clock_status, 128) ;
  p = (pbyte)addr(pcom->ring->rec) ;
  storeseedhdr (addr(p), (pvoid)addr(pcom->ring->hdr_buf), FALSE) ;
  storetiming (addr(p), ptim) ;
  inc(q->records_generated_session) ;
  q->last_record_generated = secsince () ;
  send_to_client (q660, q660->tim_lcq, pcom->ring, SCD_BOTH) ;
end

void flush_timing (pq660 q660)
begin
  plcq q ;

  q = q660->tim_lcq ;
  if ((q == NIL) lor (q->com->ring == NIL))
    then
      return ;
  if ((q->arc.amini_filter) land (q->arc.total_frames > 0))
    then
      flush_archive (q660, q) ;
end

void logevent (pq660 q660, pdet_packet det, tonset_mh *onset)
begin
#ifdef dasfadsfsdf
  string on_, s ;
  string63 w ;
  string31 s1, s2 ;
  boolean mh ;
  tsystemtime newtime ;
  longint newusec ;
  murdock_detect eblk ;
  threshold_detect *pt ;
  double ts ;
  byte buffer[FRAME_SIZE] ; /* for creating "blockette image" */
  pbyte p ;
  plcq ppar ;
  pdetect pdef ;

  q660 = paqs->owner ;
  ppar = det->parent ;
  pdef = det->detector_def ;
  mh = (pdef->dtype == MURDOCK_HUTT) ;
  if (mh)
    then
      sprintf(on_, "%c %c %c %c%c%c%c%c", onset->event_detection_flags + 0x63,
              onset->pick_algorithm + 0x41, onset->lookback_value + 0x30,
              onset->snr[0] + 0x30, onset->snr[1] + 0x30, onset->snr[2] + 0x30,
              onset->snr[3] + 0x30, onset->snr[4] + 0x30) ;
    else
      strcpy(on_, "           ") ;
  ts = onset->signal_onset_time.seed_fpt ;
  convert_time (onset->signal_onset_time.seed_fpt, addr(newtime), addr(newusec)) ;
  sprintf(s2, "%d", newusec) ;
  zpad(s2, 6) ;
  sprintf(w, " %s.%s ", string_time (addr(newtime), s1), s2) ;
  strcat(on_, w) ;
  if ((lnot mh) lor (onset->signal_amplitude >= 0))
    then
      begin
        sprintf(s1, "%d", lib_round(onset->signal_amplitude)) ;
        spad(s1, 10) ;
        strcat(on_, s1) ;
        strcat(on_, " ") ;
      end
    else
      strcat(on_, "?????????? ") ;
  sprintf(s2, "%d", lib_round(onset->background_estimate)) ;
  if (mh)
    then
      begin
        if ((onset->signal_period > 0) land (onset->signal_period < 1000))
          then
            begin
              sprintf(s1, "%6.2f ", onset->signal_period) ;
              strcat(on_, s1) ;
            end
          else
            strcat(on_, "???.?? ") ;
        if (onset->background_estimate >= 0)
          then
            begin
              spad(s2, 7) ;
              strcat(on_, s2) ;
            end
          else
            strcat(on_, "???????") ;
      end
    else
      begin
        zpad(s2, 14) ;
        strcat(on_, s2) ;
      end
  strcat(on_, " ") ;
  sprintf(w, "%s:%s", seed2string(ppar->location, ppar->seedname, s2), pdef->detname) ;
  while (strlen(w) < 18)
    strcat(w, " ") ;
  if (det->det_options and DO_MSG)
    then
      begin
        sprintf(s, "%s-%s", w, on_) ;
        libmsgadd(q660, LIBMSG_DETECT, s) ;
      end
  lib660_seed_time (addr(eblk.mh_onset.signal_onset_time), addr(newtime), newusec) ;
  eblk.mh_onset.signal_amplitude = onset->signal_amplitude ;
  eblk.mh_onset.signal_period = onset->signal_period ;
  eblk.mh_onset.background_estimate = onset->background_estimate ;
  eblk.mh_onset.event_detection_flags = onset->event_detection_flags ;
  eblk.mh_onset.reserved_byte = 0 ;
  pt = (pointer)addr(eblk) ;
  p = addr(buffer) ;
  if (mh)
    then
      begin
        eblk.blockette_type = 201 ;
        eblk.next_blockette = 0 ;
        memcpy(addr(eblk.mh_onset.snr), addr(onset->snr), 6) ;
        eblk.mh_onset.lookback_value = onset->lookback_value ;
        eblk.mh_onset.pick_algorithm = onset->pick_algorithm ;
        lib660_padright (addr(pdef->detname), addr(eblk.s_detname), 24) ;
        storemurdock (addr(p), addr(eblk)) ;
      end
    else
      begin
        pt->blockette_type = 200 ;
        pt->next_blockette = 0 ;
        lib660_padright (addr(pdef->detname), addr(pt->s_detname), 24) ;
        storethreshold (addr(p), pt) ;
      end
  if (ppar->lcq_opt and LO_DETP)
    then
      begin
        if (q660->par_create.mini_embed)
          then
            add_blockette (paqs, ppar, (pword)addr(buffer), ts) ;
        if (q660->par_create.mini_separate)
          then
            build_separate_record (paqs, ppar, (pword)addr(buffer), ts, PKC_EVENT) ;
      end
#endif
end

static void set_cal2 (pcal2 pc2, tcalstart *cals, pq660 q660)
begin
  integer sub ;
  plcq pq ;

  pc2->calibration_amplitude = (cals->amplitude + 1) * -6 ;
  pc2->cal2_res = 0 ;
  pc2->ref_amp = 0 ;
  lib660_padright ("Resistive", pc2->coupling, 12) ;
  lib660_padright ("3DB@10Hz", pc2->rolloff, 12) ;
  memset(addr(pc2->calibration_input_channel), ' ', 3) ;
  for (sub = FREQS - 1 ; sub >= 0 ; sub--)
    begin
      pq = q660->mdispatch[CAL_CHANNEL][sub] ;
      if (pq)
        then
          begin
            memcpy (addr(pc2->calibration_input_channel), addr(pq->seedname), 3) ;
            break ;
          end
    end
end

void log_cal (pq660 q660, pbyte pb, boolean start)
begin
  tcalstart cals ;
  plcq q ;
  integer idx, sub ;
  word map ;
  step_calibration *pstep ;
  sine_calibration *psine ;
  random_calibration *prand ;
  abort_calibration *pabort ;
  random_calibration cblk ;
  byte buffer[FRAME_SIZE] ; /* for creating "blockette image" */
  pbyte p ;

  memclr(addr(cblk), sizeof(random_calibration)) ;
  p = (pbyte)addr(buffer) ;
  if (start)
    then
      begin
        loadcalstart (addr(pb), addr(cals)) ;
        map = cals.calbit_map ;
        psine = (pvoid)addr(cblk) ;
        /* common for all waveforms */
        psine->next_blockette = 0 ;
        psine->calibration_time.seed_fpt = q660->data_timetag ;
        fix_time (addr(psine->calibration_time)) ;
        if (cals.waveform and 0x80)
          then
            psine->calibration_flags = 4 ; /* automatic */
          else
            psine->calibration_flags = 0 ;
        psine->calibration_duration = (longint)cals.duration * 10000 ;
        switch ((byte)(cals.waveform and 7)) begin
          case 0 : /* sine */
            psine->blockette_type = 310 ;
            psine->calibration_flags = psine->calibration_flags or 0x10 ;
            if (cals.freqdiv and 0x8000)
              then
                psine->sine_period = 1.0 / (cals.freqdiv and 0xFF) ; /* 2Hz to 20Hz */
              else
                psine->sine_period = cals.freqdiv ;
            set_cal2 (addr(psine->sine2), addr(cals), q660) ;
            storesine (addr(p), psine) ;
            break ;
          case 1 :
          case 2 :
          case 4 : /* random */
            prand = addr(cblk) ;
            prand->blockette_type = 320 ;
        /* white uses random amplitudes, red and telegraph don't */
            if ((byte)(cals.waveform and 7) == 2)
              then
                prand->calibration_flags = prand->calibration_flags or 0x10 ;
            set_cal2 (addr(prand->random2), addr(cals), q660) ;
            switch ((byte)(cals.waveform and 7)) begin
              case 1 :
                lib660_padright("Red",prand->noise_type, 8) ;
                break ;
              case 2 :
                lib660_padright("White", prand->noise_type, 8) ;
                break ;
              case 4 :
                lib660_padright("Telegraf", prand->noise_type, 8) ;
                break ;
            end
            storerandom (addr(p), prand) ;
            break ;
          case 3 : /* step */
            pstep = (pvoid)addr(cblk) ;
            pstep->blockette_type = 300 ;
            pstep->number_of_steps = 1 ;
            if ((cals.waveform and 0x40) == 0)
              then
                pstep->calibration_flags = pstep->calibration_flags or 1 ; /* positive pulse */
            pstep->interval_duration = 0 ;
            set_cal2 (addr(pstep->step2), addr(cals), q660) ;
            storestep (addr(p), pstep) ;
            break ;
        end
      end
    else
      begin
        loadbyte (addr(pb)) ;
        loadbyte (addr(pb)) ;
        map = loadword (addr(pb)) ;
        pabort = (pvoid)addr(cblk) ;
        pabort->blockette_type = 395 ;
        pabort->next_blockette = 0 ;
        pabort->calibration_time.seed_fpt = q660->data_timetag ;
        fix_time (addr(pabort->calibration_time)) ;
        storeabort (addr(p), pabort) ;
      end ;
  /* need a record for each channel in the map */
  for (idx = 0 ; idx <= TOTAL_CHANNELS - 1 ; idx++)
    if (map and (1 shl idx))
      then
        for (sub = 0 ; sub <= FREQS - 1 ; sub++)
          begin
            q = q660->mdispatch[idx][sub] ;
            if (q)
              then
                begin
                  inc(q->calibrations_session) ;
                  if (q660->par_create.mini_embed)
                    then
                      add_blockette (q660, q, (pword)addr(buffer), q660->data_timetag) ;
                  if (q660->par_create.mini_separate)
                    then
                      build_separate_record (q660, q, (pword)addr(buffer), q660->data_timetag, PKC_CALIBRATE) ;
                end
          end
end

void log_message (pq660 q660, pchar msg)
begin
  integer i ;
  double ts ;
  pbyte p ;
  pchar pc ;
  plcq q ;
  tcom_packet *pcom ;
  seed_header *phdr ;

  q = q660->msg_lcq ;
  if ((q == NIL) lor (q->com->ring == NIL))
    then
      return ; /* not initialized */
  ts = now() ;
  pcom = q->com ;
  phdr = addr(pcom->ring->hdr_buf) ;
  i = (integer)strlen(msg) + 2 ;
  if ((pcom->frame >= 2) land
     (((pcom->frame + i) > NONDATA_AREA) lor (ts > (phdr->starting_time.seed_fpt + 60))))
    then
      begin /* won't fit in current frame or was too long ago */
        phdr->samples_in_record = pcom->frame ;
        q660->nested_log = TRUE ;
        p = (pbyte)addr(pcom->ring->rec) ;
        storeseedhdr (addr(p), (pvoid)phdr, FALSE) ;
        inc(q->records_generated_session) ;
        q->last_record_generated = secsince () ;
        send_to_client (q660, q, pcom->ring, SCD_BOTH) ;
        q660->nested_log = FALSE ;
        pcom->frame = 0 ;
      end
  if (pcom->frame < 2)
    then
      begin /* at least a blank line */
        memset(addr(pcom->ring->rec), 0, LIB_REC_SIZE) ;
        phdr->samples_in_record = 0 ;
        pcom->frame = 0 ;
        phdr->starting_time.seed_fpt = ts ;
        phdr->seed_record_type = 'D' ;
        phdr->continuation_record = ' ' ;
        phdr->sequence.seed_num = pcom->records_written + 1 ;
        inc(pcom->records_written) ;
        memcpy(addr(phdr->location_id), addr(q->location), sizeof(tlocation)) ;
        memcpy(addr(phdr->channel_id), addr(q->seedname), sizeof(tseed_name)) ;
        memcpy(addr(phdr->station_id_call_letters), addr(q660->station), sizeof(tseed_stn)) ;
        memcpy(addr(phdr->seednet), addr(q660->network), sizeof(tseed_net)) ;
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
      end
  strcat(msg, "\x0D\x0A") ;
  pc = (pointer)((pntrint)addr(pcom->ring->rec) + 56 + pcom->frame) ; /* add text to data area */
  memcpy(pc, msg, strlen(msg)) ;
  incn(pcom->frame, strlen(msg)) ;
  q660->log_timer = LOG_TIMEOUT ;
end

void flush_messages (pq660 q660)
begin
  pbyte p ;
  plcq q ;
  tcom_packet *pcom ;
  seed_header *phdr ;

  q = q660->msg_lcq ;
  if ((q == NIL) lor (q->com->ring == NIL))
    then
      return ; /* not initialized */
  pcom = q->com ;
  phdr = addr(pcom->ring->hdr_buf) ;
  if (pcom->frame >= 2)
    then
      begin
        phdr->samples_in_record = pcom->frame ;
        q660->nested_log = TRUE ;
        p = (pbyte)addr(pcom->ring->rec) ;
        storeseedhdr (addr(p), (pvoid)phdr, FALSE) ;
        inc(q->records_generated_session) ;
        q->last_record_generated = secsince () ;
        send_to_client (q660, q, pcom->ring, SCD_BOTH) ;
        q660->nested_log = FALSE ;
        pcom->frame = 0 ;
      end
  if ((q->arc.amini_filter) land (q->arc.hdr_buf.samples_in_record))
    then
      flush_archive (q660, q) ;
  q660->log_timer = 0 ;
end


