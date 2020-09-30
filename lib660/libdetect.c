/*   Lib660 Detector Routines
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
*/
#include "libdetect.h"
#include "libseed.h"
#include "libsupport.h"

/* this is a little weird because it was originally a nested procedure
   within te_detect and had to be moved outside of te_detect */
static void gettime (boolean high, pthreshold_control_struc ptcs,
                     tonset_base *pob, tdet_packet *pdet)
begin
  longint adj ;

  ptcs->new_onset = TRUE ;
  memset(pob, 0, sizeof(tonset_mh)) ;
  if (high)
    then
      begin
        pob->signal_amplitude = ptcs->peakhi ;
        pob->background_estimate = ptcs->parent->ucon.filhi ;
      end
    else
      begin
        pob->signal_amplitude = ptcs->peaklo ;
        pob->background_estimate = ptcs->parent->ucon.fillo ;
      end
  adj = pdet->sam_no - (ptcs->parent->ucon.n_hits - 1) ;
  pob->signal_onset_time.seed_fpt = ptcs->etime + adj / ptcs->sample_rate ;
end

boolean Te_detect (tdet_packet *detector)
begin
  pdataarray indatar ;
  psinglearray realdata ;
  boolean realflag ;
  longint in_data ;
  pthreshold_control_struc tcs_ptr ;
  tdetload *pdl ;

  tcs_ptr = detector->cont ;
  indatar = detector->indatar ;
  realdata = (pointer)indatar ;
  realflag = detector->singleflag ;
  pdl = addr(tcs_ptr->parent->ucon) ;
  tcs_ptr->new_onset = FALSE ;
  if (lnot detector->remaining)
    then
      begin
        detector->sam_ch = detector->datapts ;
        tcs_ptr->etime = tcs_ptr->startt ;
        detector->sam_no = 0 ;
      end
  while (detector->sam_no < detector->sam_ch)
    begin
      if (realflag)
        then
          in_data = lib_round((*realdata)[detector->sam_no]) ;
        else
          in_data = (*indatar)[detector->sam_no] ;
      if (in_data > tcs_ptr->peakhi)
        then
          tcs_ptr->peakhi = in_data ;
      if (in_data < tcs_ptr->peaklo)
        then
          tcs_ptr->peaklo = in_data ;
      if (in_data > pdl->filhi)
        then
          begin
            if (lnot tcs_ptr->hevon)
              then
                begin
                  inc(tcs_ptr->overhi) ;
                  if (tcs_ptr->overhi >= pdl->n_hits)
                    then
                      begin
                        tcs_ptr->hevon = TRUE ;
                        gettime (TRUE, tcs_ptr, tcs_ptr->onsetdata, detector) ;
                        inc(detector->sam_no) ;
                        break ;
                      end
                end
            tcs_ptr->waitdly = 0 ;
          end
      else if ((tcs_ptr->hevon) land (in_data < pdl->filhi - pdl->iwin))
        then
          begin
            inc(tcs_ptr->waitdly) ;
            if (tcs_ptr->waitdly > pdl->wait_blk)
              then
                begin
                  tcs_ptr->peakhi = -MAXLINT ;
                  tcs_ptr->overhi = 0 ;
                  tcs_ptr->hevon = FALSE ;
                end
          end
      if (in_data < pdl->fillo)
        then
          begin
            if (lnot tcs_ptr->levon)
              then
                begin
                  inc(tcs_ptr->overlo) ;
                  if (tcs_ptr->overlo >= pdl->n_hits)
                    then
                      begin
                        tcs_ptr->levon = TRUE ;
                        gettime (FALSE, tcs_ptr, tcs_ptr->onsetdata, detector) ;
                        inc(detector->sam_no) ;
                        break ;
                      end
                end
            tcs_ptr->waitdly = 0 ;
          end
      else if ((tcs_ptr->levon) land (in_data > pdl->fillo + pdl->iwin))
        then
          begin
            inc(tcs_ptr->waitdly) ;
            if (tcs_ptr->waitdly > pdl->wait_blk)
              then
                begin
                  tcs_ptr->peaklo = MAXLINT ;
                  tcs_ptr->overlo = 0 ;
                  tcs_ptr->levon = FALSE ;
                end
          end
      inc(detector->sam_no) ;
    end
  detector->remaining = (detector->sam_no < detector->sam_ch) ;
  return (tcs_ptr->hevon lor tcs_ptr->levon) ;
end

static void Tcs_setup (tdet_packet *detector)
begin
  pthreshold_control_struc tcs_ptr ;

  tcs_ptr = detector->cont ;
  memset(tcs_ptr, 0, sizeof(threshold_control_struc)) ;
  detector->remaining = FALSE ;
  tcs_ptr->first_detection = FALSE ;
  tcs_ptr->detector_enabled = TRUE ;
  tcs_ptr->default_enabled = TRUE ;
  tcs_ptr->peaklo = MAXLINT ;
  tcs_ptr->peakhi = -MAXLINT ;
  tcs_ptr->onsetdata = (tonset_base *)addr(detector->onset) ;
  tcs_ptr->parent = detector ;
  tcs_ptr->sample_rate = detector->samrte ;
end

void initialize_detector (pq660 q660, pdet_packet pdp, piirfilter pi)
begin
  boolean sl ;
  integer i, j ;
  pthreshold_control_struc pt ;
  plcq q ;
  pdetect pdef ;

  pdef = pdp->detector_def ;
  q = pdp->parent ;
  sl = (pdef->dtype == STALTA) ;
  if (q->gen_src == GDS_DEC)
    then
      begin
        pdp->indatar = (pvoid)addr(q->processed_stream) ;
        pdp->singleflag = TRUE ;
      end
    else
      begin
        pdp->indatar = (pvoid)q->databuf ;
        pdp->singleflag = FALSE ;
      end
  if (sl)
    then
      begin
        if (q->rate > 0)
          then
            begin
              pdp->samrte = q->rate ;
              if (q->rate >= MINPOINTS)
                then
                  begin
                    pdp->datapts = q->rate ;
                    if (q->gen_src == GDS_DEC)
                      then
                        pdp->grpsize = 1 ; /* processed data */
                      else
                        pdp->grpsize = 0 ; /*nonbuffered*/
                  end
                else
                  begin
                    if (q->gen_src == GDS_DEC)
                      then
                        pdp->grpsize = 1  ; /* processed data */
                      else
                        pdp->grpsize = q->rate ;
                    i = MINPOINTS ;
                    j = i div pdp->grpsize ;
                    if ((pdp->grpsize * j) != i) /*doesn't go evenly*/
                      then
                        i = pdp->grpsize * (j + 1) ;
                    pdp->datapts = i ;
                  end
            end
          else
            begin
              pdp->samrte = 1.0 / abs(q->rate) ;
              pdp->datapts = MINPOINTS ;
              pdp->grpsize = 1 ;
            end
        if (pdp->grpsize)
          then
            begin /*buffered*/
              if (pi)
                then
                  pdp->insamps_size = pdp->datapts * sizeof(tfloat) ;
                else
                  pdp->insamps_size = pdp->datapts * sizeof(longint) ;
              getbuf (addr(q660->connmem), (pointer)addr(pdp->insamps), pdp->insamps_size) ;
            end
          else
            pdp->insamps = NIL ;
      end
    else
      begin
        if (q->rate > 0)
          then
            begin
              pdp->datapts = q->rate ;
              pdp->samrte = q->rate ;
            end
          else
            begin
              pdp->datapts = 1 ;
              pdp->samrte = 1.0 / abs(q->rate) ;
            end
        pdp->insamps = NIL ;
      end
  if (sl)
    then
      begin
#ifdef adsfadsfadsf
        getbuf (q660, addr(pmh), sizeof(con_sto)) ;
        pdp->cont = pmh ;
        pdp->cont_size = sizeof(con_sto) ;
        Cont_setup (pdp);
        memcpy(addr(pmh->parent->ucon), addr(pdef->uconst), sizeof(tdetload)) ;
#endif
      end
    else
      begin
        getbuf (addr(q660->connmem), (pvoid)addr(pt), sizeof(threshold_control_struc)) ;
        pdp->cont = pt ;
        pdp->cont_size = sizeof(threshold_control_struc) ;
        Tcs_setup (pdp) ;
        memcpy(addr(pt->parent->ucon), addr(pdef->uconst), sizeof(tdetload)) ;
      end
  if (pi)
    then
      begin
        pdp->indatar = (pvoid)addr(pi->out) ;
        pdp->singleflag = TRUE ;
      end
end

enum tliberr lib_detstat (pq660 q660, tdetstat *detstat)
begin
  plcq q ;
  pdet_packet pd ;
  con_common *pcc ;
  tonedetstat *pone ;
  string15 s ;

  detstat->count = 0 ;
  if (q660->libstate != LIBSTATE_RUN)
    then
      return LIBERR_NOSTAT ;
  q = q660->lcqs ;
  while ((detstat->count < MAX_DETSTAT) land (q))
    begin
      pd = q->det ;
      while ((detstat->count < MAX_DETSTAT) land (pd != NIL))
        begin
          pcc = pd->cont ;
          pone = addr(detstat->entries[detstat->count]) ;
          sprintf(pone->name, "%s:%s", seed2string(q->location, q->seedname, s),
                  pd->detector_def->detname) ;
          pone->ison = pcc->detector_on ;
          pone->declared = pcc->detection_declared ;
          pone->first = pcc->first_detection ;
          pone->enabled = pcc->detector_enabled ;
          inc(detstat->count) ;
          pd = pd->link ;
        end
      q = q->link ;
    end
  return LIBERR_NOERR ;
end

void lib_changeenable (pq660 q660, tdetchange *detchange)
begin
  plcq q ;
  pdet_packet pd ;
  con_common *pcc ;
  char s[DETECTOR_NAME_LENGTH + 11] ;

  if (q660->libstate != LIBSTATE_RUN)
    then
      return ;
  q = q660->lcqs ;
  while (q)
    begin
      pd = q->det ;
      while (pd)
        begin
          pcc = pd->cont ;
          sprintf(s, "%s:%s", seed2string(q->location, q->seedname, s),
                  pd->detector_def->detname) ;
          if (strcmp(detchange->name, s) == 0)
            then
              begin
                pcc->detector_enabled = detchange->run_detector ;
                return ;
              end
          pd = pd->link ;
        end
      q = q->link ;
    end
end


