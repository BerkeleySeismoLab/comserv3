



/*   Lib660 Detector Routines
     Copyright 2017 by
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
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libdetect.h"
#include "libseed.h"
#include "libsupport.h"

/* this is a little weird because it was originally a nested procedure
   within te_detect and had to be moved outside of te_detect */
static void gettime (BOOLEAN high, pthreshold_control_struc ptcs,
                     tonset_base *pob, tdet_packet *pdet)
{
    I32 adj ;

    ptcs->new_onset = TRUE ;
    memset(pob, 0, sizeof(tonset_mh)) ;

    if (high) {
        pob->signal_amplitude = ptcs->peakhi ;
        pob->background_estimate = ptcs->parent->ucon.filhi ;
    } else {
        pob->signal_amplitude = ptcs->peaklo ;
        pob->background_estimate = ptcs->parent->ucon.fillo ;
    }

    adj = pdet->sam_no - (ptcs->parent->ucon.n_hits - 1) ;
    pob->signal_onset_time.seed_fpt = ptcs->etime + adj / ptcs->sample_rate ;
}

BOOLEAN Te_detect (tdet_packet *detector)
{
    pdataarray indatar ;
    psinglearray realdata ;
    BOOLEAN realflag ;
    I32 in_data ;
    pthreshold_control_struc tcs_ptr ;
    tdetload *pdl ;

    tcs_ptr = detector->cont ;
    indatar = detector->indatar ;
    realdata = (pointer)indatar ;
    realflag = detector->singleflag ;
    pdl = &(tcs_ptr->parent->ucon) ;
    tcs_ptr->new_onset = FALSE ;

    if (! detector->remaining) {
        detector->sam_ch = detector->datapts ;
        tcs_ptr->etime = tcs_ptr->startt ;
        detector->sam_no = 0 ;
    }

    while (detector->sam_no < detector->sam_ch) {
        if (realflag)
            in_data = lib_round((*realdata)[detector->sam_no]) ;
        else
            in_data = (*indatar)[detector->sam_no] ;

        if (in_data > tcs_ptr->peakhi)
            tcs_ptr->peakhi = in_data ;

        if (in_data < tcs_ptr->peaklo)
            tcs_ptr->peaklo = in_data ;

        if (in_data > pdl->filhi) {
            if (! tcs_ptr->hevon) {
                (tcs_ptr->overhi)++ ;

                if (tcs_ptr->overhi >= pdl->n_hits) {
                    tcs_ptr->hevon = TRUE ;
                    gettime (TRUE, tcs_ptr, tcs_ptr->onsetdata, detector) ;
                    (detector->sam_no)++ ;
                    break ;
                }
            }

            tcs_ptr->waitdly = 0 ;
        } else if ((tcs_ptr->hevon) && (in_data < pdl->filhi - pdl->iwin)) {
            (tcs_ptr->waitdly)++ ;

            if (tcs_ptr->waitdly > pdl->wait_blk) {
                tcs_ptr->peakhi = -MAXLINT ;
                tcs_ptr->overhi = 0 ;
                tcs_ptr->hevon = FALSE ;
            }
        }

        if (in_data < pdl->fillo) {
            if (! tcs_ptr->levon) {
                (tcs_ptr->overlo)++ ;

                if (tcs_ptr->overlo >= pdl->n_hits) {
                    tcs_ptr->levon = TRUE ;
                    gettime (FALSE, tcs_ptr, tcs_ptr->onsetdata, detector) ;
                    (detector->sam_no)++ ;
                    break ;
                }
            }

            tcs_ptr->waitdly = 0 ;
        } else if ((tcs_ptr->levon) && (in_data > pdl->fillo + pdl->iwin)) {
            (tcs_ptr->waitdly)++ ;

            if (tcs_ptr->waitdly > pdl->wait_blk) {
                tcs_ptr->peaklo = MAXLINT ;
                tcs_ptr->overlo = 0 ;
                tcs_ptr->levon = FALSE ;
            }
        }

        (detector->sam_no)++ ;
    }

    detector->remaining = (detector->sam_no < detector->sam_ch) ;
    return (tcs_ptr->hevon || tcs_ptr->levon) ;
}

static void Tcs_setup (tdet_packet *detector)
{
    pthreshold_control_struc tcs_ptr ;

    tcs_ptr = detector->cont ;
    memset(tcs_ptr, 0, sizeof(threshold_control_struc)) ;
    detector->remaining = FALSE ;
    tcs_ptr->first_detection = FALSE ;
    tcs_ptr->detector_enabled = TRUE ;
    tcs_ptr->default_enabled = TRUE ;
    tcs_ptr->peaklo = MAXLINT ;
    tcs_ptr->peakhi = -MAXLINT ;
    tcs_ptr->onsetdata = (tonset_base *)&(detector->onset) ;
    tcs_ptr->parent = detector ;
    tcs_ptr->sample_rate = detector->samrte ;
}

void initialize_detector (pq660 q660, pdet_packet pdp, piirfilter pi)
{
    BOOLEAN sl ;
    int i, j ;
    pthreshold_control_struc pt ;
    plcq q ;
    pdetect pdef ;

    pdef = pdp->detector_def ;
    q = pdp->parent ;
    sl = (pdef->dtype == STALTA) ;

    if (q->gen_src == GDS_DEC) {
        pdp->indatar = (pvoid)&(q->processed_stream) ;
        pdp->singleflag = TRUE ;
    } else {
        pdp->indatar = (pvoid)q->databuf ;
        pdp->singleflag = FALSE ;
    }

    if (sl) {
        if (q->rate > 0) {
            pdp->samrte = q->rate ;

            if (q->rate >= MINPOINTS) {
                pdp->datapts = q->rate ;

                if (q->gen_src == GDS_DEC)
                    pdp->grpsize = 1 ; /* processed data */
                else
                    pdp->grpsize = 0 ; /*nonbuffered*/
            } else {
                if (q->gen_src == GDS_DEC)
                    pdp->grpsize = 1  ; /* processed data */
                else
                    pdp->grpsize = q->rate ;

                i = MINPOINTS ;
                j = i / pdp->grpsize ;

                if ((pdp->grpsize * j) != i) /*doesn't go evenly*/
                    i = pdp->grpsize * (j + 1) ;

                pdp->datapts = i ;
            }
        } else {
            pdp->samrte = 1.0 / abs(q->rate) ;
            pdp->datapts = MINPOINTS ;
            pdp->grpsize = 1 ;
        }

        if (pdp->grpsize) {
            /*buffered*/
            if (pi)
                pdp->insamps_size = pdp->datapts * sizeof(tfloat) ;
            else
                pdp->insamps_size = pdp->datapts * sizeof(I32) ;

            getbuf (&(q660->connmem), (pointer)&(pdp->insamps), pdp->insamps_size) ;
        } else
            pdp->insamps = NIL ;
    } else {
        if (q->rate > 0) {
            pdp->datapts = q->rate ;
            pdp->samrte = q->rate ;
        } else {
            pdp->datapts = 1 ;
            pdp->samrte = 1.0 / abs(q->rate) ;
        }

        pdp->insamps = NIL ;
    }

    if (sl) {
#ifdef adsfadsfadsf
        getbuf (q660, &(pmh), sizeof(con_sto)) ;
        pdp->cont = pmh ;
        pdp->cont_size = sizeof(con_sto) ;
        Cont_setup (pdp);
        memcpy(&(pmh->parent->ucon), &(pdef->uconst), sizeof(tdetload)) ;
#endif
    } else {
        getbuf (&(q660->connmem), (pvoid)&(pt), sizeof(threshold_control_struc)) ;
        pdp->cont = pt ;
        pdp->cont_size = sizeof(threshold_control_struc) ;
        Tcs_setup (pdp) ;
        memcpy(&(pt->parent->ucon), &(pdef->uconst), sizeof(tdetload)) ;
    }

    if (pi) {
        pdp->indatar = (pvoid)&(pi->out) ;
        pdp->singleflag = TRUE ;
    }
}

enum tliberr lib_detstat (pq660 q660, tdetstat *detstat)
{
    plcq q ;
    pdet_packet pd ;
    con_common *pcc ;
    tonedetstat *pone ;
    string15 s ;

    detstat->count = 0 ;

    if (q660->libstate != LIBSTATE_RUN)
        return LIBERR_NOSTAT ;

    q = q660->lcqs ;

    while ((detstat->count < MAX_DETSTAT) && (q)) {
        pd = q->det ;

        while ((detstat->count < MAX_DETSTAT) && (pd != NIL)) {
            pcc = pd->cont ;
            pone = &(detstat->entries[detstat->count]) ;
            sprintf(pone->name, "%s:%s", seed2string(q->location, q->seedname, s),
                    pd->detector_def->detname) ;
            pone->ison = pcc->detector_on ;
            pone->declared = pcc->detection_declared ;
            pone->first = pcc->first_detection ;
            pone->enabled = pcc->detector_enabled ;
            (detstat->count)++ ;
            pd = pd->link ;
        }

        q = q->link ;
    }

    return LIBERR_NOERR ;
}

void lib_changeenable (pq660 q660, tdetchange *detchange)
{
    plcq q ;
    pdet_packet pd ;
    con_common *pcc ;
    char s[DETECTOR_NAME_LENGTH + 11] ;

    if (q660->libstate != LIBSTATE_RUN)
        return ;

    q = q660->lcqs ;

    while (q) {
        pd = q->det ;

        while (pd) {
            pcc = pd->cont ;
            sprintf(s, "%s:%s", seed2string(q->location, q->seedname, s),
                    pd->detector_def->detname) ;

            if (strcmp(detchange->name, s) == 0) {
                pcc->detector_enabled = detchange->run_detector ;
                return ;
            }

            pd = pd->link ;
        }

        q = q->link ;
    }
}


