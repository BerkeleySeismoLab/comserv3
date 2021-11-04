/*   Lib660 time series configuration routines
     Copyright 2017-2019 Certified Software Corporation

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
    0 2017-06-11 rdr Created
    1 2018-04-06 rdr In lib_lcqstat description is read from DATA XML, not made up here.
    2 2018-08-28 rdr Show filter delays down to usec.
    3 2018-08-29 rdr In init_lcq fix handling of sub-hz rates when allocating buffers.
                     Remove logdispatch table, it wasn't used.
    4 2018-08-31 rdr Missed changing one of the places delay is shown to 6 places.
    5 2019-06-26 rdr Add support for GPS data streams.
    6 2019-09-26 rdr Add init_ll_lcq.
    7 2010-01-28 jms allocated LL buffer for 100sps and above.
    8 2020-03-17 jms fix use of loc codes with dplcq channels
    9 2021-01-06 jms remove unused code. block admin DP channels on IDL
*/

#define OMITADMINCHANNELSONIDL



#include "libsampcfg.h"
#include "xmlseed.h"
#include "libmsgs.h"
#include "libseed.h"
#include "libcmds.h"
#include "libcont.h"
#include "libsupport.h"
#include "libfilters.h"
#include "libarchive.h"
#include "liblogs.h"
#include "libsample.h"
#include "libverbose.h"

#define LOWEST_LL_FREQ 100

longword secsince (void)
begin

  return lib_round(now()) ;
end

void deallocate_sg (pq660 q660)
begin
  longword totrec ;
  string63 s ;
  string31 s2 ;

  if (q660->need_sats)
    then
      finish_log_clock (q660) ;
  if (q660->cur_verbosity and VERB_LOGEXTRA)
    then
      begin
        flush_lcqs (q660) ;
        flush_dplcqs (q660) ;
        totrec = print_generated_rectotals (q660) ;
        sprintf (s, "Done: %d recs. data end: %s", (integer)totrec, jul_string(q660->dt_data_sequence, s2)) ;
        libdatamsg (q660, LIBMSG_TOTAL, s) ;
      end
  flush_lcqs (q660) ;
  save_continuity (q660) ;
/* info not available for this
  totrec = print_actual_rectotals (q660) ;
  newuser.msg = inttostr(totrec) + ' recs. seq end: ' +
                 inttostr(dt_data_sequence) ;
*/
  memclr (addr(q660->first_clear), (integer)((pntrint)addr(q660->last_clear) - (pntrint)addr(q660->first_clear))) ;
  memclr (addr(q660->share.first_share_clear), (integer)((pntrint)addr(q660->share.last_share_clear) -
                                              (pntrint)addr(q660->share.first_share_clear))) ;
  initialize_memory_utils (addr(q660->connmem)) ;
end

pchar realtostr (double r, integer digits, pchar result)
begin
  string15 fmt ;

  sprintf(fmt, "%%%d.%df", digits + 2, digits) ;
  sprintf(result, fmt, r) ;
  return result ;
end

static pchar scvrate (integer rate, pchar result)
begin

  if (rate >= 0)
    then
      sprintf(result, "%d", rate) ;
    else
      realtostr(-1.0 / rate, 4, result) ;
  return result ;
end

static pchar sfcorner (pq660 q660, integer i, pchar result)
begin
  enum tlinear filt ;

  lock (q660) ;
  filt = q660->share.digis[i].linear ;
  unlock (q660) ;
  switch (filt) begin
    case LIN_ALL :
      strcpy(result, "Linear all") ;
      break ;
    case LIN_100 :
      strcpy(result, "Linear below 100sps") ;
      break ;
    case LIN_40 :
      strcpy(result, "Linear below 40sps") ;
      break ;
    default :
      strcpy(result, "Linear below 20sps") ;
      break ;
  end
  return result ;
end

void set_gaps (plcq q)
begin

  if (q->rate > 0)
    then
      begin
        q->gap_secs = q->gap_threshold / q->rate ;
        if ((q->gen_src >= GDS_MD) land (q->gen_src <= GDS_AC) land (q->rate == 1))
          then
            q->gap_offset = 10.0 ; /* Only received every 10 seconds */
          else
            q->gap_offset = 1.0 ; /* default is one set of data points per second */
      end
    else
      begin
        q->gap_secs = q->gap_threshold * abs(q->rate) ;
        q->gap_offset = abs(q->rate) ; /* default is one data point per "rate" seconds */
      end
end

void set_gain_bits (pq660 q660, plcq q, byte *gb)
begin
  byte w ;
  integer i, chan ;

  chan = q->sub_field shr 4 ; /* get to channel bits */
  w = 0 ;
  i = q660->share.digis[chan].pgagain ;
  while (i > 1)
    begin
      inc(w) ;
      i = i shr 1 ;
    end
  w = w and 7 ; /* Only 3 bits allocated */
  if (q660->share.digis[chan].lowvolt)
    then
      w = w or DEB_LOWV ;
  *gb = w ;
end

static void setup_names (plcq q, pchar name)
begin
  integer i ;
  pchar pc ;

  pc = strchr(name, '-') ;
  if (pc)
    then
      begin
        *pc++ = 0 ; /* terminate location, move to seedname */
        strcpy (q->sseedname, pc) ;
        strcpy (q->slocation, name) ;
      end
    else
      begin
        q->slocation[0] = 0 ;
        strcpy (q->sseedname, name) ;
      end
  memset (addr(q->location), 0x20, 2) ;
  memset (addr(q->seedname), 0x20, 3) ;
  for (i = 0 ; i < (integer)strlen(q->slocation) ; i++)
    q->location[i] = q->slocation[i] ;
  for (i = 0 ; i < (integer)strlen(q->sseedname) ; i++)
    q->seedname[i] = q->sseedname[i] ;
end

void set_net_station (pq660 q660)
begin
  integer i ;

  memset (q660->network, 0x20, 2) ;
  memset (q660->station, 0x20, 5) ;
  for (i = 0 ; i < (integer)strlen(q660->share.seed.network) ; i++)
    q660->network[i] = q660->share.seed.network[i] ;
  for (i = 0 ; i < (integer)strlen(q660->share.seed.station) ; i++)
    q660->station[i] = q660->share.seed.station[i] ;
end

static void add_mdispatch_x (pq660 q660, pchan pch, plcq q, integer offset)
begin
  integer i, j ;

  i = (pch->sub_field shr 4) + offset ; /* get channel */
  j = pch->freqnum ; /* frequency bit */
  q660->mdispatch[i][j] = q ;
end

void init_lcq (pq660 q660)
begin
  plcq p, pl, pl2 ;
  integer i, count, crate ;
  pchan pch, psrc ;
  string63 s ;
  string31 s1, s2, s3, s4 ;
  pcompressed_buffer_ring pr, lastpr ;
  integer buffers ;
  pdownstream_packet pds ;
#ifdef adsfadsfadsf
  pdet_packet pdp ;
  pdetect pd ;
#endif

  pch = q660->chanchain ;
  count = 0 ;
  while (pch)
    begin
      getbuf (addr(q660->connmem), (pointer)addr(pch->auxinfo), sizeof(tlcq)) ; /* for non-xml fields */
      pl = pch->auxinfo ;
      if (q660->lcqs == NIL)
        then
          q660->lcqs = pl ;
        else
          q660->lcqs = extend_link (q660->lcqs, pl) ;
      /* Setup lcq fields from XML configuration */
      pl->def = pch ;
      if (pch->rate > 0.9)
        then
          pl->rate = lib_round(pch->rate) ;
      else if (pch->rate != 0.0)
        then
          pl->rate = -lib_round(1.0 / pch->rate) ;
        else
          pl->rate = 0 ;
      setup_names (pl, pch->seedname) ;
      pl->lcq_num = q660->highest_lcqnum ;
      inc (q660->highest_lcqnum) ;
      pl->caldly = 60 ;
      pl->delay = pch->delay ;
      pl->gen_src = pch->gen_src ;
      pl->sub_field = pch->sub_field ;
      getbuf (addr(q660->connmem), (pointer)addr(pl->com), sizeof(tcom_packet)) ;
      pl->firfixing_gain = 1.000 ; /* default if not over-ridden */
      pl->com->maxframes = FRAMES_PER_RECORD - 1 ;
      pl->com->frame = 1 ;
      pl->com->next_compressed_sample = 1 ;
      pl->pre_event_buffers = 0 ; /* NEEDS WORK */
      pl->gap_threshold = 0.5 ;
      set_gaps (pl) ;
      pl->firfixing_gain = 1.0 ;
      if ((pl->gen_src >= GDS_MD) land (pl->gen_src <= GDS_AC))
        then
          begin
            set_gain_bits (q660, pl, addr(pl->gain_bits)) ;
            /* Get digitizer channel */
            i = pl->sub_field shr 4 ;
            switch (pl->gen_src) begin
              case GDS_CM :
                i = i + CAL_CHANNEL ;
                break ;
              case GDS_AC :
                i = i + MAIN_CHANNELS ;
                break ;
              default :
                break ;
            end
            if ((pl->sub_field and 0xF) == 0)
              then
                pl->delay = pl->delay + 9.0 ; /* Compensate for bunching into 10 second blocks */
            if (q660->cur_verbosity and VERB_LOGEXTRA)
              then
                begin
                  sprintf(s, "%s:%d@%s,%s=%s", seed2string(pl->location, pl->seedname, s1),
                          i + 1, scvrate(pl->rate, s2), sfcorner(q660, i, s3), realtostr(pl->delay, 6, s4)) ;
                  libmsgadd (q660, LIBMSG_FILTDLY, s) ;
                end
          end
      switch (pl->gen_src) begin
        case GDS_MD :
          add_mdispatch_x (q660, pch, pl, 0) ;
          break ;
        case GDS_CM :
          add_mdispatch_x (q660, pch, pl, CAL_CHANNEL) ;
          break ;
        case GDS_AC :
          add_mdispatch_x (q660, pch, pl, MAIN_CHANNELS) ;
          break ;
        case GDS_TIM :
          q660->timdispatch[(enum ttimfld)pl->sub_field] = pl ;
          break ;
        case GDS_SOH :
          q660->sohdispatch[(enum tsohfld)pl->sub_field] = pl ;
          break ;
        case GDS_ENG :
          q660->engdispatch[(enum tengfld)pl->sub_field] = pl ;
          break ;
        case GDS_GPS :
          q660->gpsdispatch[(enum tgpsfld)pl->sub_field] = pl ;
          break ;
        case GDS_DEC : /* decimated from another LCQ */
          psrc = q660->chanchain ;
          while (psrc)
            if (strcmp(psrc->seedname, pch->decsrc) == 0)
              then
                begin
                  pl->prev_link = (plcq)psrc->auxinfo ;
                  getbuf (addr(q660->connmem), (pointer)addr(pds), sizeof(tdownstream_packet)) ;
                  pds->link = NIL ;
                  pds->derived_q = pl ;
                  pl2 = psrc->auxinfo ;
                  pl2->downstream_link = extend_link (pl2->downstream_link, pds) ;
                  break ;
                end
              else
                psrc = psrc->link ;
          if (pch->decfilt[0])
            then
              begin
                pl->source_fir = find_fir (q660, pch->decfilt) ;
                if (pl->source_fir == NIL)
                  then
                    libmsgadd(q660, LIBMSG_DECNOTFOUND, pch->seedname) ;
              end
          pl2 = psrc->auxinfo ;
          pl->input_sample_rate = psrc->rate ; /* In floating point */
          if (pl->input_sample_rate >= 0.999)
            then
              pl->gap_offset = 10.0 ;
          else if (pl->input_sample_rate != 0)
            then
              pl->gap_offset = 1.0 / pl->input_sample_rate ; /* set new gap offset based on input rate */
            else
              pl->gap_offset = 1.0 ;
          if ((pl->source_fir != NIL) land (pl->input_sample_rate != 0))
            then
              pl->delay = pl2->delay + pl->source_fir->dly / pl->input_sample_rate ;
            else
              pl->delay = pl2->delay ;
          if (q660->cur_verbosity and VERB_LOGEXTRA)
            then
              begin
                sprintf(s, "%s:%s@%s=%s", seed2string(pl->location, pl->seedname, s1),
                        seed2string(pl->prev_link->location, pl->prev_link->seedname, s2),
                        scvrate(pl->rate, s3), realtostr(pl->delay, 6, s4)) ;
                libmsgadd (q660, LIBMSG_FILTDLY, s) ;
              end
          pl->com->charging = TRUE ;
          p = pl->prev_link ;
          while (p->prev_link)
            p = p->prev_link ;
          /* see if root source is 1hz */
          if ((p) land (p->rate == 1))
            then
              begin
                /* yes, need to synchronize based on rate */
                pl->slipping = TRUE ;
                pl->slip_modulus = abs(pl->rate) ; /* .1hz has modulus of 10 */
              end
          /* see if root source is main digitizer */
          if ((p) land (p->def->gen_src >= GDS_MD) land (p->def->gen_src <= GDS_AC))
            then
              set_gain_bits (q660, p, addr(pl->gain_bits)) ;
          break ;
        default :
          break ;
      end
     /* BUILD DETECTOR CHAIN HERE */
      crate = pl->rate ; /* Compression rate */
      if ((pl->gen_src >= GDS_MD) land (pl->gen_src <= GDS_AC) land (crate == 1))
        then
          crate = 10 ; /* 1Hz main digitizer data is compressed in 10 sample blocks */
      if (crate < 0)
        then
          crate = 1 ; /* sub 1Hz data is compressed one sample at a time */
      pl->datasize = crate * sizeof(longint) ;
      getbuf (addr(q660->connmem), (pointer)addr(pl->databuf), pl->datasize) ;
      if (crate > 1)
        then
          getbuf (addr(q660->connmem), (pointer)addr(pl->idxbuf), (crate + 1) * sizeof(word)) ;
      if (q660->par_create.call_minidata)
        then
          pl->mini_filter = q660->par_create.opt_minifilter and OMF_ALL ;
      if ((q660->arc_size > 0) land (q660->par_create.call_aminidata))
        then
          begin
            pl->arc.amini_filter = q660->par_create.opt_aminifilter and OMF_ALL ;
            pl->arc.incremental = (pl->rate <= q660->par_create.amini_512highest) ;
          end
      if (pl->rate >= LOWEST_LL_FREQ)
        then
          begin
            pl->precomp.data_size = pl->rate * sizeof(longint) ;
            pl->precomp.map_size = ((pl->rate shr 2) + 3) and 0xFFFC ; /* round up to 32 bits */
            getbuf (addr(q660->connmem), (pointer)addr(pl->precomp.map_buf), pl->precomp.map_size) ;
            getbuf (addr(q660->connmem), (pointer)addr(pl->precomp.data_buf), pl->precomp.data_size) ;
            pl->precomp.map_ptr = pl->precomp.map_buf ;
            pl->precomp.data_ptr = pl->precomp.data_buf ;
            pl->precomp.map_cntr = 0 ;
          end
        else
          begin
            pl->precomp.map_size = 0 ;
            pl->precomp.data_size = 0 ;
          end
      pl->pack_class = PKC_DATA ; /* assume data */
      set_gain_bits (q660, pl, addr(pl->gain_bits)) ;
      buffers = pl->pre_event_buffers + 1 ; /* need one for construction */
      pr = NIL ;
      lastpr = NIL ;
      while (buffers > 0)
        begin
          getbuf (addr(q660->connmem), (pointer)addr(pr), sizeof(tcompressed_buffer_ring)) ;
          pr->link = NIL ;
          pr->full = FALSE ;
          if (pl->com->ring == NIL)
            then
              pl->com->ring = pr ;
          else if (lastpr)
            then
              lastpr->link = pr ;
          lastpr = pr ;
          dec(buffers) ;
        end
      if (pr)
        then
          pr->link = pl->com->ring ;
      if (pl->arc.amini_filter)
        then
          getbuf (addr(q660->connmem), (pointer)addr(pl->arc.pcfr), q660->arc_size) ;
// Future event only handling goes here
      pl->scd_cont = SCD_BOTH ;
      allocate_lcq_filters (q660, pl) ;
      inc(count) ;
      pch = pch->link ;
    end
end

void init_ll_lcq (pq660 q660)
begin
  plcq pl ;
  integer idxsize ;

  pl = addr(q660->ll_lcq) ;
  pl->gen_src = GDS_MD ; /* To force setup */
  pl->datasize = MAX_RATE * sizeof(longint) ; /* allocate for largest */
  idxsize = (MAX_RATE + 1) * sizeof(word) ;
  getbuf (addr(q660->connmem), (pointer)addr(pl->databuf), pl->datasize) ;
  getbuf (addr(q660->connmem), (pointer)addr(pl->idxbuf), idxsize) ;
end ;

void init_dplcq (pq660 q660, plcq pl, boolean newone)
begin
  pcompressed_buffer_ring pr ;
  enum tlogfld acctype ;

  pl->dtsequence = 0 ;
  acctype = (enum tlogfld)pl->sub_field ;
  if (acctype >= AC_FIRST)
    then
      begin
        if (acctype <= AC_LAST)
          then
            q660->share.accmstats[acctype].ds_lcq = pl ;
        else if (acctype == LOGF_NDATLAT)
          then
            q660->data_latency_lcq = pl ;
        else if (acctype == LOGF_LDATLAT)
          then
            q660->low_latency_lcq = pl ;
        else if (acctype == LOGF_STATLAT)
          then
            q660->status_latency_lcq = pl ;
      end
  if (q660->par_create.call_minidata)
    then
      pl->mini_filter = q660->par_create.opt_minifilter and OMF_ALL ;
  if ((q660->arc_size > 0) land (q660->par_create.call_aminidata))
    then
      begin
        pl->arc.amini_filter = q660->par_create.opt_aminifilter and OMF_ALL ;
        pl->arc.incremental = pl->rate <= q660->par_create.amini_512highest ;
      end
  pl->scd_cont = SCD_BOTH ; /* both are continuous */
  pl->scd_evt = 0 ;
  switch (acctype) begin
    case LOGF_MSGS :
      pl->pack_class = PKC_MESSAGE ;
      break ;
    case LOGF_TIME :
      pl->pack_class = PKC_TIMING ;
      break ;
    case LOGF_CFG :
      pl->pack_class = PKC_OPAQUE ;
      break ;
    default :
      pl->pack_class = PKC_DATA ;
      break ;
  end
  if (pl->com->maxframes >= FRAMES_PER_RECORD)
    then
      pl->com->maxframes = FRAMES_PER_RECORD - 1 ;
  if (lnot newone)
    then
      return ; /* don't need to allocate new buffers */
  getbuf (addr(q660->thrmem), (pointer)addr(pr), sizeof(tcompressed_buffer_ring)) ;
  pr->full = FALSE ;
  pl->com->ring = pr ;
  pr->link = pl->com->ring ; /* just keeps going back to itself */
  if (pl->arc.amini_filter)
    then
      getbuf (addr(q660->thrmem), (pointer)addr(pl->arc.pcfr), q660->arc_size) ;
end

void process_dplcqs (pq660 q660)
begin
  integer count ;
  plcq cur_lcq ;
  pchan pch ;
  pchar pc ;
  string2 loc ;
  string3 name ;
  boolean newone ;
  string snamecpy ;

  pch = q660->dlchain ;
  count = 0 ;
  while (pch)
    begin

#ifdef OMITADMINCHANNELSONIDL
       if (strstr(q660->par_register.q660id_address, "@IDL")) {
            /* skip administrative DP channels on IDL */
            switch ((enum tlogfld)pch->sub_field) 
            {
              case LOGF_MSGS :
                break ;
              case LOGF_CFG :
                break ;
              case LOGF_TIME :
                break ;
              default :
                // fprintf(stderr,"SKIPPING: %s\n",pch->seedname);
                pch = pch->link ;
                continue;
            }
       }
#endif

      strcpy(snamecpy,pch->seedname) ;
      pc = strchr(snamecpy, '-') ;
      if (pc)
        then
          begin
            *pc++ = 0 ; /* terminate location, move to seedname */
            strcpy (name, pc) ;
            strcpy (loc, name) ;
          end
        else
          begin
            loc[0] = 0 ;
            strcpy (name, pch->seedname) ;
          end
      cur_lcq = q660->dplcqs ;
      newone = FALSE ;
      while (cur_lcq)
        if ((strcmp(cur_lcq->slocation, loc) == 0) land
            (strcmp(cur_lcq->sseedname, name) == 0))
          then
            break ;
          else
            cur_lcq = cur_lcq->link ;
      if (cur_lcq == NIL)
        then
          begin /* add new one */
            newone = TRUE ;
            getbuf (addr(q660->thrmem), (pointer)addr(cur_lcq), sizeof(tlcq)) ;
            if (q660->dplcqs == NIL)
              then
                q660->dplcqs = cur_lcq ;
              else
                q660->dplcqs = extend_link (q660->dplcqs, cur_lcq) ;
          end
      cur_lcq->def = pch ;
      setup_names (cur_lcq, pch->seedname) ;
      cur_lcq->delay = pch->delay ;
      cur_lcq->gen_src = pch->gen_src ;
      cur_lcq->sub_field = pch->sub_field ;
      switch ((enum tlogfld)cur_lcq->sub_field) begin
        case LOGF_MSGS :
          cur_lcq->rate = 0 ;
          q660->msg_lcq = cur_lcq ;
          break ;
        case LOGF_CFG :
          cur_lcq->rate = 0 ;
          q660->cfg_lcq = cur_lcq ;
          break ;
        case LOGF_TIME :
          cur_lcq->rate = 0 ;
          q660->tim_lcq = cur_lcq ;
          break ;
        default :
          cur_lcq->rate = -10 ;
          break ;
      end
      cur_lcq->lcq_num = 0xFF ; /* we don't actually use indexing for these lcqs */
      cur_lcq->validated = TRUE ;
      cur_lcq->firfixing_gain = 1.000 ; /* default if not over-ridden */
      if (newone)
        then
          begin
            getbuf (addr(q660->thrmem), (pointer)addr(cur_lcq->com), sizeof(tcom_packet)) ;
            cur_lcq->com->frame = 1 ;
            cur_lcq->com->next_compressed_sample = 1 ;
            cur_lcq->com->maxframes = FRAMES_PER_RECORD - 1 ;
          end
      cur_lcq->caldly = 60 ;
      if (cur_lcq->gap_threshold == 0.0)
        then
          cur_lcq->gap_threshold = 0.5 ;
      set_gaps (cur_lcq) ;
      init_dplcq (q660, cur_lcq, newone) ;
      if (newone)
        then
          preload_archive (q660, FALSE, cur_lcq) ;
      inc(count) ;
      pch = pch->link ;
    end
end

void init_dplcqs (pq660 q660)
begin
  plcq pl ;
  integer i ;

  pl = q660->dplcqs ;
  i = 0 ;
  while (pl)
    begin
      init_dplcq (q660, pl, TRUE) ;
      inc(i) ;
      pl = pl->link ;
    end
  if (q660->arc_size > 0)
    then
      preload_archive (q660, FALSE, NIL) ;
end

enum tliberr lib_lcqstat (pq660 q660, tlcqstat *lcqstat)
begin
  plcq q ;
  longword cur ;
  integer pass ;
  tonelcqstat *pone ;

  lcqstat->count = 0 ;
  cur = secsince () ;
  for (pass = 1 ; pass <= 2 ; pass++)
    begin
      if (pass == 1)
        then
          q = q660->lcqs ;
        else
          q = q660->dplcqs ;
      while ((lcqstat->count < MAX_LCQ) land (q))
        begin
          pone = addr(lcqstat->entries[lcqstat->count]) ;
          strcpy(pone->location, q->slocation) ;
          strcpy(pone->channel, q->sseedname) ;
          pone->chan_number = q->lcq_num ;
          pone->rec_cnt = q->records_generated_session ;
          pone->rec_seq = q->com->records_written ;
          if (q->last_record_generated == 0)
            then
              pone->rec_age = -1 ; /* not written */
            else
              pone->rec_age = cur - q->last_record_generated ;
          pone->det_count = q->detections_session ;
          pone->cal_count = q->calibrations_session ;
          pone->arec_cnt = q->arc.records_written_session ;
          pone->arec_over = q->arc.records_overwritten_session ;
          if (q->arc.last_updated == 0)
            then
              pone->arec_age = -1 ;
            else
              pone->arec_age = cur - q->arc.last_updated ;
          pone->arec_seq = q->arc.records_written ;
          if (q->def)
            then
              strcpy (pone->desc, q->def->desc) ;
            else
              strcpy (pone->desc, "") ;
          inc(lcqstat->count) ;
          q = q->link ;
        end
    end
  return LIBERR_NOERR ;
end

void clear_calstat (pq660 q660)
begin
  plcq pl ;

  if (q660->libstate != LIBSTATE_RUN)
    then
      return ;
  pl = q660->lcqs ;
  while (pl)
    begin
      pl->calstat = FALSE ; /* can't be on */
      pl = pl->link ;
    end
end

enum tliberr lib_getdpcfg (pq660 q660, tdpcfg *dpcfg)
begin
  plcq q ;

  if ((q660->libstate != LIBSTATE_RUN) land (q660->libstate != LIBSTATE_RUNWAIT))
    then
      return LIBERR_CFGWAIT ;
  memclr (dpcfg, sizeof(tdpcfg)) ;
  memcpy(addr(dpcfg->station_name), addr(q660->station_ident), sizeof(string9)) ;
  q = q660->lcqs ;
  while (q)
    begin
      dpcfg->buffer_counts[q->lcq_num] = q->pre_event_buffers + 1 ;
      q = q->link ;
    end
  return LIBERR_NOERR ;
end


