/*   Lib660 MD5 Routines
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
    1 2021-01-06 jms omit admin DP channels on IDL
*/

#define OMITADMINCHANNELSONIDL


#ifndef libcont_h
#include "libcont.h"
#endif

#include "libseed.h"
#include "libclient.h"
#include "libstats.h"
#include "libsupport.h"
#include "libmsgs.h"
#include "libsampglob.h"
#include "libsampcfg.h"
#include "libdetect.h"

#define CT_VER 0
#define CTY_STATIC 0 /* Static storage for status, etc */
#define CTY_SYSTEM 1 /* system identification */
#define CTY_IIR 2 /* IIR filter */
#define CTY_FIR 3 /* FIR filter */
#define CTY_LCQ 5 /* A LCQ */
#define CTY_RING 6 /* Ring Buffer Entry */
#define CTY_SL 7 /* STA/LTA Detector */
#define CTY_THR 8 /* Threshold Detector */
#define CTY_DPLCQ 9 /* DP LCQ */
#define CTY_PURGED 86 /* Continuity file already used */
#define DP_MESSAGE 0x7F /* when used as dp_src means message log */

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
} tctyhdr ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  byte version ;
  tseed_net net ;
  tseed_stn stn ; /* these make sure nothing tricky has happened */
  boolean spare1 ; /* was timezone adjustment from continuity file */
  longint spare ; /* was timezone offset */
  integer mem_required ; /* amount of memory required */
  integer thrmem_required ; /* amount of thread memory required */
  longint time_written ; /* seconds since 2016 that this was written */
  integer stat_minutes ; /* minutes worth of info will go in this slot */
  integer stat_hours ; /* hours worth will go into this slot */
  longint total_minutes ; /* total minutes accumulated */
  double timetag_save ; /* for the purposes of calculating data latency */
  double last_status_save ; /* for the purposes of calculating status latency */
  longint tag_save ; /* tagid save */
  t64 sn_save ; /* serial number */
  longword reboot_save ; /* reboot time save */
  topstat opstat ; /* snapshot of operational status */
  taccmstats accmstats ; /* snapshot of opstat generation information */
} tstatic ;
typedef tstatic *pstatic ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  byte version ;
  byte high_freq ; /* high frequency encoding */
  t64 serial ; /* serial number of q660 */
  double lasttime ; /* data_timetag of last second of data */
  word last_dataqual ; /* 0-100% */
  longword last_dataseq ; /* data record sequence. sequence continuity important too */
  longword reboot_counter ; /* last reboot counter */
} tsystem ;

typedef struct {
  tvector x ;
  tvector y ;
} tiirvalues ;
typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  char fn[FILTER_NAME_LENGTH] ;
  tiirvalues flt[MAXSECTIONS + 1] ; /* element 0 not used */
  tfloat outbuf ; /* this may be an array */
} tctyiir ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  word fcnt ; /* current number of samples in fir buffer */
  longint foff ; /* offset from start of buffer to "f" pointer */
  char fn[FILTER_NAME_LENGTH] ;
  tfloat fbuffer ; /* this is an array */
} tctyfir ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  longword lastdtsequence ; /* last data record sequence processed */
  integer prev_rate ; /* rate when continuity written */
  tfloat prev_delay ; /* delay when continuity written */
  boolean glast ; /* gen_last_on */
  boolean con ; /* cal_on */
  boolean cstat ; /* calstat */
  boolean overwrite_slipping ;
  byte qpad ;
  word cinc ; /* calinc */
  longint rec_written ; /* records written */
  longint arec_written ; /* archive records written */
  longword gnext ; /* gen_next */
  longint last_sample ; /* last compression sample */
  double nextrec_tag ; /* this is the expected starting time of the next record */
  double lastrec_tag ; /* this was the last timetag used */
  double backup_timetag ;
  word backup_timequal ;
} tctylcq ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  byte dp_src ; /* selects among the DP Statistics */
  longword lcq_options ;
  integer frame_limit ;
  single gap_thresh ;
  longint rec_written ; /* records written */
  longint arec_written ; /* archive records written */
  longint last_sample ; /* last compression sample */
  double nextrec_tag ; /* this is the expected starting time of the next record */
  double lastrec_tag ; /* this was the last timetag used */
} tctydplcq ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  word spare ; /* for longword alignment */
  completed_record comprec ;
} tctyring ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  char dn[DETECTOR_NAME_LENGTH] ;
//  con_sto mh_cont ;
} tctymh ;

typedef struct {
  longint crc ; /* CRC of everything following */
  word id ; /* what kind of entry */
  word size ; /* size of this entry */
  tlocation loc ;
  tseed_name name ;
  byte lpad ;
  char dn[DETECTOR_NAME_LENGTH] ;
  threshold_control_struc thr_cont ;
} tctythr ;

static void write_q660_cont (pq660 q660)
begin
  tcont_cache *pcc ;
  tfile_handle cf ;
  string fname ;

  strcpy(fname, "cont/q660.cnt") ;
  if (lib_file_open (q660->par_create.file_owner, fname, LFO_CREATE or LFO_WRITE, addr(cf)))
    then
      begin
        q660->media_error = TRUE ;
        return ;
      end
    else
      q660->media_error = FALSE ;
  pcc = q660->conthead ;
  while (pcc)
    begin
      lib_file_write (q660->par_create.file_owner, cf, pcc->payload, pcc->size) ;
      pcc = pcc->next ;
    end
  lib_file_close (q660->par_create.file_owner, cf) ;
  q660->q660_cont_written = now () ;
  q660->q660cont_updated = FALSE ; /* disk now has latest */
end

void save_thread_continuity (pq660 q660)
begin
  tctydplcq *pdldest ;
  pstatic pstat ;
  plcq q ;
  tfile_handle cf ;
  string fname ;
  integer good, value ;

  if (q660->q660cont_updated)
    then
      write_q660_cont (q660) ; /* write cache to disk */
  strcpy(fname, "cont/stn.cnt") ;
  if (lib_file_open (q660->par_create.file_owner, fname, LFO_CREATE or LFO_WRITE, addr(cf)))
    then
      return ;
  pstat = (pointer)q660->cbuf ;
  pstat->id = CTY_STATIC ;
  pstat->size = sizeof(tstatic) ;
  pstat->version = CT_VER ;
  memcpy(addr(pstat->net), addr(q660->network), sizeof(tseed_net)) ;
  memcpy(addr(pstat->stn), addr(q660->station), sizeof(tseed_stn)) ;
  pstat->spare = 0 ;
  pstat->spare1 = FALSE ;
  lock (q660) ;
  pstat->time_written = lib_round(now()) ;
  pstat->stat_minutes = q660->share.stat_minutes ;
  pstat->stat_hours = q660->share.stat_hours ;
  pstat->total_minutes = q660->share.total_minutes ;
  pstat->timetag_save = q660->saved_data_timetag ;
  pstat->last_status_save = q660->last_status_received ;
  pstat->tag_save = 0 ;
  if (q660->share.sysinfo.prop_tag[0])
    then
      begin
        good = sscanf (q660->share.sysinfo.prop_tag, "%d", addr(value)) ;
        if (good == 1)
          then
            pstat->tag_save = value ;
      end
  memcpy(addr(pstat->sn_save), addr(q660->par_create.q660id_serial), sizeof(t64)) ;
  pstat->reboot_save = q660->share.sysinfo.boot ;
  memcpy(addr(pstat->opstat), addr(q660->share.opstat), sizeof(topstat)) ;
  memcpy(addr(pstat->accmstats), addr(q660->share.accmstats), sizeof(taccmstats)) ;
  unlock (q660) ;
  pstat->crc = gcrccalc ((pointer)((pntrint)pstat + 4), sizeof(tstatic) - 4) ;
  lib_file_write (q660->par_create.file_owner, cf, pstat, sizeof(tstatic)) ;
  q = q660->dplcqs ;
  while (q)
    begin
      pdldest = (pointer)q660->cbuf ;
      if (q->validated)
        then
          begin /* wasn't removed by new tokens */
            pdldest->id = CTY_DPLCQ ;
            pdldest->size = sizeof(tctydplcq) ;
            memcpy(addr(pdldest->loc), addr(q->location), sizeof(tlocation)) ;
            memcpy(addr(pdldest->name), addr(q->seedname), sizeof(tseed_name)) ; ;
            pdldest->lpad = 0 ;
            pdldest->dp_src = q->sub_field ;
            pdldest->gap_thresh = q->gap_threshold ;
            pdldest->frame_limit = q->com->maxframes ;
            pdldest->rec_written = q->com->records_written ;
            pdldest->arec_written = q->arc.records_written ;
            pdldest->last_sample = q->com->last_sample ;
            pdldest->nextrec_tag = q->backup_tag ;
            pdldest->lastrec_tag = q->last_timetag ;
            pdldest->crc = gcrccalc ((pointer)((pntrint)pdldest + 4), pdldest->size - 4) ;
            lib_file_write (q660->par_create.file_owner, cf, pdldest, pdldest->size) ;
          end
      q = q->link ;
    end
  lib_file_close (q660->par_create.file_owner, cf) ;
end

/* This is only called once to preload the cache */
static boolean read_q660_cont (pq660 q660)
begin
  tcont_cache *pcc, *last ;
  tfile_handle cf ;
  string fname ;
  tctyhdr hdr ;
  integer next, loops, sz ;
  pbyte p ;
  boolean good ;

  last = NIL ;
  good = FALSE ;
  strcpy(fname, "cont/q660.cnt") ;
  if (lib_file_open (q660->par_create.file_owner, fname, LFO_OPEN or LFO_READ, addr(cf)))
    then
      begin
        q660->media_error = TRUE ;
        return good ;
      end
    else
      q660->media_error = FALSE ;
  loops = 0 ;
  next = 0 ;
  repeat
    if (lib_file_read (q660->par_create.file_owner, cf, addr(hdr), sizeof(tctyhdr)))
      then
        break ;
    sz = hdr.size ;
    next = next + sz ; /* next record */
    getbuf (addr(q660->thrmem), (pvoid)addr(pcc), sz + sizeof(tcont_cache)) ;
    if (last)
      then
        last->next = pcc ;
      else
        q660->conthead = pcc ;
    last = pcc ;
    pcc->size = sz ;
    pcc->allocsize = sz ; /* initially both of these are the same */
    pcc->next = NIL ;
    pcc->payload = (pointer)((pntrint)pcc + sizeof(tcont_cache)) ; /* skip cache header */
    p = pcc->payload ;
    memcpy (p, addr(hdr), sizeof(tctyhdr)) ; /* copy header in */
    incn(p, sizeof(tctyhdr)) ; /* skip part we already read */
    if (lib_file_read (q660->par_create.file_owner, cf, p, sz - sizeof(tctyhdr)))
      then
        break ;
    if (hdr.crc != gcrccalc ((pointer)((pntrint)pcc->payload + 4), sz - 4))
      then
        begin
          libmsgadd (q660, LIBMSG_CONCRC, "Q660") ;
          lib_file_close (q660->par_create.file_owner, cf) ;
          return good ;
        end
      else
        good = TRUE ; /* good CRC found */
    lib_file_seek (q660->par_create.file_owner, cf, next) ;
    inc(loops) ;
  until (loops > 9999)) ;
  lib_file_close (q660->par_create.file_owner, cf) ;
  q660->q660cont_updated = FALSE ; /* Now have latest from disk */
  q660->q660_cont_written = now () ; /* start timing here */
  return good ;
end

static void q660cont_write (pq660 q660, pointer buf, integer size)
begin
  tcont_cache *pcc, *best, *bestlast, *last ;
  integer diff, bestdiff ;

  bestdiff = 99999999 ;
  best = NIL ;
  last = NIL ;
  bestlast = NIL ;
  pcc = q660->contfree ;
  while (pcc)
    begin
      diff = pcc->allocsize - size ;
      if (diff == 0)
        then
          break ; /* exact match */
      else if ((diff > 0) land (diff < bestdiff))
        then
          begin /* best fit so far */
            best = pcc ;
            bestlast = last ;
            bestdiff = diff ;
          end
      last = pcc ;
      pcc = pcc->next ;
    end
  if ((pcc == NIL) land (best))
    then
      begin /* use the best fit */
        pcc = best ;
        last = bestlast ;
      end
  if (pcc)
    then
      begin /* found a free buffer to use */
        if (last)
          then /* middle of the pack */
            last->next = pcc->next ; /* skip over me */
          else
            q660->contfree = pcc->next ; /* this was first packet */
      end
    else
      begin /* must allocate a new one */
        getbuf (addr(q660->thrmem), (pvoid)addr(pcc), size + sizeof(tcont_cache)) ;
        pcc->size = size ;
        pcc->allocsize = size ;
        pcc->payload = (pointer)((pntrint)pcc + sizeof(tcont_cache)) ; /* skip cache header */
      end
  pcc->next = NIL ; /* new end of list */
  if (q660->contlast)
    then
      q660->contlast->next = pcc ; /* extend list */
    else
      q660->conthead = pcc ; /* first in list */
  q660->contlast = pcc ;
  pcc->size = size ; /* this may be smaller than allocsize */
  memcpy (pcc->payload, buf, size) ; /* now in linked list */
end

void save_continuity (pq660 q660)
begin
  tsystem *psystem ;
  tctylcq *pldest ;
  tctyiir *pdest ;
  piirfilter psrc ;
  tctyfir *pfdest ;
  tctyring *prdest ;
//  tctymh *pmdest ;
  tctythr *ptdest ;
  pcompressed_buffer_ring pr ;
  pdet_packet pdp ;
//  con_sto *pmc ;
  threshold_control_struc *pmt ;
  integer points ;
  plcq q ;
  integer i ;
  string s ;
  string31 s1 ;
  tcont_cache *freec ;
  string fname ;

  freec = q660->contfree ;
  if (freec)
    then
      begin /* append active list to end of free chain */
        while (freec->next)
          freec = freec->next ;
        freec->next = q660->conthead ;
      end
    else /* just xfer active to free */
      q660->contfree = q660->conthead ;
  q660->conthead = NIL ; /* no active entries */
  q660->contlast = NIL ; /* no last segment */
  libmsgadd (q660, LIBMSG_WRCONT, "Q660") ;
  psystem = (pointer)q660->cbuf ;
  psystem->id = CTY_SYSTEM ;
  psystem->size = sizeof(tsystem) ;
  psystem->version = CT_VER ;
  memcpy(addr(psystem->serial), addr(q660->par_create.q660id_serial), sizeof(t64)) ;
  psystem->lasttime = q660->data_timetag ;
  psystem->last_dataqual = q660->data_qual ;
  psystem->last_dataseq = q660->dt_data_sequence ;
  strcpy(fname, "cont/q660.cnt") ;
  sprintf(s, "%s", jul_string(lib_round(q660->lasttime), s1));
  libmsgadd (q660, LIBMSG_CSAVE, s) ;
  lock (q660) ;
  psystem->reboot_counter = q660->share.sysinfo.reboots ;
  unlock (q660) ;
  psystem->crc = gcrccalc ((pointer)((pntrint)psystem + 4), sizeof(tsystem) - 4) ;
  q660cont_write (q660, psystem, sizeof(tsystem)) ;
  q = q660->lcqs ;
  while (q)
    begin
      pldest = (pointer)q660->cbuf ;
      pfdest = (pointer)q660->cbuf ;
      prdest = (pointer)q660->cbuf ;
//      pmdest = (pointer)q660->cbuf ;
      ptdest = (pointer)q660->cbuf ;
      pldest->id = CTY_LCQ ;
      pldest->size = sizeof(tctylcq) ;
      memcpy(addr(pldest->loc), addr(q->location), sizeof(tlocation)) ;
      memcpy(addr(pldest->name), addr(q->seedname), sizeof(tseed_name)) ;
      pldest->lpad = 0 ;
      pldest->lastdtsequence = q->dtsequence ;
      pldest->prev_rate = q->rate ;
      pldest->prev_delay = q->delay ;
      pldest->glast = q->gen_last_on ;
      pldest->con = q->cal_on ;
      pldest->cstat = q->calstat ;
      pldest->cinc = q->calinc ;
      pldest->qpad = 0 ;
      pldest->rec_written = q->com->records_written ;
      pldest->arec_written = q->arc.records_written ;
      pldest->gnext = q->gen_next ;
      pldest->last_sample = q->com->last_sample ;
      pldest->nextrec_tag =q-> backup_tag ;
      pldest->lastrec_tag = q->last_timetag ;
      pldest->overwrite_slipping = q->slipping ;
      pldest->backup_timetag = q->backup_tag ;
      pldest->backup_timequal = q->backup_qual ;
      pldest->crc = gcrccalc ((pointer)((pntrint)pldest + 4), pldest->size - 4) ;
      q660cont_write (q660, pldest, pldest->size) ;
      if ((q->fir) land (q->source_fir))
        then
          begin
            pfdest->id = CTY_FIR ;
            pfdest->size = sizeof(tctyfir) - sizeof(tfloat) ;
            memcpy(addr(pfdest->loc), addr(q->location), sizeof(tlocation)) ;
            memcpy(addr(pfdest->name), addr(q->seedname), sizeof(tseed_name)) ;
            strcpy(pfdest->fn, q->source_fir->fname) ;
            pfdest->lpad = 0 ;
            pfdest->fcnt = q->fir->fcount ;
            pfdest->foff = (longint)((pntrint)q->fir->f - (pntrint)q->fir->fbuf) ;
            memcpy (addr(pfdest->fbuffer), q->fir->fbuf, q->fir->flen * sizeof (tfloat)) ;
            pfdest->size = pfdest->size + sizeof(tfloat) * q->fir->flen ;
            pfdest->crc = gcrccalc ((pointer)((pntrint)pfdest + 4), pfdest->size - 4) ;
            q660cont_write (q660, pfdest, pfdest->size) ;
          end
      psrc = q->stream_iir ;
      if (q->rate > 0)
        then
          points = q->rate ;
        else
          points = 1 ;
      while (psrc)
        begin
          pdest = (pointer)q660->cbuf ;
          pdest->id = CTY_IIR ;
          pdest->size = sizeof(tctyiir) - sizeof(tfloat) ; /* not counting any output buffer yet */
          memcpy(addr(pdest->loc), addr(q->location), sizeof(tlocation)) ;
          memcpy(addr(pdest->name), addr(q->seedname), sizeof(tseed_name)) ;
          strcpy(pdest->fn, psrc->def->fname) ;
          for (i = 1 ; i <= psrc->sects ; i++)
            begin
              memcpy(addr(pdest->flt[i].x), addr(psrc->filt[i].x), sizeof(tvector)) ;
              memcpy(addr(pdest->flt[i].y), addr(psrc->filt[i].y), sizeof(tvector)) ;
            end
          memcpy (addr(pdest->outbuf), addr(psrc->out), sizeof(tfloat) * points) ;
          pdest->size = pdest->size + sizeof(tfloat) * points ;
          pdest->crc = gcrccalc ((pointer)((pntrint)pdest + 4), pdest->size - 4) ;
          q660cont_write (q660, pdest, pdest->size) ;
          psrc = psrc->link ;
        end
      pdp = q->det ;
      while (pdp)
        begin
          if (pdp->detector_def->dtype == STALTA)
            then
              begin
#ifdef asdfadsfasdff
                pmdest->id = CTY_MH ;
                pmdest->size = sizeof(tctymh) ;
                memcpy(addr(pmdest->loc), addr(q->location), sizeof(tlocation)) ;
                memcpy(addr(pmdest->name), addr(q->seedname), sizeof(tseed_name)) ;
                strcpy(addr(pmdest->dn), addr(pdp->detector_def->detname)) ;
                pmdest->lpad = 0 ;
                pmc = pdp->cont ;
                memcpy (addr(pmdest->mh_cont), pmc, sizeof(con_sto)) ;
                if (pdp->insamps_size)
                  then
                    begin /* append contents of insamps array for low frequency stuff */
                      pmdest->size = pmdest->size + pdp->insamps_size ;
                      pl = (pointer)((pntrint)pmdest + sizeof(tctymh)) ;
                      memcpy (pl, pdp->insamps, pdp->insamps_size) ;
                    end
                pmdest->crc = gcrccalc ((pointer)((pntrint)pmdest + 4), pmdest->size - 4) ;
                q660cont_write (q660, pmdest, pmdest->size) ;
#endif
              end
            else
              begin
                ptdest->id = CTY_THR ;
                ptdest->size = sizeof(tctythr) ;
                memcpy(addr(ptdest->loc), addr(q->location), sizeof(tlocation)) ;
                memcpy(addr(ptdest->name), addr(q->seedname), sizeof(tseed_name)) ;
                strcpy(ptdest->dn, pdp->detector_def->detname) ;
                ptdest->lpad = 0 ;
                pmt = pdp->cont ;
                memcpy (addr(ptdest->thr_cont), pmt, sizeof(threshold_control_struc)) ;
                ptdest->crc = gcrccalc ((pointer)((pntrint)ptdest + 4), ptdest->size - 4) ;
                q660cont_write (q660, ptdest, ptdest->size) ;
              end
          pdp = pdp->link ;
        end
      if (q->pre_event_buffers > 0)
        then
          begin
            pr = q->com->ring->link ;
            while (pr != q->com->ring)
              begin
                if (pr->full)
                  then
                    begin
                      prdest->id = CTY_RING ;
                      prdest->size = sizeof(tctyring) ;
                      memcpy(addr(prdest->loc), addr(q->location), sizeof(tlocation)) ;
                      memcpy(addr(prdest->name), addr(q->seedname), sizeof(tseed_name)) ;
                      prdest->lpad = 0 ;
                      prdest->spare = 0 ;
                      memcpy (addr(prdest->comprec), addr(pr->rec), LIB_REC_SIZE) ;
                      prdest->crc = gcrccalc ((pointer)((pntrint)prdest + 4), prdest->size - 4) ;
                      q660cont_write (q660, prdest, prdest->size) ;
                    end
                pr = pr->link ;
              end
          end
      q = q->link ;
    end
/* flush to disk if appropriate */
  if ((now() - q660->q660_cont_written) > ((integer)q660->par_register.opt_q660_cont * 60.0))
    then
      write_q660_cont (q660) ;
    else
      q660->q660cont_updated = TRUE ; /* updated and not on disk */
end

static void process_status (pq660 q660, tstatic *statstor) /* static is reserved word in C, change name */
begin
#define MINS_PER_DAY 1440
  longint tdiff, useable, new_total ;
  enum tlogfld acctype ;
  integer hour, dhour, uhours, thours, min, dmin, umins, tmins ;
  taccmstat *paccm ;

  min = lib_round(now()) ;
  tdiff = (min - statstor->time_written) div 60 ; /* get minutes difference */
  if ((tdiff >= MINS_PER_DAY) lor (tdiff < 0))
    then
      return ; /* a day old or newer than now, can't use anything */
  new_total = statstor->total_minutes + tdiff ; /* new status time period */
  if (new_total > MINS_PER_DAY)
    then
      new_total = MINS_PER_DAY ;
  useable = new_total - tdiff ;
  if (useable <= 0)
    then
      return ; /* don't have enough data to fill the gap */
  /* clear out accumulators */
  for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
    begin
      paccm = addr(statstor->accmstats[acctype]) ;
      paccm->accum = 0 ;
      paccm->accum_ds = 0 ;
      paccm->ds_lcq = NIL ;
    end
  /* initialize communications efficiency to INVALID_ENTRY */
  for (hour = 0 ; hour <= 23 ; hour++)
    q660->share.accmstats[LOGF_COMMEFF].hours[hour] = INVALID_ENTRY ;
  for (min = 0 ; min <= 59 ; min++)
    q660->share.accmstats[LOGF_COMMEFF].minutes[min] = INVALID_ENTRY ;
  if (useable > MINS_PER_DAY)
    then
      useable = MINS_PER_DAY ;
  thours = new_total div 60 ; /* total hours */
  tmins = new_total - (thours * 60) ;
  q660->share.total_minutes = new_total ;
  q660->share.stat_minutes = tmins ;
  q660->share.stat_hours = thours ;
  if (q660->share.stat_hours > 23)
    then
      q660->share.stat_hours = 0 ; /* wrap the day */
  uhours = useable div 60 ; /* useable old hours */
  umins = useable - (uhours * 60) ;
  if (uhours > 0)
    then
      begin /* have at least an hour, copy the hours plus the entire minutes */
        /* transfer in hour entries */
        hour = statstor->stat_hours - 1 ; /* last useable data */
        dhour = thours - 1 ; /* new last useable data */
        while (uhours > 0)
          begin
            if (hour < 0)
              then
                hour = 23 ;
            if (dhour < 0)
              then
                dhour = 23 ;
            for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
              q660->share.accmstats[acctype].hours[dhour] = statstor->accmstats[acctype].hours[hour] ;
            dec(uhours) ;
            dec(hour) ;
            dec(dhour) ;
          end
        for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
          memcpy(addr(q660->share.accmstats[acctype].minutes), addr(statstor->accmstats[acctype].minutes), sizeof(taccminutes)) ;
      end
    else
      begin
        /* transfer in minutes entries */
        min = statstor->stat_minutes - 1 ;
        dmin = tmins - 1 ; /* new last useable */
        while (umins > 0)
          begin
            if (min < 0)
              then
                min = 59 ;
            if (dmin < 0)
              then
                dmin = 59 ;
            for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
              q660->share.accmstats[acctype].minutes[dmin] = statstor->accmstats[acctype].minutes[min] ;
            dec(umins) ;
            dec(min) ;
            dec(dmin) ;
          end
      end
  memcpy(addr(q660->station_ident), addr(statstor->opstat.station_name), sizeof(string9)) ;
end

void set_loc_name (plcq q)
begin
  integer i, lth ;
  string3 s ;

  lth = 0 ;
  for (i = 0 ; i <= 1 ; i++)
    if (q->location[i] != ' ')
      then
        s[lth++] = q->location[i] ;
  s[lth] = 0 ;
  strcpy(q->slocation, s) ;
  lth = 0 ;
  for (i = 0 ; i <= 2 ; i++)
    if (q->seedname[i] != ' ')
      then
        s[lth++] = q->seedname[i] ;
  s[lth] = 0 ;
  strcpy(q->sseedname, s) ;
end

void build_fake_log_lcq (pq660 q660, boolean done)
begin
  plcq cur_lcq ;

  getbuf (addr(q660->thrmem), (pvoid)addr(cur_lcq), sizeof(tlcq)) ;
  if (q660->dplcqs == NIL)
    then
      q660->dplcqs = cur_lcq ;
    else
      q660->dplcqs = extend_link (q660->dplcqs, cur_lcq) ;
  string2fixed (addr(cur_lcq->location), "  ") ;
  string2fixed (addr(cur_lcq->seedname), "LOG") ;
  set_loc_name (cur_lcq) ;
  cur_lcq->validated = TRUE ; /* unless new tokens remove it */
  cur_lcq->gen_src = GDS_LOG ;
  cur_lcq->sub_field = (byte)LOGF_MSGS ;
  cur_lcq->lcq_num = 0xFF ; /* flag as not indexed */
  cur_lcq->gap_threshold = 0.5 ;
  getbuf (addr(q660->thrmem), (pvoid)addr(cur_lcq->com), sizeof(tcom_packet)) ;
  cur_lcq->com->frame = 1 ;
  cur_lcq->com->next_compressed_sample = 1 ;
  cur_lcq->com->maxframes = 255 ;
  cur_lcq->arc.records_written = 0 ;
  q660->msg_lcq = cur_lcq ;
  if (done)
    then
      init_dplcqs (q660) ;
end

void restore_thread_continuity (pq660 q660, boolean pass1, pchar result)
begin
  tstatic statstor ;
  tfile_handle cf ;
  integer loops, next ;
  tctydplcq *pdlsrc ;
  plcq cur_lcq ;
  string fname ;

  if (result)
   then
     result[0] = 0 ;
  if ((lnot pass1) land (q660->dplcqs))
    then
      return ; /* we already did this */
  strcpy(fname, "cont/stn.cnt") ;
  if (lib_file_open (q660->par_create.file_owner, fname, LFO_OPEN or LFO_READ, addr(cf)))
    then
      begin
        q660->media_error = TRUE ;
        if (lnot pass1)
          then
            build_fake_log_lcq (q660, TRUE) ;
        return ;
      end
    else
      q660->media_error = FALSE ;
  if (pass1)
    then
      begin
        lib_file_read (q660->par_create.file_owner, cf, (pvoid)addr(statstor), sizeof(tstatic)) ;
        lib_file_close (q660->par_create.file_owner, cf) ;
        if (statstor.id != CTY_STATIC)
          then
            begin
              if (result)
                then
                  strcpy(result, "Thread Continuity Format Mis-match") ;
              lib_file_delete (q660->par_create.file_owner, fname) ;
              return ;
            end
        if (statstor.version != CT_VER)
          then
            begin
              if (result)
                then
                  sprintf(result, "Thread Continuity Version Mis-match, Got=%d, Expected=%d", statstor.version, CT_VER) ;
              lib_file_delete (q660->par_create.file_owner, fname) ;
              return ;
            end
        if (statstor.crc != gcrccalc ((pointer)((pntrint)addr(statstor) + 4), sizeof(tstatic) - 4))
          then
            begin
              if (result)
                then
                  strcpy(result, "Thread Continuity CRC Error, Ignoring rest of file") ;
              lib_file_delete (q660->par_create.file_owner, fname) ;
              return ;
            end
        memcpy(addr(q660->network), addr(statstor.net), sizeof(tseed_net)) ;
        memcpy(addr(q660->station), addr(statstor.stn), sizeof(tseed_stn)) ;
        q660->saved_data_timetag = statstor.timetag_save ;
        q660->last_status_received = statstor.last_status_save ;
        sprintf (q660->share.sysinfo.prop_tag, "%d", (integer)statstor.tag_save) ;
//        memcpy(addr(q660->share.fixed.sys_num), addr(statstor.sn_save), sizeof(t64)) ;
        q660->share.sysinfo.boot = statstor.reboot_save ;
        process_status (q660, addr(statstor)) ;
      end
    else
      begin
        lib_file_seek (q660->par_create.file_owner, cf, sizeof(tstatic)) ; /* skip this */
        loops = 0 ;
        next = sizeof(tstatic) ;
        repeat
          if (lib_file_read (q660->par_create.file_owner, cf, q660->cbuf, sizeof(tctydplcq)))
            then
              break ;
          pdlsrc = (pointer)q660->cbuf ;
          next = next + pdlsrc->size ; /* next record */
          if (pdlsrc->crc != gcrccalc ((pointer)((pntrint)pdlsrc + 4), pdlsrc->size - 4))
            then
              begin
                libmsgadd (q660, LIBMSG_CONCRC, "Thread") ;
                lib_file_close (q660->par_create.file_owner, cf) ;
                if (q660->msg_lcq == NIL)
                  then
                    build_fake_log_lcq (q660, TRUE) ;
                return ;
              end
          if (pdlsrc->id != CTY_DPLCQ)
            then
              begin
                libmsgadd (q660, LIBMSG_CONPURGE, "Thread") ;
                lib_file_close (q660->par_create.file_owner, cf) ;
                if (q660->msg_lcq == NIL)
                  then
                    build_fake_log_lcq (q660, TRUE) ;
                return ;
              end

#ifdef OMITADMINCHANNELSONIDL
          if (strstr(q660->par_register.q660id_address, "@IDL")) {
               /* skip administrative DP channels on IDL */
               switch ((enum tlogfld) pdlsrc->dp_src) 
               {
                 case LOGF_MSGS :
                   break ;
                 case LOGF_CFG :
                   break ;
                 case LOGF_TIME :
                   break ;
                 default :
                   inc(loops) ;
                   continue;
               }
          }
#endif              
              
          getbuf (addr(q660->thrmem), (pvoid)addr(cur_lcq), sizeof(tlcq)) ;
          if (q660->dplcqs == NIL)
            then
              q660->dplcqs = cur_lcq ;
            else
              q660->dplcqs = extend_link (q660->dplcqs, cur_lcq) ;
          memcpy(addr(cur_lcq->location), addr(pdlsrc->loc), sizeof(tlocation)) ;
          memcpy(addr(cur_lcq->seedname), addr(pdlsrc->name), sizeof(tseed_name)) ;
          set_loc_name (cur_lcq) ;
          cur_lcq->validated = TRUE ; /* unless new tokens remove it */
          cur_lcq->gen_src = GDS_LOG  ;
          cur_lcq->sub_field = pdlsrc->dp_src ;
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
          cur_lcq->lcq_num = 0xFF ; /* flag as not indexed */
          cur_lcq->gap_threshold = pdlsrc->gap_thresh ;
          if (cur_lcq->gap_threshold == 0.0)
            then
              cur_lcq->gap_threshold = 0.5 ;
          cur_lcq->gap_secs = (1 + cur_lcq->gap_threshold) * abs(cur_lcq->rate) ; /* will always be at least a multiple of the rate */
          cur_lcq->firfixing_gain = 1.000 ; /* default if not over-ridden */
          getbuf (addr(q660->thrmem), (pvoid)addr(cur_lcq->com), sizeof(tcom_packet)) ;
          cur_lcq->com->frame = 1 ;
          cur_lcq->com->next_compressed_sample = 1 ;
          cur_lcq->com->maxframes = pdlsrc->frame_limit ;
          cur_lcq->com->records_written = pdlsrc->rec_written ;
          cur_lcq->com->last_sample = pdlsrc->last_sample ;
          cur_lcq->backup_tag = pdlsrc->nextrec_tag ;
          cur_lcq->last_timetag = 0 ; /* Expecting a gap, don't report */
          cur_lcq->arc.records_written = pdlsrc->arec_written ;
          lib_file_seek (q660->par_create.file_owner, cf, next) ;
          inc(loops) ;
        until (loops > 9999)) ;
        lib_file_close (q660->par_create.file_owner, cf) ;
        if (q660->msg_lcq == NIL)
          then
            build_fake_log_lcq (q660, FALSE) ;
        init_dplcqs (q660) ;
      end
end

void check_continuity (pq660 q660)
begin
  tsystem system ;
  longword newreboots ;
  string s ;
  string31 s1 ;
  tcont_cache *pcc ;

  if (q660->conthead == NIL)
    then
      begin
        if (lnot read_q660_cont (q660))
          then
            return ; /* didn't find it */
      end
  pcc = q660->conthead ;
  memcpy (addr(system), pcc->payload, sizeof(tsystem)) ;
  if (system.version != CT_VER)
    then
      begin
        sprintf(s, "Version Mis-match, Got=%d, Expected=%d", system.version, CT_VER) ;
        libmsgadd (q660, LIBMSG_CONTIN, s) ;
        return ;
      end
  if (system.crc != gcrccalc ((pointer)((pntrint)addr(system) + 4), sizeof(tsystem) - 4))
    then
      begin
        libmsgadd (q660, LIBMSG_CONCRC, "Q660") ;
        return ;
      end
  if (system.id != CTY_SYSTEM)
    then
      begin
        libmsgadd (q660, LIBMSG_CONPURGE, "Q660") ;
        return ;
      end
  if ((system.serial[0] != q660->par_create.q660id_serial[0]) lor (system.serial[1] != q660->par_create.q660id_serial[1]))
    then
      return ;
  lock (q660) ;
  newreboots = q660->share.sysinfo.reboots ;
  unlock (q660) ;
  if (system.reboot_counter != newreboots)
    then
      begin
        sprintf(s, "%d Q660 Reboot(s)", (integer)(newreboots - system.reboot_counter)) ;
        libmsgadd (q660, LIBMSG_CONTBOOT, s) ;
        add_status (q660, LOGF_REBOOTS, newreboots - system.reboot_counter) ;
        return ;
      end
  q660->data_qual = system.last_dataqual ;
  q660->dt_data_sequence = system.last_dataseq ;
  q660->data_timetag = system.lasttime ;
  sprintf(s, "%s Q=%d", jul_string(lib_round(system.lasttime), s1), system.last_dataqual) ;
  libdatamsg (q660, LIBMSG_CONTFND, s) ;
end

boolean restore_continuity (pq660 q660)
begin
  integer loops ;
  plcq q ;
  tsystem *psystem ;
  tctylcq *plsrc ;
  tctyiir *psrc ;
  tctyfir *pfsrc ;
  tctyring *prsrc ;
  piirfilter pdest ;
//  tctymh *pmsrc ;
  tctythr *ptsrc ;
  pdet_packet pdp ;
//  con_sto *pmc ;
  threshold_control_struc *pmt ;
  integer points ;
  integer i ;
  tcont_cache *pcc ;

  psystem = (pointer)q660->cbuf ;
  pcc = q660->conthead ;
  if (pcc == NIL)
    then
      return FALSE ;
  memcpy (psystem, pcc->payload, sizeof(tsystem)) ;
  pcc = pcc->next ;
  loops = 0 ;
  plsrc = (pointer)q660->cbuf ;
  while ((pcc) land (loops < 9999))
    begin
      memcpy (q660->cbuf, pcc->payload, pcc->size) ;
      pcc = pcc->next ;
      plsrc = (pointer)q660->cbuf ;
      psrc = (pointer)q660->cbuf ;
      pfsrc = (pointer)q660->cbuf ;
      prsrc = (pointer)q660->cbuf ;
//      pmsrc = (pointer)q660->cbuf ;
      ptsrc = (pointer)q660->cbuf ;
      switch (plsrc->id) begin
        case CTY_IIR :
          q = q660->lcqs ;
          pdest = NIL ;
          while (q)
            if ((memcmp(addr(q->location), addr(psrc->loc), 2) == 0) land
                (memcmp(addr(q->seedname), addr(psrc->name), 3) == 0))
              then
                begin
                  pdest = q->stream_iir ;
                  break ;
                end
              else
                q = q->link ;
          while (pdest)
            if (strcmp(pdest->def->fname, psrc->fn) == 0)
              then
                begin
                  if (q->rate > 0)
                    then
                      points = q->rate ;
                    else
                      points = 1 ;
                  for (i = 1 ; i <= pdest->sects ; i++)
                    begin
                      memcpy(addr(pdest->filt[i].x), addr(psrc->flt[i].x), sizeof(tvector)) ;
                      memcpy(addr(pdest->filt[i].y), addr(psrc->flt[i].y), sizeof(tvector)) ;
                    end
                  memcpy (addr(pdest->out), addr(psrc->outbuf), sizeof(tfloat) * points) ;
                  break ;
                end
              else
                pdest = pdest->link ;
          break ;
        case CTY_FIR :
          q = q660->lcqs ;
          while (q)
            if ((memcmp(addr(q->location), addr(pfsrc->loc), 2) == 0) land
                (memcmp(addr(q->seedname), addr(pfsrc->name), 3) == 0) land
                (q->source_fir) land (strcmp(q->source_fir->fname, pfsrc->fn) == 0))
              then
                begin
                  q->fir->fcount = pfsrc->fcnt ;
                  q->fir->f = (pointer)((pntrint)q->fir->fbuf + pfsrc->foff) ;
                  memcpy (q->fir->fbuf, addr(pfsrc->fbuffer), q->fir->flen * sizeof(tfloat)) ;
                  q->com->charging = FALSE ; /* not any more */
                  break ;
                end
              else
                q = q->link ;
          break ;
        case CTY_LCQ :
          q = q660->lcqs ;
          while (q)
            if ((memcmp(addr(q->location), addr(plsrc->loc), 2) == 0) land
                (memcmp(addr(q->seedname), addr(plsrc->name), 3) == 0))
              then
                begin
                  if (q->rate == 0)
                    then
                      begin /* use backup values */
                        q->rate = plsrc->prev_rate ;
                        q->delay = plsrc->prev_delay ;
                      end
                  q->dtsequence = plsrc->lastdtsequence ;
                  q->gen_last_on = plsrc->glast ;
                  q->cal_on = plsrc->con ;
                  q->calstat = plsrc->cstat ;
                  q->calinc = plsrc->cinc ;
                  q->com->records_written = plsrc->rec_written ;
                  q->arc.records_written = plsrc->arec_written ;
                  q->gen_next = plsrc->gnext ;
                  q->com->last_sample = plsrc->last_sample ;
                  q->backup_tag = plsrc->nextrec_tag ;
                  q->last_timetag = plsrc->lastrec_tag ;
                  q->slipping = plsrc->overwrite_slipping ;
                  q->backup_tag = plsrc->backup_timetag ;
                  q->backup_qual = plsrc->backup_timequal ;
                  break ;
                end
              else
                q = q->link ;
          break ;
        case CTY_RING :
          q = q660->lcqs ;
          while (q)
            if ((memcmp(addr(q->location), addr(prsrc->loc), 2) == 0) land
                (memcmp(addr(q->seedname), addr(prsrc->name), 3) == 0))
              then
                begin
                  q->com->ring->full = TRUE ;
                  memcpy (addr(q->com->ring->rec), addr(prsrc->comprec), LIB_REC_SIZE) ;
                  q->com->last_in_ring = q->com->ring ; /* last one we filled */
                  q->com->ring = q->com->ring->link ;
                  break ;
                end
              else
                q = q->link ;
          break ;
#ifdef asdfasdfadsfadsf
        case CTY_SL :
          q = q660->lcqs ;
          pdp = NIL ;
          while (q)
            if ((memcmp(addr(q->location), addr(pmsrc->loc), 2) == 0) land
                (memcmp(addr(q->seedname), addr(pmsrc->name), 3) == 0))
              then
                begin
                  pdp = q->det ;
                  break ;
                end
              else
                q = q->link ;
          while (pdp)
            if (strcmp(addr(pdp->detector_def->detname), addr(pmsrc->dn)) == 0)
              then
                begin
                  pmc = pdp->cont ;
                  memcpy (pmc, addr(pmsrc->mh_cont), (pntrint)addr(pmc->onsetdata) - (pntrint)pmc) ;
                  if ((pdp->insamps) land (pmsrc->size > sizeof(tctymh)))
                    then
                      begin
                        pl = (pointer)((pntrint)pmsrc + sizeof(tctymh)) ;
                        memcpy (pdp->insamps, pl, pdp->insamps_size) ;
                      end
                  if ((pmc->default_enabled) != (pdp->det_options and DO_RUN))
                    then
                      begin
                        pmc->default_enabled = (pdp->det_options and DO_RUN) ;
                        pmc->detector_enabled = (pdp->det_options and DO_RUN) ;
                      end
                  break ;
                end
              else
                pdp = pdp->link ;
          break ;
#endif
        case CTY_THR :
          q = q660->lcqs ;
          pdp = NIL ;
          while (q)
            if ((memcmp(addr(q->location), addr(ptsrc->loc), 2) == 0) land
                (memcmp(addr(q->seedname), addr(ptsrc->name), 3) == 0))
              then
                begin
                  pdp = q->det ;
                  break ;
                end
              else
                q = q->link ;
          while (pdp)
            if (strcmp(pdp->detector_def->detname, ptsrc->dn) == 0)
              then
                begin
                  pmt = pdp->cont ;
                  memcpy (pmt, addr(ptsrc->thr_cont), (pntrint)addr(pmt->onsetdata) - (pntrint)pmt) ;
#ifdef dasfadsf
                  if ((pmt->default_enabled) != (pdp->det_options and DO_RUN))
                    then
                      begin
                        pmt->default_enabled = (pdp->det_options and DO_RUN) ;
                        pmt->detector_enabled = (pdp->det_options and DO_RUN) ;
                      end
#endif
                  break ;
                end
              else
                pdp = pdp->link ;
          break ;
      end
      inc(loops) ;
    end
  return TRUE ;
end

void purge_continuity (pq660 q660)
begin
  tsystem *psystem ;
  tcont_cache *pcc ;

  psystem = (pointer)q660->cbuf ;
  pcc = q660->conthead ;
  if (pcc)
    then
      begin
        memcpy(psystem, pcc->payload, sizeof(tsystem)) ; /* get existing system record */
        if (psystem->id != CTY_SYSTEM)
          then
            return ; /* already done? */
        psystem->id = CTY_PURGED ;
        psystem->crc = gcrccalc ((pointer)((pntrint)psystem + 4), sizeof(tsystem) - 4) ;
        memcpy(pcc->payload, psystem, sizeof(tsystem)) ; /* secretly replace with Folgers Crystals */
      end
end

/* follows immediately after 2nd pass at restoring thread continuity */
void purge_thread_continuity (pq660 q660)
begin
  tctydplcq *pdlsrc ;
  tfile_handle cf ;
  string fname ;

  strcpy(fname, "cont/stn.cnt") ;
  if (q660->media_error)
    then
      return ; /* can't initialize media */
  if (lib_file_open (q660->par_create.file_owner, fname, LFO_OPEN or LFO_READ or LFO_WRITE, addr(cf)))
    then
      return ;
  pdlsrc = (pointer)q660->cbuf ;
  lib_file_seek (q660->par_create.file_owner, cf, sizeof(tstatic)) ; /* skip this */
  if (lib_file_read (q660->par_create.file_owner, cf, pdlsrc, sizeof(tctydplcq)))
    then
      begin
        lib_file_close (q660->par_create.file_owner, cf) ;
        return ;
      end
  pdlsrc->id = CTY_PURGED ;
  pdlsrc->crc = gcrccalc ((pointer)((pntrint)pdlsrc + 4), sizeof(tctydplcq) - 4) ;
  lib_file_seek (q660->par_create.file_owner, cf, sizeof(tstatic)) ;
  lib_file_write (q660->par_create.file_owner, cf, pdlsrc, sizeof(tctydplcq)) ;
  lib_file_close (q660->par_create.file_owner, cf) ;
end

void continuity_timer (pq660 q660)
begin

  if ((q660->q660cont_updated) land
      ((now() - q660->q660_cont_written) > ((integer)q660->par_register.opt_q660_cont * 60.0)))
    then
      write_q660_cont (q660) ;
end

