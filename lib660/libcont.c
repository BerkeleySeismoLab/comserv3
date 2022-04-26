



/*   Lib660 Continuity Routines
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
    1 2021-01-06 jms omit admin DP channels on IDL
    2 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
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

#define OMITADMINCHANNELSONIDL

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

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
} tctyhdr ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    U8 version ;
    tseed_net net ;
    tseed_stn stn ; /* these make sure nothing tricky has happened */
    BOOLEAN spare1 ; /* was timezone adjustment from continuity file */
    I32 spare ; /* was timezone offset */
    int mem_required ; /* amount of memory required */
    int thrmem_required ; /* amount of thread memory required */
    I32 time_written ; /* seconds since 2016 that this was written */
    int stat_minutes ; /* minutes worth of info will go in this slot */
    int stat_hours ; /* hours worth will go into this slot */
    I32 total_minutes ; /* total minutes accumulated */
    double timetag_save ; /* for the purposes of calculating data latency */
    double last_status_save ; /* for the purposes of calculating status latency */
    I32 tag_save ; /* tagid save */
    t64 sn_save ; /* serial number */
    U32 reboot_save ; /* reboot time save */
    topstat opstat ; /* snapshot of operational status */
    taccmstats accmstats ; /* snapshot of opstat generation information */
} tstatic ;
typedef tstatic *pstatic ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    U8 version ;
    U8 high_freq ; /* high frequency encoding */
    t64 serial ; /* serial number of q660 */
    double lasttime ; /* data_timetag of last second of data */
    U16 last_dataqual ; /* 0-100% */
    U32 last_dataseq ; /* data record sequence. sequence continuity important too */
    U32 reboot_counter ; /* last reboot counter */
} tsystem ;

typedef struct
{
    tvector x ;
    tvector y ;
} tiirvalues ;
typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    char fn[FILTER_NAME_LENGTH] ;
    tiirvalues flt[MAXSECTIONS + 1] ; /* element 0 not used */
    tfloat outbuf ; /* this may be an array */
} tctyiir ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    U16 fcnt ; /* current number of samples in fir buffer */
    I32 foff ; /* offset from start of buffer to "f" pointer */
    char fn[FILTER_NAME_LENGTH] ;
    tfloat fbuffer ; /* this is an array */
} tctyfir ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    U32 lastdtsequence ; /* last data record sequence processed */
    int prev_rate ; /* rate when continuity written */
    tfloat prev_delay ; /* delay when continuity written */
    BOOLEAN glast ; /* gen_last_on */
    BOOLEAN con ; /* cal_on */
    BOOLEAN cstat ; /* calstat */
    BOOLEAN overwrite_slipping ;
    U8 qpad ;
    U16 cinc ; /* calinc */
    I32 rec_written ; /* records written */
    I32 arec_written ; /* archive records written */
    U32 gnext ; /* gen_next */
    I32 last_sample ; /* last compression sample */
    double nextrec_tag ; /* this is the expected starting time of the next record */
    double lastrec_tag ; /* this was the last timetag used */
    double backup_timetag ;
    U16 backup_timequal ;
} tctylcq ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    U8 dp_src ; /* selects among the DP Statistics */
    U32 lcq_options ;
    int frame_limit ;
    float gap_thresh ;
    I32 rec_written ; /* records written */
    I32 arec_written ; /* archive records written */
    I32 last_sample ; /* last compression sample */
    double nextrec_tag ; /* this is the expected starting time of the next record */
    double lastrec_tag ; /* this was the last timetag used */
} tctydplcq ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    U16 spare ; /* for longword alignment */
    completed_record comprec ;
} tctyring ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    char dn[DETECTOR_NAME_LENGTH] ;
    /*  con_sto mh_cont ; */
} tctymh ;

typedef struct
{
    I32 crc ; /* CRC of everything following */
    U16 id ; /* what kind of entry */
    U16 size ; /* size of this entry */
    tlocation loc ;
    tseed_name name ;
    U8 lpad ;
    char dn[DETECTOR_NAME_LENGTH] ;
    threshold_control_struc thr_cont ;
} tctythr ;

static void write_q660_cont (pq660 q660)
{
    tcont_cache *pcc ;
    tfile_handle cf ;
    string fname ;

    strcpy(fname, "cont/q660.cnt") ;

    if (lib_file_open (q660->par_create.file_owner, fname, LFO_CREATE | LFO_WRITE, &(cf))) {
        q660->media_error = TRUE ;
        return ;
    } else
        q660->media_error = FALSE ;

    pcc = q660->conthead ;

    while (pcc) {
        lib_file_write (q660->par_create.file_owner, cf, pcc->payload, pcc->size) ;
        pcc = pcc->next ;
    }

    lib_file_close (q660->par_create.file_owner, cf) ;
    q660->q660_cont_written = now () ;
    q660->q660cont_updated = FALSE ; /* disk now has latest */
}

void save_thread_continuity (pq660 q660)
{
    tctydplcq *pdldest ;
    pstatic pstat ;
    plcq q ;
    tfile_handle cf ;
    string fname ;
    int good, value ;

    if (q660->q660cont_updated)
        write_q660_cont (q660) ; /* write cache to disk */

    strcpy(fname, "cont/stn.cnt") ;

    if (lib_file_open (q660->par_create.file_owner, fname, LFO_CREATE | LFO_WRITE, &(cf)))
        return ;

    pstat = (pointer)q660->cbuf ;
    pstat->id = CTY_STATIC ;
    pstat->size = sizeof(tstatic) ;
    pstat->version = CT_VER ;
    memcpy(&(pstat->net), &(q660->network), sizeof(tseed_net)) ;
    memcpy(&(pstat->stn), &(q660->station), sizeof(tseed_stn)) ;
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

    if (q660->share.sysinfo.prop_tag[0]) {
        good = sscanf (q660->share.sysinfo.prop_tag, "%d", &(value)) ;

        if (good == 1)
            pstat->tag_save = value ;
    }

    memcpy(&(pstat->sn_save), &(q660->par_create.q660id_serial), sizeof(t64)) ;
    pstat->reboot_save = q660->share.sysinfo.boot ;
    memcpy(&(pstat->opstat), &(q660->share.opstat), sizeof(topstat)) ;
    memcpy(&(pstat->accmstats), &(q660->share.accmstats), sizeof(taccmstats)) ;
    unlock (q660) ;
    pstat->crc = gcrccalc ((pointer)((PNTRINT)pstat + 4), sizeof(tstatic) - 4) ;
    lib_file_write (q660->par_create.file_owner, cf, pstat, sizeof(tstatic)) ;
    q = q660->dplcqs ;

    while (q) {
        pdldest = (pointer)q660->cbuf ;

        if (q->validated) {
            /* wasn't removed by new tokens */
            pdldest->id = CTY_DPLCQ ;
            pdldest->size = sizeof(tctydplcq) ;
            memcpy(&(pdldest->loc), &(q->location), sizeof(tlocation)) ;
            memcpy(&(pdldest->name), &(q->seedname), sizeof(tseed_name)) ; ;
            pdldest->lpad = 0 ;
            pdldest->dp_src = q->sub_field ;
            pdldest->gap_thresh = q->gap_threshold ;
            pdldest->frame_limit = q->com->maxframes ;
            pdldest->rec_written = q->com->records_written ;
            pdldest->arec_written = q->arc.records_written ;
            pdldest->last_sample = q->com->last_sample ;
            pdldest->nextrec_tag = q->backup_tag ;
            pdldest->lastrec_tag = q->last_timetag ;
            pdldest->crc = gcrccalc ((pointer)((PNTRINT)pdldest + 4), pdldest->size - 4) ;
            lib_file_write (q660->par_create.file_owner, cf, pdldest, pdldest->size) ;
        }

        q = q->link ;
    }

    lib_file_close (q660->par_create.file_owner, cf) ;
}

/* This is only called once to preload the cache */
static BOOLEAN read_q660_cont (pq660 q660)
{
    tcont_cache *pcc, *last ;
    tfile_handle cf ;
    string fname ;
    tctyhdr hdr ;
    int next, loops, sz ;
    PU8 p ;
    BOOLEAN good ;

    last = NIL ;
    good = FALSE ;
    strcpy(fname, "cont/q660.cnt") ;

    if (lib_file_open (q660->par_create.file_owner, fname, LFO_OPEN | LFO_READ, &(cf))) {
        q660->media_error = TRUE ;
        return good ;
    } else
        q660->media_error = FALSE ;

    loops = 0 ;
    next = 0 ;

    do {
        if (lib_file_read (q660->par_create.file_owner, cf, &(hdr), sizeof(tctyhdr)))
            break ;

        sz = hdr.size ;
        next = next + sz ; /* next record */
        getbuf (&(q660->thrmem), (pvoid)&(pcc), sz + sizeof(tcont_cache)) ;

        if (last)
            last->next = pcc ;
        else
            q660->conthead = pcc ;

        last = pcc ;
        pcc->size = sz ;
        pcc->allocsize = sz ; /* initially both of these are the same */
        pcc->next = NIL ;
        pcc->payload = (pointer)((PNTRINT)pcc + sizeof(tcont_cache)) ; /* skip cache header */
        p = pcc->payload ;
        memcpy (p, &(hdr), sizeof(tctyhdr)) ; /* copy header in */
        p = p + (sizeof(tctyhdr)) ; /* skip part we already read */

        if (lib_file_read (q660->par_create.file_owner, cf, p, sz - sizeof(tctyhdr)))
            break ;

        if (hdr.crc != gcrccalc ((pointer)((PNTRINT)pcc->payload + 4), sz - 4)) {
            libmsgadd (q660, LIBMSG_CONCRC, "Q660") ;
            lib_file_close (q660->par_create.file_owner, cf) ;
            return good ;
        } else
            good = TRUE ; /* good CRC found */

        lib_file_seek (q660->par_create.file_owner, cf, next) ;
        (loops)++ ;
    } while (! (loops > 9999)) ;

    lib_file_close (q660->par_create.file_owner, cf) ;
    q660->q660cont_updated = FALSE ; /* Now have latest from disk */
    q660->q660_cont_written = now () ; /* start timing here */
    return good ;
}

static void q660cont_write (pq660 q660, pointer buf, int size)
{
    tcont_cache *pcc, *best, *bestlast, *last ;
    int diff, bestdiff ;

    bestdiff = 99999999 ;
    best = NIL ;
    last = NIL ;
    bestlast = NIL ;
    pcc = q660->contfree ;

    while (pcc) {
        diff = pcc->allocsize - size ;

        if (diff == 0)
            break ; /* exact match */
        else if ((diff > 0) && (diff < bestdiff)) {
            /* best fit so far */
            best = pcc ;
            bestlast = last ;
            bestdiff = diff ;
        }

        last = pcc ;
        pcc = pcc->next ;
    }

    if ((pcc == NIL) && (best)) {
        /* use the best fit */
        pcc = best ;
        last = bestlast ;
    }

    if (pcc) {
        /* found a free buffer to use */
        if (last)
            /* middle of the pack */
            last->next = pcc->next ; /* skip over me */
        else
            q660->contfree = pcc->next ; /* this was first packet */
    } else {
        /* must allocate a new one */
        getbuf (&(q660->thrmem), (pvoid)&(pcc), size + sizeof(tcont_cache)) ;
        pcc->size = size ;
        pcc->allocsize = size ;
        pcc->payload = (pointer)((PNTRINT)pcc + sizeof(tcont_cache)) ; /* skip cache header */
    }

    pcc->next = NIL ; /* new end of list */

    if (q660->contlast)
        q660->contlast->next = pcc ; /* extend list */
    else
        q660->conthead = pcc ; /* first in list */

    q660->contlast = pcc ;
    pcc->size = size ; /* this may be smaller than allocsize */
    memcpy (pcc->payload, buf, size) ; /* now in linked list */
}

void save_continuity (pq660 q660)
{
    tsystem *psystem ;
    tctylcq *pldest ;
    tctyiir *pdest ;
    piirfilter psrc ;
    tctyfir *pfdest ;
    tctyring *prdest ;
    /*  tctymh *pmdest ; */
    tctythr *ptdest ;
    pcompressed_buffer_ring pr ;
    pdet_packet pdp ;
    /*  con_sto *pmc ; */
    threshold_control_struc *pmt ;
    int points ;
    plcq q ;
    int i ;
    string s ;
    string31 s1 ;
    tcont_cache *freec ;
    string fname ;

    freec = q660->contfree ;

    if (freec) {
        /* append active list to end of free chain */
        while (freec->next)
            freec = freec->next ;

        freec->next = q660->conthead ;
    } else /* just xfer active to free */
        q660->contfree = q660->conthead ;

    q660->conthead = NIL ; /* no active entries */
    q660->contlast = NIL ; /* no last segment */
    libmsgadd (q660, LIBMSG_WRCONT, "Q660") ;
    psystem = (pointer)q660->cbuf ;
    psystem->id = CTY_SYSTEM ;
    psystem->size = sizeof(tsystem) ;
    psystem->version = CT_VER ;
    memcpy(&(psystem->serial), &(q660->par_create.q660id_serial), sizeof(t64)) ;
    psystem->lasttime = q660->data_timetag ;
    psystem->last_dataqual = q660->data_qual ;
    psystem->last_dataseq = q660->dt_data_sequence ;
    strcpy(fname, "cont/q660.cnt") ;
    sprintf(s, "%s", jul_string(lib_round(q660->lasttime), s1));
    libmsgadd (q660, LIBMSG_CSAVE, s) ;
    lock (q660) ;
    psystem->reboot_counter = q660->share.sysinfo.reboots ;
    unlock (q660) ;
    psystem->crc = gcrccalc ((pointer)((PNTRINT)psystem + 4), sizeof(tsystem) - 4) ;
    q660cont_write (q660, psystem, sizeof(tsystem)) ;
    q = q660->lcqs ;

    while (q) {
        pldest = (pointer)q660->cbuf ;
        pfdest = (pointer)q660->cbuf ;
        prdest = (pointer)q660->cbuf ;
        /*      pmdest = (pointer)q660->cbuf ; */
        ptdest = (pointer)q660->cbuf ;
        pldest->id = CTY_LCQ ;
        pldest->size = sizeof(tctylcq) ;
        memcpy(&(pldest->loc), &(q->location), sizeof(tlocation)) ;
        memcpy(&(pldest->name), &(q->seedname), sizeof(tseed_name)) ;
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
        pldest->crc = gcrccalc ((pointer)((PNTRINT)pldest + 4), pldest->size - 4) ;
        q660cont_write (q660, pldest, pldest->size) ;

        if ((q->fir) && (q->source_fir)) {
            pfdest->id = CTY_FIR ;
            pfdest->size = sizeof(tctyfir) - sizeof(tfloat) ;
            memcpy(&(pfdest->loc), &(q->location), sizeof(tlocation)) ;
            memcpy(&(pfdest->name), &(q->seedname), sizeof(tseed_name)) ;
            strcpy(pfdest->fn, q->source_fir->fname) ;
            pfdest->lpad = 0 ;
            pfdest->fcnt = q->fir->fcount ;
            pfdest->foff = (I32)((PNTRINT)q->fir->f - (PNTRINT)q->fir->fbuf) ;
            memcpy (&(pfdest->fbuffer), q->fir->fbuf, q->fir->flen * sizeof (tfloat)) ;
            pfdest->size = pfdest->size + sizeof(tfloat) * q->fir->flen ;
            pfdest->crc = gcrccalc ((pointer)((PNTRINT)pfdest + 4), pfdest->size - 4) ;
            q660cont_write (q660, pfdest, pfdest->size) ;
        }

        psrc = q->stream_iir ;

        if (q->rate > 0)
            points = q->rate ;
        else
            points = 1 ;

        while (psrc) {
            pdest = (pointer)q660->cbuf ;
            pdest->id = CTY_IIR ;
            pdest->size = sizeof(tctyiir) - sizeof(tfloat) ; /* not counting any output buffer yet */
            memcpy(&(pdest->loc), &(q->location), sizeof(tlocation)) ;
            memcpy(&(pdest->name), &(q->seedname), sizeof(tseed_name)) ;
            strcpy(pdest->fn, psrc->def->fname) ;

            for (i = 1 ; i <= psrc->sects ; i++) {
                memcpy(&(pdest->flt[i].x), &(psrc->filt[i].x), sizeof(tvector)) ;
                memcpy(&(pdest->flt[i].y), &(psrc->filt[i].y), sizeof(tvector)) ;
            }

            memcpy (&(pdest->outbuf), &(psrc->out), sizeof(tfloat) * points) ;
            pdest->size = pdest->size + sizeof(tfloat) * points ;
            pdest->crc = gcrccalc ((pointer)((PNTRINT)pdest + 4), pdest->size - 4) ;
            q660cont_write (q660, pdest, pdest->size) ;
            psrc = psrc->link ;
        }

        pdp = q->det ;

        while (pdp) {
            if (pdp->detector_def->dtype == STALTA) {
#ifdef asdfadsfasdff
                pmdest->id = CTY_MH ;
                pmdest->size = sizeof(tctymh) ;
                memcpy(&(pmdest->loc), &(q->location), sizeof(tlocation)) ;
                memcpy(&(pmdest->name), &(q->seedname), sizeof(tseed_name)) ;
                strcpy(&(pmdest->dn), &(pdp->detector_def->detname)) ;
                pmdest->lpad = 0 ;
                pmc = pdp->cont ;
                memcpy (&(pmdest->mh_cont), pmc, sizeof(con_sto)) ;

                if (pdp->insamps_size) {
                    /* append contents of insamps array for low frequency stuff */
                    pmdest->size = pmdest->size + pdp->insamps_size ;
                    pl = (pointer)((PNTRINT)pmdest + sizeof(tctymh)) ;
                    memcpy (pl, pdp->insamps, pdp->insamps_size) ;
                }

                pmdest->crc = gcrccalc ((pointer)((PNTRINT)pmdest + 4), pmdest->size - 4) ;
                q660cont_write (q660, pmdest, pmdest->size) ;
#endif
            } else {
                ptdest->id = CTY_THR ;
                ptdest->size = sizeof(tctythr) ;
                memcpy(&(ptdest->loc), &(q->location), sizeof(tlocation)) ;
                memcpy(&(ptdest->name), &(q->seedname), sizeof(tseed_name)) ;
                strcpy(ptdest->dn, pdp->detector_def->detname) ;
                ptdest->lpad = 0 ;
                pmt = pdp->cont ;
                memcpy (&(ptdest->thr_cont), pmt, sizeof(threshold_control_struc)) ;
                ptdest->crc = gcrccalc ((pointer)((PNTRINT)ptdest + 4), ptdest->size - 4) ;
                q660cont_write (q660, ptdest, ptdest->size) ;
            }

            pdp = pdp->link ;
        }

        if (q->pre_event_buffers > 0) {
            pr = q->com->ring->link ;

            while (pr != q->com->ring) {
                if (pr->full) {
                    prdest->id = CTY_RING ;
                    prdest->size = sizeof(tctyring) ;
                    memcpy(&(prdest->loc), &(q->location), sizeof(tlocation)) ;
                    memcpy(&(prdest->name), &(q->seedname), sizeof(tseed_name)) ;
                    prdest->lpad = 0 ;
                    prdest->spare = 0 ;
                    memcpy (&(prdest->comprec), &(pr->rec), LIB_REC_SIZE) ;
                    prdest->crc = gcrccalc ((pointer)((PNTRINT)prdest + 4), prdest->size - 4) ;
                    q660cont_write (q660, prdest, prdest->size) ;
                }

                pr = pr->link ;
            }
        }

        q = q->link ;
    }

    /* flush to disk if appropriate */
    if ((now() - q660->q660_cont_written) > ((int)q660->par_register.opt_q660_cont * 60.0))
        write_q660_cont (q660) ;
    else
        q660->q660cont_updated = TRUE ; /* updated and not on disk */
}

static void process_status (pq660 q660, tstatic *statstor) /* static is reserved word in C, change name */
{
#define MINS_PER_DAY 1440
    I32 tdiff, useable, new_total ;
    enum tlogfld acctype ;
    int hour, dhour, uhours, thours, min, dmin, umins, tmins ;
    taccmstat *paccm ;

    min = lib_round(now()) ;
    tdiff = (min - statstor->time_written) / 60 ; /* get minutes difference */

    if ((tdiff >= MINS_PER_DAY) || (tdiff < 0))
        return ; /* a day old or newer than now, can't use anything */

    new_total = statstor->total_minutes + tdiff ; /* new status time period */

    if (new_total > MINS_PER_DAY)
        new_total = MINS_PER_DAY ;

    useable = new_total - tdiff ;

    if (useable <= 0)
        return ; /* don't have enough data to fill the gap */

    /* clear out accumulators */
    for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++) {
        paccm = &(statstor->accmstats[acctype]) ;
        paccm->accum = 0 ;
        paccm->accum_ds = 0 ;
        paccm->ds_lcq = NIL ;
    }

    /* initialize communications efficiency to INVALID_ENTRY */
    for (hour = 0 ; hour <= 23 ; hour++)
        q660->share.accmstats[LOGF_COMMEFF].hours[hour] = INVALID_ENTRY ;

    for (min = 0 ; min <= 59 ; min++)
        q660->share.accmstats[LOGF_COMMEFF].minutes[min] = INVALID_ENTRY ;

    if (useable > MINS_PER_DAY)
        useable = MINS_PER_DAY ;

    thours = new_total / 60 ; /* total hours */
    tmins = new_total - (thours * 60) ;
    q660->share.total_minutes = new_total ;
    q660->share.stat_minutes = tmins ;
    q660->share.stat_hours = thours ;

    if (q660->share.stat_hours > 23)
        q660->share.stat_hours = 0 ; /* wrap the day */

    uhours = useable / 60 ; /* useable old hours */
    umins = useable - (uhours * 60) ;

    if (uhours > 0) {
        /* have at least an hour, copy the hours plus the entire minutes */
        /* transfer in hour entries */
        hour = statstor->stat_hours - 1 ; /* last useable data */
        dhour = thours - 1 ; /* new last useable data */

        while (uhours > 0) {
            if (hour < 0)
                hour = 23 ;

            if (dhour < 0)
                dhour = 23 ;

            for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
                q660->share.accmstats[acctype].hours[dhour] = statstor->accmstats[acctype].hours[hour] ;

            (uhours)-- ;
            (hour)-- ;
            (dhour)-- ;
        }

        for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
            memcpy(&(q660->share.accmstats[acctype].minutes), &(statstor->accmstats[acctype].minutes), sizeof(taccminutes)) ;
    } else {
        /* transfer in minutes entries */
        min = statstor->stat_minutes - 1 ;
        dmin = tmins - 1 ; /* new last useable */

        while (umins > 0) {
            if (min < 0)
                min = 59 ;

            if (dmin < 0)
                dmin = 59 ;

            for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
                q660->share.accmstats[acctype].minutes[dmin] = statstor->accmstats[acctype].minutes[min] ;

            (umins)-- ;
            (min)-- ;
            (dmin)-- ;
        }
    }

    memcpy(&(q660->station_ident), &(statstor->opstat.station_name), sizeof(string9)) ;
}

void set_loc_name (plcq q)
{
    int i, lth ;
    string3 s ;

    lth = 0 ;

    for (i = 0 ; i <= 1 ; i++)
        if (q->location[i] != ' ')
            s[lth++] = q->location[i] ;

    s[lth] = 0 ;
    strcpy(q->slocation, s) ;
    lth = 0 ;

    for (i = 0 ; i <= 2 ; i++)
        if (q->seedname[i] != ' ')
            s[lth++] = q->seedname[i] ;

    s[lth] = 0 ;
    strcpy(q->sseedname, s) ;
}

void build_fake_log_lcq (pq660 q660, BOOLEAN done)
{
    plcq cur_lcq ;

    getbuf (&(q660->thrmem), (pvoid)&(cur_lcq), sizeof(tlcq)) ;

    if (q660->dplcqs == NIL)
        q660->dplcqs = cur_lcq ;
    else
        q660->dplcqs = extend_link (q660->dplcqs, cur_lcq) ;

    string2fixed (&(cur_lcq->location), "  ") ;
    string2fixed (&(cur_lcq->seedname), "LOG") ;
    set_loc_name (cur_lcq) ;
    cur_lcq->validated = TRUE ; /* unless new tokens remove it */
    cur_lcq->gen_src = GDS_LOG ;
    cur_lcq->sub_field = (U8)LOGF_MSGS ;
    cur_lcq->lcq_num = 0xFF ; /* flag as not indexed */
    cur_lcq->gap_threshold = 0.5 ;
    getbuf (&(q660->thrmem), (pvoid)&(cur_lcq->com), sizeof(tcom_packet)) ;
    cur_lcq->com->frame = 1 ;
    cur_lcq->com->next_compressed_sample = 1 ;
    cur_lcq->com->maxframes = 255 ;
    cur_lcq->arc.records_written = 0 ;
    q660->msg_lcq = cur_lcq ;

    if (done)
        init_dplcqs (q660) ;
}

void restore_thread_continuity (pq660 q660, BOOLEAN pass1, pchar result)
{
    tstatic statstor ;
    tfile_handle cf ;
    int loops, next ;
    tctydplcq *pdlsrc ;
    plcq cur_lcq ;
    string fname ;

    if (result)
        result[0] = 0 ;

    if ((! pass1) && (q660->dplcqs))
        return ; /* we already did this */

    strcpy(fname, "cont/stn.cnt") ;

    if (lib_file_open (q660->par_create.file_owner, fname, LFO_OPEN | LFO_READ, &(cf))) {
        q660->media_error = TRUE ;

        if (! pass1)
            build_fake_log_lcq (q660, TRUE) ;

        return ;
    } else
        q660->media_error = FALSE ;

    if (pass1) {
        lib_file_read (q660->par_create.file_owner, cf, (pvoid)&(statstor), sizeof(tstatic)) ;
        lib_file_close (q660->par_create.file_owner, cf) ;

        if (statstor.id != CTY_STATIC) {
            if (result)
                strcpy(result, "Thread Continuity Format Mis-match") ;

            lib_file_delete (q660->par_create.file_owner, fname) ;
            return ;
        }

        if (statstor.version != CT_VER) {
            if (result)
                sprintf(result, "Thread Continuity Version Mis-match, Got=%d, Expected=%d", statstor.version, CT_VER) ;

            lib_file_delete (q660->par_create.file_owner, fname) ;
            return ;
        }

        if (statstor.crc != gcrccalc ((pointer)((PNTRINT)&(statstor) + 4), sizeof(tstatic) - 4)) {
            if (result)
                strcpy(result, "Thread Continuity CRC Error, Ignoring rest of file") ;

            lib_file_delete (q660->par_create.file_owner, fname) ;
            return ;
        }

        memcpy(&(q660->network), &(statstor.net), sizeof(tseed_net)) ;
        memcpy(&(q660->station), &(statstor.stn), sizeof(tseed_stn)) ;
        q660->saved_data_timetag = statstor.timetag_save ;
        q660->last_status_received = statstor.last_status_save ;
        sprintf (q660->share.sysinfo.prop_tag, "%d", (int)statstor.tag_save) ;
        /*        memcpy(addr(q660->share.fixed.sys_num), addr(statstor.sn_save), sizeof(t64)) ; */
        q660->share.sysinfo.boot = statstor.reboot_save ;
        process_status (q660, &(statstor)) ;
    } else {
        lib_file_seek (q660->par_create.file_owner, cf, sizeof(tstatic)) ; /* skip this */
        loops = 0 ;
        next = sizeof(tstatic) ;

        do {
            if (lib_file_read (q660->par_create.file_owner, cf, q660->cbuf, sizeof(tctydplcq)))
                break ;

            pdlsrc = (pointer)q660->cbuf ;
            next = next + pdlsrc->size ; /* next record */

            if (pdlsrc->crc != gcrccalc ((pointer)((PNTRINT)pdlsrc + 4), pdlsrc->size - 4)) {
                libmsgadd (q660, LIBMSG_CONCRC, "Thread") ;
                lib_file_close (q660->par_create.file_owner, cf) ;

                if (q660->msg_lcq == NIL)
                    build_fake_log_lcq (q660, TRUE) ;

                return ;
            }

            if (pdlsrc->id != CTY_DPLCQ) {
                libmsgadd (q660, LIBMSG_CONPURGE, "Thread") ;
                lib_file_close (q660->par_create.file_owner, cf) ;

                if (q660->msg_lcq == NIL)
                    build_fake_log_lcq (q660, TRUE) ;

                return ;
            }

#ifdef OMITADMINCHANNELSONIDL

            if (strstr(q660->par_register.q660id_address, "@IDL"))

                /* skip administrative DP channels on IDL */
                switch ((enum tlogfld) pdlsrc->dp_src) {
                case LOGF_MSGS :
                case LOGF_CFG :
                case LOGF_TIME :
                    break ;

                default :
                    (loops)++ ;
                    continue ;
                }

#endif

            getbuf (&(q660->thrmem), (pvoid)&(cur_lcq), sizeof(tlcq)) ;

            if (q660->dplcqs == NIL)
                q660->dplcqs = cur_lcq ;
            else
                q660->dplcqs = extend_link (q660->dplcqs, cur_lcq) ;

            memcpy(&(cur_lcq->location), &(pdlsrc->loc), sizeof(tlocation)) ;
            memcpy(&(cur_lcq->seedname), &(pdlsrc->name), sizeof(tseed_name)) ;
            set_loc_name (cur_lcq) ;
            cur_lcq->validated = TRUE ; /* unless new tokens remove it */
            cur_lcq->gen_src = GDS_LOG  ;
            cur_lcq->sub_field = pdlsrc->dp_src ;

            switch ((enum tlogfld)cur_lcq->sub_field) {
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
            }

            cur_lcq->lcq_num = 0xFF ; /* flag as not indexed */
            cur_lcq->gap_threshold = pdlsrc->gap_thresh ;

            if (cur_lcq->gap_threshold == 0.0)
                cur_lcq->gap_threshold = 0.5 ;

            cur_lcq->gap_secs = (1 + cur_lcq->gap_threshold) * abs(cur_lcq->rate) ; /* will always be at least a multiple of the rate */
            cur_lcq->firfixing_gain = 1.000 ; /* default if not over-ridden */
            getbuf (&(q660->thrmem), (pvoid)&(cur_lcq->com), sizeof(tcom_packet)) ;
            cur_lcq->com->frame = 1 ;
            cur_lcq->com->next_compressed_sample = 1 ;
            cur_lcq->com->maxframes = pdlsrc->frame_limit ;
            cur_lcq->com->records_written = pdlsrc->rec_written ;
            cur_lcq->com->last_sample = pdlsrc->last_sample ;
            cur_lcq->backup_tag = pdlsrc->nextrec_tag ;
            cur_lcq->last_timetag = 0 ; /* Expecting a gap, don't report */
            cur_lcq->arc.records_written = pdlsrc->arec_written ;
            lib_file_seek (q660->par_create.file_owner, cf, next) ;
            (loops)++ ;
        } while (! (loops > 9999)) ;

        lib_file_close (q660->par_create.file_owner, cf) ;

        if (q660->msg_lcq == NIL)
            build_fake_log_lcq (q660, FALSE) ;

        init_dplcqs (q660) ;
    }
}

void check_continuity (pq660 q660)
{
    tsystem system ;
    U32 newreboots ;
    string s ;
    string31 s1 ;
    tcont_cache *pcc ;

    if (q660->conthead == NIL) {
        if (! read_q660_cont (q660))
            return ; /* didn't find it */
    }

    pcc = q660->conthead ;
    memcpy (&(system), pcc->payload, sizeof(tsystem)) ;

    if (system.version != CT_VER) {
        sprintf(s, "Version Mis-match, Got=%d, Expected=%d", system.version, CT_VER) ;
        libmsgadd (q660, LIBMSG_CONTIN, s) ;
        return ;
    }

    if (system.crc != gcrccalc ((pointer)((PNTRINT)&(system) + 4), sizeof(tsystem) - 4)) {
        libmsgadd (q660, LIBMSG_CONCRC, "Q660") ;
        return ;
    }

    if (system.id != CTY_SYSTEM) {
        libmsgadd (q660, LIBMSG_CONPURGE, "Q660") ;
        return ;
    }

    if ((system.serial[0] != q660->par_create.q660id_serial[0]) || (system.serial[1] != q660->par_create.q660id_serial[1]))
        return ;

    lock (q660) ;
    newreboots = q660->share.sysinfo.reboots ;
    unlock (q660) ;

    if (system.reboot_counter != newreboots) {
        sprintf(s, "%d Q660 Reboot(s)", (int)(newreboots - system.reboot_counter)) ;
        libmsgadd (q660, LIBMSG_CONTBOOT, s) ;
        add_status (q660, LOGF_REBOOTS, newreboots - system.reboot_counter) ;
        return ;
    }

    q660->data_qual = system.last_dataqual ;
    q660->dt_data_sequence = system.last_dataseq ;
    q660->data_timetag = system.lasttime ;
    sprintf(s, "%s Q=%d", jul_string(lib_round(system.lasttime), s1), system.last_dataqual) ;
    libdatamsg (q660, LIBMSG_CONTFND, s) ;
}

BOOLEAN restore_continuity (pq660 q660)
{
    int loops ;
    plcq q ;
    tsystem *psystem ;
    tctylcq *plsrc ;
    tctyiir *psrc ;
    tctyfir *pfsrc ;
    tctyring *prsrc ;
    piirfilter pdest ;
    /*  tctymh *pmsrc ; */
    tctythr *ptsrc ;
    pdet_packet pdp ;
    /*  con_sto *pmc ; */
    threshold_control_struc *pmt ;
    int points ;
    int i ;
    tcont_cache *pcc ;

    psystem = (pointer)q660->cbuf ;
    pcc = q660->conthead ;

    if (pcc == NIL)
        return FALSE ;

    memcpy (psystem, pcc->payload, sizeof(tsystem)) ;
    pcc = pcc->next ;
    loops = 0 ;
    plsrc = (pointer)q660->cbuf ;

    while ((pcc) && (loops < 9999)) {
        memcpy (q660->cbuf, pcc->payload, pcc->size) ;
        pcc = pcc->next ;
        plsrc = (pointer)q660->cbuf ;
        psrc = (pointer)q660->cbuf ;
        pfsrc = (pointer)q660->cbuf ;
        prsrc = (pointer)q660->cbuf ;
        /*      pmsrc = (pointer)q660->cbuf ; */
        ptsrc = (pointer)q660->cbuf ;

        switch (plsrc->id) {
        case CTY_IIR :
            q = q660->lcqs ;
            pdest = NIL ;

            while (q)
                if ((memcmp(&(q->location), &(psrc->loc), 2) == 0) &&
                        (memcmp(&(q->seedname), &(psrc->name), 3) == 0))

                {
                    pdest = q->stream_iir ;
                    break ;
                } else
                    q = q->link ;

            while (pdest)
                if (strcmp(pdest->def->fname, psrc->fn) == 0) {
                    if (q->rate > 0)
                        points = q->rate ;
                    else
                        points = 1 ;

                    for (i = 1 ; i <= pdest->sects ; i++) {
                        memcpy(&(pdest->filt[i].x), &(psrc->flt[i].x), sizeof(tvector)) ;
                        memcpy(&(pdest->filt[i].y), &(psrc->flt[i].y), sizeof(tvector)) ;
                    }

                    memcpy (&(pdest->out), &(psrc->outbuf), sizeof(tfloat) * points) ;
                    break ;
                } else
                    pdest = pdest->link ;

            break ;

        case CTY_FIR :
            q = q660->lcqs ;

            while (q)
                if ((memcmp(&(q->location), &(pfsrc->loc), 2) == 0) &&
                        (memcmp(&(q->seedname), &(pfsrc->name), 3) == 0) &&
                        (q->source_fir) && (strcmp(q->source_fir->fname, pfsrc->fn) == 0))

                {
                    q->fir->fcount = pfsrc->fcnt ;
                    q->fir->f = (pointer)((PNTRINT)q->fir->fbuf + pfsrc->foff) ;
                    memcpy (q->fir->fbuf, &(pfsrc->fbuffer), q->fir->flen * sizeof(tfloat)) ;
                    q->com->charging = FALSE ; /* not any more */
                    break ;
                } else
                    q = q->link ;

            break ;

        case CTY_LCQ :
            q = q660->lcqs ;

            while (q)
                if ((memcmp(&(q->location), &(plsrc->loc), 2) == 0) &&
                        (memcmp(&(q->seedname), &(plsrc->name), 3) == 0))

                {
                    if (q->rate == 0) {
                        /* use backup values */
                        q->rate = plsrc->prev_rate ;
                        q->delay = plsrc->prev_delay ;
                    }

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
                } else
                    q = q->link ;

            break ;

        case CTY_RING :
            q = q660->lcqs ;

            while (q)
                if ((memcmp(&(q->location), &(prsrc->loc), 2) == 0) &&
                        (memcmp(&(q->seedname), &(prsrc->name), 3) == 0))

                {
                    q->com->ring->full = TRUE ;
                    memcpy (&(q->com->ring->rec), &(prsrc->comprec), LIB_REC_SIZE) ;
                    q->com->last_in_ring = q->com->ring ; /* last one we filled */
                    q->com->ring = q->com->ring->link ;
                    break ;
                } else
                    q = q->link ;

            break ;
#ifdef asdfasdfadsfadsf

        case CTY_SL :
            q = q660->lcqs ;
            pdp = NIL ;

            while (q)
                if ((memcmp(&(q->location), &(pmsrc->loc), 2) == 0) &&
                        (memcmp(&(q->seedname), &(pmsrc->name), 3) == 0))

                {
                    pdp = q->det ;
                    break ;
                } else
                    q = q->link ;

            while (pdp)
                if (strcmp(&(pdp->detector_def->detname), &(pmsrc->dn)) == 0) {
                    pmc = pdp->cont ;
                    memcpy (pmc, &(pmsrc->mh_cont), (PNTRINT)&(pmc->onsetdata) - (PNTRINT)pmc) ;

                    if ((pdp->insamps) && (pmsrc->size > sizeof(tctymh))) {
                        pl = (pointer)((PNTRINT)pmsrc + sizeof(tctymh)) ;
                        memcpy (pdp->insamps, pl, pdp->insamps_size) ;
                    }

                    if ((pmc->default_enabled) != (pdp->det_options & DO_RUN)) {
                        pmc->default_enabled = (pdp->det_options & DO_RUN) ;
                        pmc->detector_enabled = (pdp->det_options & DO_RUN) ;
                    }

                    break ;
                } else
                    pdp = pdp->link ;

            break ;
#endif

        case CTY_THR :
            q = q660->lcqs ;
            pdp = NIL ;

            while (q)
                if ((memcmp(&(q->location), &(ptsrc->loc), 2) == 0) &&
                        (memcmp(&(q->seedname), &(ptsrc->name), 3) == 0))

                {
                    pdp = q->det ;
                    break ;
                } else
                    q = q->link ;

            while (pdp)
                if (strcmp(pdp->detector_def->detname, ptsrc->dn) == 0) {
                    pmt = pdp->cont ;
                    memcpy (pmt, &(ptsrc->thr_cont), (PNTRINT)&(pmt->onsetdata) - (PNTRINT)pmt) ;
#ifdef dasfadsf

                    if ((pmt->default_enabled) != (pdp->det_options & DO_RUN)) {
                        pmt->default_enabled = (pdp->det_options & DO_RUN) ;
                        pmt->detector_enabled = (pdp->det_options & DO_RUN) ;
                    }

#endif
                    break ;
                } else
                    pdp = pdp->link ;

            break ;
        }

        (loops)++ ;
    }

    return TRUE ;
}

void purge_continuity (pq660 q660)
{
    tsystem *psystem ;
    tcont_cache *pcc ;

    psystem = (pointer)q660->cbuf ;
    pcc = q660->conthead ;

    if (pcc) {
        memcpy(psystem, pcc->payload, sizeof(tsystem)) ; /* get existing system record */

        if (psystem->id != CTY_SYSTEM)
            return ; /* already done? */

        psystem->id = CTY_PURGED ;
        psystem->crc = gcrccalc ((pointer)((PNTRINT)psystem + 4), sizeof(tsystem) - 4) ;
        memcpy(pcc->payload, psystem, sizeof(tsystem)) ; /* secretly replace with Folgers Crystals */
    }
}

/* follows immediately after 2nd pass at restoring thread continuity */
void purge_thread_continuity (pq660 q660)
{
    tctydplcq *pdlsrc ;
    tfile_handle cf ;
    string fname ;

    strcpy(fname, "cont/stn.cnt") ;

    if (q660->media_error)
        return ; /* can't initialize media */

    if (lib_file_open (q660->par_create.file_owner, fname, LFO_OPEN | LFO_READ | LFO_WRITE, &(cf)))
        return ;

    pdlsrc = (pointer)q660->cbuf ;
    lib_file_seek (q660->par_create.file_owner, cf, sizeof(tstatic)) ; /* skip this */

    if (lib_file_read (q660->par_create.file_owner, cf, pdlsrc, sizeof(tctydplcq))) {
        lib_file_close (q660->par_create.file_owner, cf) ;
        return ;
    }

    pdlsrc->id = CTY_PURGED ;
    pdlsrc->crc = gcrccalc ((pointer)((PNTRINT)pdlsrc + 4), sizeof(tctydplcq) - 4) ;
    lib_file_seek (q660->par_create.file_owner, cf, sizeof(tstatic)) ;
    lib_file_write (q660->par_create.file_owner, cf, pdlsrc, sizeof(tctydplcq)) ;
    lib_file_close (q660->par_create.file_owner, cf) ;
}

void continuity_timer (pq660 q660)
{

    if ((q660->q660cont_updated) &&
            ((now() - q660->q660_cont_written) > ((int)q660->par_register.opt_q660_cont * 60.0)))

        write_q660_cont (q660) ;
}

