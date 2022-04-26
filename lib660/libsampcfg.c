



/*   Lib660 time series configuration routines
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
    0 2017-06-11 rdr Created
    1 2018-04-06 rdr In lib_lcqstat description is read from DATA XML, not made up here.
    2 2018-08-28 rdr Show filter delays down to usec.
    3 2018-08-29 rdr In init_lcq fix handling of sub-hz rates when allocating buffers.
                     Remove logdispatch table, it wasn't used.
    4 2018-08-31 rdr Missed changing one of the places delay is shown to 6 places.
    5 2019-06-26 rdr Add support for GPS data streams.
    6 2019-09-26 rdr Add init_ll_lcq.
    7 2020-01-07 rdr Add setting up precompressed buffer for >= 100SPS due to lower
                     limit on low latency data.
    8 2020-03-17 jms fix use of loc codes with dplcq channels
    9 2021-01-06 jms remove unused code. block admin DP channels on IDL
   10 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
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

#define OMITADMINCHANNELSONIDL

U32 secsince (void)
{

    return lib_round(now()) ;
}

void deallocate_sg (pq660 q660)
{
    U32 totrec ;
    string63 s ;
    string31 s2 ;

    if (q660->need_sats)
        finish_log_clock (q660) ;

    if (q660->cur_verbosity & VERB_LOGEXTRA) {
        flush_lcqs (q660) ;
        flush_dplcqs (q660) ;
        totrec = print_generated_rectotals (q660) ;
        sprintf (s, "Done: %d recs. data end: %s", (int)totrec, jul_string(q660->dt_data_sequence, s2)) ;
        libdatamsg (q660, LIBMSG_TOTAL, s) ;
    }

    flush_lcqs (q660) ;
    save_continuity (q660) ;
    /* info not available for this
      totrec = print_actual_rectotals (q660) ;
      newuser.msg = inttostr(totrec) + ' recs. seq end: ' +
                     inttostr(dt_data_sequence) ;
    */
    memclr (&(q660->first_clear), (int)((PNTRINT)&(q660->last_clear) - (PNTRINT)&(q660->first_clear))) ;
    memclr (&(q660->share.first_share_clear), (int)((PNTRINT)&(q660->share.last_share_clear) -
            (PNTRINT)&(q660->share.first_share_clear))) ;
    initialize_memory_utils (&(q660->connmem)) ;
}

pchar realtostr (double r, int digits, pchar result)
{
    string15 fmt ;

    sprintf(fmt, "%%%d.%df", digits + 2, digits) ;
    sprintf(result, fmt, r) ;
    return result ;
}

static pchar scvrate (int rate, pchar result)
{

    if (rate >= 0)
        sprintf(result, "%d", rate) ;
    else
        realtostr(-1.0 / rate, 4, result) ;

    return result ;
}

static pchar sfcorner (pq660 q660, int i, pchar result)
{
    enum tlinear filt ;

    lock (q660) ;
    filt = q660->share.digis[i].linear ;
    unlock (q660) ;

    switch (filt) {
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
    }

    return result ;
}

void set_gaps (plcq q)
{

    if (q->rate > 0) {
        q->gap_secs = q->gap_threshold / q->rate ;

        if ((q->gen_src >= GDS_MD) && (q->gen_src <= GDS_AC) && (q->rate == 1))
            q->gap_offset = 10.0 ; /* Only received every 10 seconds */
        else
            q->gap_offset = 1.0 ; /* default is one set of data points per second */
    } else {
        q->gap_secs = q->gap_threshold * abs(q->rate) ;
        q->gap_offset = abs(q->rate) ; /* default is one data point per "rate" seconds */
    }
}

void set_gain_bits (pq660 q660, plcq q, U8 *gb)
{
    U8 w ;
    int i, chan ;

    chan = q->sub_field >> 4 ; /* get to channel bits */
    w = 0 ;
    i = q660->share.digis[chan].pgagain ;

    while (i > 1) {
        (w)++ ;
        i = i >> 1 ;
    }

    w = w & 7 ; /* Only 3 bits allocated */

    if (q660->share.digis[chan].lowvolt)
        w = w | DEB_LOWV ;

    *gb = w ;
}

static void setup_names (plcq q, pchar name)
{
    int i ;
    pchar pc ;

    pc = strchr(name, '-') ;

    if (pc) {
        *pc++ = 0 ; /* terminate location, move to seedname */
        strcpy (q->sseedname, pc) ;
        strcpy (q->slocation, name) ;
    } else {
        q->slocation[0] = 0 ;
        strcpy (q->sseedname, name) ;
    }

    memset (&(q->location), 0x20, 2) ;
    memset (&(q->seedname), 0x20, 3) ;

    for (i = 0 ; i < (int)strlen(q->slocation) ; i++)
        q->location[i] = q->slocation[i] ;

    for (i = 0 ; i < (int)strlen(q->sseedname) ; i++)
        q->seedname[i] = q->sseedname[i] ;
}

void set_net_station (pq660 q660)
{
    int i ;

    memset (q660->network, 0x20, 2) ;
    memset (q660->station, 0x20, 5) ;

    for (i = 0 ; i < (int)strlen(q660->share.seed.network) ; i++)
        q660->network[i] = q660->share.seed.network[i] ;

    for (i = 0 ; i < (int)strlen(q660->share.seed.station) ; i++)
        q660->station[i] = q660->share.seed.station[i] ;
}

static void add_mdispatch_x (pq660 q660, pchan pch, plcq q, int offset)
{
    int i, j ;

    i = (pch->sub_field >> 4) + offset ; /* get channel */
    j = pch->freqnum ; /* frequency bit */
    q660->mdispatch[i][j] = q ;
}

void init_lcq (pq660 q660)
{
    plcq p, pl, pl2 ;
    int i, count, crate ;
    pchan pch, psrc ;
    string63 s ;
    string31 s1, s2, s3, s4 ;
    pcompressed_buffer_ring pr, lastpr ;
    int buffers ;
    pdownstream_packet pds ;
#ifdef adsfadsfadsf
    pdet_packet pdp ;
    pdetect pd ;
#endif

    pch = q660->chanchain ;
    count = 0 ;

    while (pch) {
        getbuf (&(q660->connmem), (pointer)&(pch->auxinfo), sizeof(tlcq)) ; /* for non-xml fields */
        pl = pch->auxinfo ;

        if (q660->lcqs == NIL)
            q660->lcqs = pl ;
        else
            q660->lcqs = extend_link (q660->lcqs, pl) ;

        /* Setup lcq fields from XML configuration */
        pl->def = pch ;

        if (pch->rate > 0.9)
            pl->rate = lib_round(pch->rate) ;
        else if (pch->rate != 0.0)
            pl->rate = -lib_round(1.0 / pch->rate) ;
        else
            pl->rate = 0 ;

        setup_names (pl, pch->seedname) ;
        pl->lcq_num = q660->highest_lcqnum ;
        (q660->highest_lcqnum)++ ;
        pl->caldly = 60 ;
        pl->delay = pch->delay ;
        pl->gen_src = pch->gen_src ;
        pl->sub_field = pch->sub_field ;
        getbuf (&(q660->connmem), (pointer)&(pl->com), sizeof(tcom_packet)) ;
        pl->firfixing_gain = 1.000 ; /* default if not over-ridden */
        pl->com->maxframes = FRAMES_PER_RECORD - 1 ;
        pl->com->frame = 1 ;
        pl->com->next_compressed_sample = 1 ;
        pl->pre_event_buffers = 0 ; /* NEEDS WORK */
        pl->gap_threshold = 0.5 ;
        set_gaps (pl) ;
        pl->firfixing_gain = 1.0 ;

        if ((pl->gen_src >= GDS_MD) && (pl->gen_src <= GDS_AC)) {
            set_gain_bits (q660, pl, &(pl->gain_bits)) ;
            /* Get digitizer channel */
            i = pl->sub_field >> 4 ;

            switch (pl->gen_src) {
            case GDS_CM :
                i = i + CAL_CHANNEL ;
                break ;

            case GDS_AC :
                i = i + MAIN_CHANNELS ;
                break ;

            default :
                break ;
            }

            if ((pl->sub_field & 0xF) == 0)
                pl->delay = pl->delay + 9.0 ; /* Compensate for bunching into 10 second blocks */

            if (q660->cur_verbosity & VERB_LOGEXTRA) {
                sprintf(s, "%s:%d@%s,%s=%s", seed2string(pl->location, pl->seedname, s1),
                        i + 1, scvrate(pl->rate, s2), sfcorner(q660, i, s3), realtostr(pl->delay, 6, s4)) ;
                libmsgadd (q660, LIBMSG_FILTDLY, s) ;
            }
        }

        switch (pl->gen_src) {
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

        case GDS_DUST :
            q660->dustdispatch[pl->sub_field] = pl ;
            break ;

        case GDS_DEC : /* decimated from another LCQ */
            psrc = q660->chanchain ;

            while (psrc)
                if (strcmp(psrc->seedname, pch->decsrc) == 0) {
                    pl->prev_link = (plcq)psrc->auxinfo ;
                    getbuf (&(q660->connmem), (pointer)&(pds), sizeof(tdownstream_packet)) ;
                    pds->link = NIL ;
                    pds->derived_q = pl ;
                    pl2 = psrc->auxinfo ;
                    pl2->downstream_link = extend_link (pl2->downstream_link, pds) ;
                    break ;
                } else
                    psrc = psrc->link ;

            if (pch->decfilt[0]) {
                pl->source_fir = find_fir (q660, pch->decfilt) ;

                if (pl->source_fir == NIL)
                    libmsgadd(q660, LIBMSG_DECNOTFOUND, pch->seedname) ;
            }

            pl2 = psrc->auxinfo ;
            pl->input_sample_rate = psrc->rate ; /* In floating point */

            if (pl->input_sample_rate >= 0.999)
                pl->gap_offset = 10.0 ;
            else if (pl->input_sample_rate != 0)
                pl->gap_offset = 1.0 / pl->input_sample_rate ; /* set new gap offset based on input rate */
            else
                pl->gap_offset = 1.0 ;

            if ((pl->source_fir != NIL) && (pl->input_sample_rate != 0))
                pl->delay = pl2->delay + pl->source_fir->dly / pl->input_sample_rate ;
            else
                pl->delay = pl2->delay ;

            if (q660->cur_verbosity & VERB_LOGEXTRA) {
                sprintf(s, "%s:%s@%s=%s", seed2string(pl->location, pl->seedname, s1),
                        seed2string(pl->prev_link->location, pl->prev_link->seedname, s2),
                        scvrate(pl->rate, s3), realtostr(pl->delay, 6, s4)) ;
                libmsgadd (q660, LIBMSG_FILTDLY, s) ;
            }

            pl->com->charging = TRUE ;
            p = pl->prev_link ;

            while (p->prev_link)
                p = p->prev_link ;

            /* see if root source is 1hz */
            if ((p) && (p->rate == 1)) {
                /* yes, need to synchronize based on rate */
                pl->slipping = TRUE ;
                pl->slip_modulus = abs(pl->rate) ; /* .1hz has modulus of 10 */
            }

            /* see if root source is main digitizer */
            if ((p) && (p->def->gen_src >= GDS_MD) && (p->def->gen_src <= GDS_AC))
                set_gain_bits (q660, p, &(pl->gain_bits)) ;

            break ;

        default :
            break ;
        }

        /* BUILD DETECTOR CHAIN HERE */
        crate = pl->rate ; /* Compression rate */

        if ((pl->gen_src >= GDS_MD) && (pl->gen_src <= GDS_AC) && (crate == 1))
            crate = 10 ; /* 1Hz main digitizer data is compressed in 10 sample blocks */

        if (crate < 0)
            crate = 1 ; /* sub 1Hz data is compressed one sample at a time */

        pl->datasize = crate * sizeof(I32) ;
        getbuf (&(q660->connmem), (pointer)&(pl->databuf), pl->datasize) ;

        if (crate > 1)
            getbuf (&(q660->connmem), (pointer)&(pl->idxbuf), (crate + 1) * sizeof(U16)) ;

        if (q660->par_create.call_minidata)
            pl->mini_filter = q660->par_create.opt_minifilter & OMF_ALL ;

        if ((q660->arc_size > 0) && (q660->par_create.call_aminidata)) {
            pl->arc.amini_filter = q660->par_create.opt_aminifilter & OMF_ALL ;
            pl->arc.incremental = (pl->rate <= q660->par_create.amini_512highest) ;
        }

        if (pl->rate >= 100) {
            pl->precomp.data_size = pl->rate * sizeof(I32) ;
            pl->precomp.map_size = ((pl->rate >> 2) + 3) & 0xFFFC ; /* round up to 32 bits */
            getbuf (&(q660->connmem), (pointer)&(pl->precomp.map_buf), pl->precomp.map_size) ;
            getbuf (&(q660->connmem), (pointer)&(pl->precomp.data_buf), pl->precomp.data_size) ;
            pl->precomp.map_ptr = pl->precomp.map_buf ;
            pl->precomp.data_ptr = pl->precomp.data_buf ;
            pl->precomp.map_cntr = 0 ;
        } else {
            pl->precomp.map_size = 0 ;
            pl->precomp.data_size = 0 ;
        }

        pl->pack_class = PKC_DATA ; /* assume data */
        set_gain_bits (q660, pl, &(pl->gain_bits)) ;
        buffers = pl->pre_event_buffers + 1 ; /* need one for construction */
        pr = NIL ;
        lastpr = NIL ;

        while (buffers > 0) {
            getbuf (&(q660->connmem), (pointer)&(pr), sizeof(tcompressed_buffer_ring)) ;
            pr->link = NIL ;
            pr->full = FALSE ;

            if (pl->com->ring == NIL)
                pl->com->ring = pr ;
            else if (lastpr)
                lastpr->link = pr ;

            lastpr = pr ;
            (buffers)-- ;
        }

        if (pr)
            pr->link = pl->com->ring ;

        if (pl->arc.amini_filter)
            getbuf (&(q660->connmem), (pointer)&(pl->arc.pcfr), q660->arc_size) ;

        /* Future event only handling goes here */
        pl->scd_cont = SCD_BOTH ;
        allocate_lcq_filters (q660, pl) ;
        (count)++ ;
        pch = pch->link ;
    }
}

void init_ll_lcq (pq660 q660)
{
    plcq pl ;
    int idxsize ;

    pl = &(q660->ll_lcq) ;
    pl->gen_src = GDS_MD ; /* To force setup */
    pl->datasize = MAX_RATE * sizeof(I32) ; /* allocate for largest */
    idxsize = (MAX_RATE + 1) * sizeof(U16) ;
    getbuf (&(q660->connmem), (pointer)&(pl->databuf), pl->datasize) ;
    getbuf (&(q660->connmem), (pointer)&(pl->idxbuf), idxsize) ;
} ;

void init_dplcq (pq660 q660, plcq pl, BOOLEAN newone)
{
    pcompressed_buffer_ring pr ;
    enum tlogfld acctype ;

    pl->dtsequence = 0 ;
    acctype = (enum tlogfld)pl->sub_field ;

    if (acctype >= AC_FIRST) {
        if (acctype <= AC_LAST)
            q660->share.accmstats[acctype].ds_lcq = pl ;
        else if (acctype == LOGF_NDATLAT)
            q660->data_latency_lcq = pl ;
        else if (acctype == LOGF_LDATLAT)
            q660->low_latency_lcq = pl ;
        else if (acctype == LOGF_STATLAT)
            q660->status_latency_lcq = pl ;
    }

    if (q660->par_create.call_minidata)
        pl->mini_filter = q660->par_create.opt_minifilter & OMF_ALL ;

    if ((q660->arc_size > 0) && (q660->par_create.call_aminidata)) {
        pl->arc.amini_filter = q660->par_create.opt_aminifilter & OMF_ALL ;
        pl->arc.incremental = pl->rate <= q660->par_create.amini_512highest ;
    }

    pl->scd_cont = SCD_BOTH ; /* both are continuous */
    pl->scd_evt = 0 ;

    switch (acctype) {
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
    }

    if (pl->com->maxframes >= FRAMES_PER_RECORD)
        pl->com->maxframes = FRAMES_PER_RECORD - 1 ;

    if (! newone)
        return ; /* don't need to allocate new buffers */

    getbuf (&(q660->thrmem), (pointer)&(pr), sizeof(tcompressed_buffer_ring)) ;
    pr->full = FALSE ;
    pl->com->ring = pr ;
    pr->link = pl->com->ring ; /* just keeps going back to itself */

    if (pl->arc.amini_filter)
        getbuf (&(q660->thrmem), (pointer)&(pl->arc.pcfr), q660->arc_size) ;
}

void process_dplcqs (pq660 q660)
{
    int count ;
    plcq cur_lcq ;
    pchan pch ;
    pchar pc ;
    string2 loc ;
    string3 name ;
    BOOLEAN newone ;
    string snamecpy ;

    pch = q660->dlchain ;
    count = 0 ;

    while (pch) {

#ifdef OMITADMINCHANNELSONIDL

        if (strstr(q660->par_register.q660id_address, "@IDL"))

            /* skip administrative DP channels on IDL */
            switch ((enum tlogfld)pch->sub_field) {
            case LOGF_MSGS :
            case LOGF_CFG :
            case LOGF_TIME :
                break ;

            default :
                /* fprintf(stderr,"SKIPPING: %s\n",pch->seedname); */
                pch = pch->link ;
                continue;
            }

#endif

        strcpy(snamecpy,pch->seedname) ;
        pc = strchr(snamecpy, '-') ;

        if (pc) {
            *pc++ = 0 ; /* terminate location, move to seedname */
            strcpy (name, pc) ;
            strcpy (loc, name) ;
        } else {
            loc[0] = 0 ;
            strcpy (name, pch->seedname) ;
        }

        cur_lcq = q660->dplcqs ;
        newone = FALSE ;

        while (cur_lcq)
            if ((strcmp(cur_lcq->slocation, loc) == 0) &&
                    (strcmp(cur_lcq->sseedname, name) == 0))

                break ;
            else
                cur_lcq = cur_lcq->link ;

        if (cur_lcq == NIL) {
            /* add new one */
            newone = TRUE ;
            getbuf (&(q660->thrmem), (pointer)&(cur_lcq), sizeof(tlcq)) ;

            if (q660->dplcqs == NIL)
                q660->dplcqs = cur_lcq ;
            else
                q660->dplcqs = extend_link (q660->dplcqs, cur_lcq) ;
        }

        cur_lcq->def = pch ;
        setup_names (cur_lcq, pch->seedname) ;
        cur_lcq->delay = pch->delay ;
        cur_lcq->gen_src = pch->gen_src ;
        cur_lcq->sub_field = pch->sub_field ;

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

        cur_lcq->lcq_num = 0xFF ; /* we don't actually use indexing for these lcqs */
        cur_lcq->validated = TRUE ;
        cur_lcq->firfixing_gain = 1.000 ; /* default if not over-ridden */

        if (newone) {
            getbuf (&(q660->thrmem), (pointer)&(cur_lcq->com), sizeof(tcom_packet)) ;
            cur_lcq->com->frame = 1 ;
            cur_lcq->com->next_compressed_sample = 1 ;
            cur_lcq->com->maxframes = FRAMES_PER_RECORD - 1 ;
        }

        cur_lcq->caldly = 60 ;

        if (cur_lcq->gap_threshold == 0.0)
            cur_lcq->gap_threshold = 0.5 ;

        set_gaps (cur_lcq) ;
        init_dplcq (q660, cur_lcq, newone) ;

        if (newone)
            preload_archive (q660, FALSE, cur_lcq) ;

        (count)++ ;
        pch = pch->link ;
    }
}

void init_dplcqs (pq660 q660)
{
    plcq pl ;
    int i ;

    pl = q660->dplcqs ;
    i = 0 ;

    while (pl) {
        init_dplcq (q660, pl, TRUE) ;
        (i)++ ;
        pl = pl->link ;
    }

    if (q660->arc_size > 0)
        preload_archive (q660, FALSE, NIL) ;
}

enum tliberr lib_lcqstat (pq660 q660, tlcqstat *lcqstat)
{
    plcq q ;
    U32 cur ;
    int pass ;
    tonelcqstat *pone ;

    lcqstat->count = 0 ;
    cur = secsince () ;

    for (pass = 1 ; pass <= 2 ; pass++) {
        if (pass == 1)
            q = q660->lcqs ;
        else
            q = q660->dplcqs ;

        while ((lcqstat->count < MAX_LCQ) && (q)) {
            pone = &(lcqstat->entries[lcqstat->count]) ;
            strcpy(pone->location, q->slocation) ;
            strcpy(pone->channel, q->sseedname) ;
            pone->chan_number = q->lcq_num ;
            pone->rec_cnt = q->records_generated_session ;
            pone->rec_seq = q->com->records_written ;

            if (q->last_record_generated == 0)
                pone->rec_age = -1 ; /* not written */
            else
                pone->rec_age = cur - q->last_record_generated ;

            pone->det_count = q->detections_session ;
            pone->cal_count = q->calibrations_session ;
            pone->arec_cnt = q->arc.records_written_session ;
            pone->arec_over = q->arc.records_overwritten_session ;

            if (q->arc.last_updated == 0)
                pone->arec_age = -1 ;
            else
                pone->arec_age = cur - q->arc.last_updated ;

            pone->arec_seq = q->arc.records_written ;

            if (q->def)
                strcpy (pone->desc, q->def->desc) ;
            else
                strcpy (pone->desc, "") ;

            (lcqstat->count)++ ;
            q = q->link ;
        }
    }

    return LIBERR_NOERR ;
}

void clear_calstat (pq660 q660)
{
    plcq pl ;

    if (q660->libstate != LIBSTATE_RUN)
        return ;

    pl = q660->lcqs ;

    while (pl) {
        pl->calstat = FALSE ; /* can't be on */
        pl = pl->link ;
    }
}

enum tliberr lib_getdpcfg (pq660 q660, tdpcfg *dpcfg)
{
    plcq q ;

    if ((q660->libstate != LIBSTATE_RUN) && (q660->libstate != LIBSTATE_RUNWAIT))
        return LIBERR_CFGWAIT ;

    memclr (dpcfg, sizeof(tdpcfg)) ;
    memcpy(&(dpcfg->station_name), &(q660->station_ident), sizeof(string9)) ;
    q = q660->lcqs ;

    while (q) {
        dpcfg->buffer_counts[q->lcq_num] = q->pre_event_buffers + 1 ;
        q = q->link ;
    }

    return LIBERR_NOERR ;
}


