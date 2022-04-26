



/*   Lib660 Archival Miniseed Routines
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
    0 2017-06-06 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libarchive.h"
#include "libmsgs.h"
#include "libseed.h"
#include "libdetect.h"
#include "libcompress.h"
#include "libsampcfg.h"
#include "libfilters.h"
#include "libopaque.h"
#include "liblogs.h"
#include "libsupport.h"
#include "libsample.h"

static void clear_archive (tarc *parc, int size)
{

    parc->appended = FALSE ;
    parc->existing_record = FALSE ;
    parc->total_frames = 0 ;
    memclr(parc->pcfr, size) ;
    memclr(&(parc->hdr_buf), sizeof(seed_header)) ;
}

void flush_archive (pq660 q660, plcq q)
{
#define OCT_1_2016 23673600 /* first possible valid data */
#define MAX_DATE 0x7FFF0000 /* above this just has to be nonsense */
    PU8 p ;
    tarc *parc ;

    parc = &(q->arc) ;
    p = (pointer)parc->pcfr ; /* start of record */
    q660->miniseed_call.timestamp = parc->hdr_buf.starting_time.seed_fpt ;

    if ((q660->miniseed_call.timestamp < OCT_1_2016) || (q660->miniseed_call.timestamp > MAX_DATE)) {
        clear_archive (parc, q660->arc_size) ;
        return ; /* impossible time */
    }

    if (((q->pack_class == PKC_MESSAGE) && (parc->hdr_buf.samples_in_record == 0)) ||
            ((q->pack_class != PKC_MESSAGE) && (parc->total_frames == 0)))

    {
        clear_archive (parc, q660->arc_size) ;
        return ; /* nothing to write */
    }

    storeseedhdr (&(p), (pvoid)&(parc->hdr_buf), q->pack_class == PKC_DATA) ; /* make sure is current */
    q660->miniseed_call.context = q660 ;
    memcpy(&(q660->miniseed_call.station_name), &(q660->station_ident), sizeof(string9)) ;
    strcpy(q660->miniseed_call.location, q->slocation) ;
    strcpy(q660->miniseed_call.channel, q->sseedname) ;
    q660->miniseed_call.chan_number = q->lcq_num ;
    q660->miniseed_call.rate = q->rate ;
    q660->miniseed_call.filter_bits = parc->amini_filter ;
    q660->miniseed_call.packet_class = q->pack_class ;

    if (parc->existing_record)
        if (parc->appended) {
            (parc->records_overwritten_session)++ ;

            if (parc->leave_in_buffer)
                q660->miniseed_call.miniseed_action = MSA_INC ; /* middle of increments */
            else
                q660->miniseed_call.miniseed_action = MSA_FINAL ; /* last increment */
        } else {
            /* client is up to date, leave alone */
            parc->leave_in_buffer = FALSE ;
            clear_archive (parc, q660->arc_size) ;
            return ;
        } else {
        (parc->records_written_session)++ ;

        if (parc->leave_in_buffer)
            q660->miniseed_call.miniseed_action = MSA_FIRST ; /* incremental new record */
        else
            q660->miniseed_call.miniseed_action = MSA_ARC ; /* non-incremental new record */
    }

    parc->last_updated = secsince () ;
    q660->miniseed_call.data_size = q660->arc_size ;
    q660->miniseed_call.data_address = parc->pcfr ;

    if (q660->par_create.call_aminidata)
        q660->par_create.call_aminidata (&(q660->miniseed_call)) ;

    if (parc->leave_in_buffer)
        parc->existing_record = TRUE ; /* has been sent to client once */
    else
        clear_archive (parc, q660->arc_size) ; /* starting over */

    parc->leave_in_buffer = FALSE ;
    parc->appended = FALSE ; /* client is up to date */
}

void archive_512_record (pq660 q660, plcq q, pcompressed_buffer_ring pbuf)
{
    double drate, tdiff ;
    int fcnt, bcnt, dbcnt, i ;
    PU8 psrc, pdest, plink, plast ;
    int size, src, dest, next, offset ;
    tarc *parc ;

    parc = &(q->arc) ;

    switch (q->pack_class) {
    case PKC_DATA :
        if (q->rate > 0)
            drate = q->rate ;
        else if (q->rate < 0)
            drate = 1.0 / abs(q->rate) ;
        else
            return ; /* zero is not a valid data rate */

        bcnt = pbuf->hdr_buf.number_of_following_blockettes - 2 ;
        fcnt = pbuf->hdr_buf.deb.frame_count ;
        dbcnt = parc->hdr_buf.number_of_following_blockettes - 2 ; /* already there */

        if (parc->total_frames > 1) {
            /* check for gaps */
            tdiff = pbuf->hdr_buf.starting_time.seed_fpt -
                    (parc->hdr_buf.starting_time.seed_fpt + parc->hdr_buf.samples_in_record / drate) ;

            if (((bcnt + fcnt + parc->total_frames) > q660->arc_frames) || (fabs(tdiff) > q->gap_secs))
                /* won't fit or time gap */
                flush_archive (q660, q) ;
        }

        psrc = (pointer)((PNTRINT)&(pbuf->rec) + FRAME_SIZE) ;

        if (parc->total_frames > 1) {
            /* append to existing record */
            if (bcnt > 0) {
                /* need to insert one or more blockettes before data */
                if (parc->total_frames > (dbcnt + 1)) {
                    /* need to move some data to further in record */
                    psrc = (pointer)((PNTRINT)parc->pcfr + FRAME_SIZE * (dbcnt + 1)) ;
                    pdest = (pointer)((PNTRINT)parc->pcfr + FRAME_SIZE * (dbcnt + bcnt + 1)) ;
                    memmove (pdest, psrc, FRAME_SIZE * (parc->total_frames - dbcnt - 1)) ;
                    parc->hdr_buf.first_data_byte = parc->hdr_buf.first_data_byte + (bcnt * FRAME_SIZE) ;
                }

                /* copy new blockettes in archive record */
                psrc = (pointer)((PNTRINT)&(pbuf->rec) + FRAME_SIZE) ;
                pdest = (pointer)((PNTRINT)parc->pcfr + FRAME_SIZE * (dbcnt + 1)) ;
                memcpy(pdest, psrc, bcnt * FRAME_SIZE) ;
                psrc = psrc + (bcnt * FRAME_SIZE) ;
                dbcnt = dbcnt + (bcnt) ;

                /* update blockette links */
                for (i = 0 ; i <= dbcnt - 1 ; i++) {
                    if (i) {
                        /* extend link */
                        plink = (pointer)((PNTRINT)parc->pcfr + FRAME_SIZE * i + 2) ;
                        storeword (&(plink), FRAME_SIZE * (i + 1)) ;
                    } else
                        parc->hdr_buf.deb.next_blockette = 64 ; /* make sure goes to first blockette */
                }

                parc->total_frames = parc->total_frames + (bcnt) ;
            }

            if (fcnt > 0) {
                pdest = (pointer)((PNTRINT)parc->pcfr + parc->total_frames * FRAME_SIZE) ; /* add to end */
                memcpy(pdest, psrc, fcnt * FRAME_SIZE) ;
                parc->total_frames = parc->total_frames + (fcnt) ;
            }

            parc->appended = TRUE ;
            parc->hdr_buf.activity_flags = parc->hdr_buf.activity_flags | pbuf->hdr_buf.activity_flags ;
            parc->hdr_buf.data_quality_flags = parc->hdr_buf.data_quality_flags |
                                               (pbuf->hdr_buf.data_quality_flags & ~ SQF_QUESTIONABLE_TIMETAG) ;
            parc->hdr_buf.io_flags = parc->hdr_buf.io_flags | pbuf->hdr_buf.io_flags ;

            if ((pbuf->hdr_buf.data_quality_flags & SQF_QUESTIONABLE_TIMETAG) == 0)
                /* turn off error condition in archive */
                parc->hdr_buf.data_quality_flags = parc->hdr_buf.data_quality_flags & ~
                                                   SQF_QUESTIONABLE_TIMETAG ;

            if (pbuf->hdr_buf.deb.qual > parc->hdr_buf.deb.qual) {
                /* new record has better timetag */
                parc->hdr_buf.starting_time.seed_fpt = pbuf->hdr_buf.starting_time.seed_fpt -
                                                       (parc->hdr_buf.samples_in_record / drate) ; /* new timestamp */
                parc->hdr_buf.deb.qual = pbuf->hdr_buf.deb.qual ; /* use higher quality */
            }

            parc->hdr_buf.samples_in_record = parc->hdr_buf.samples_in_record + (pbuf->hdr_buf.samples_in_record) ;
            parc->hdr_buf.number_of_following_blockettes = parc->hdr_buf.number_of_following_blockettes + (bcnt) ;
            parc->hdr_buf.deb.frame_count = parc->hdr_buf.deb.frame_count + (fcnt) ;
            psrc = (pointer)((PNTRINT)&(pbuf->rec) + (bcnt + 1) * FRAME_SIZE + 8) ;
            pdest = (pointer)((PNTRINT)parc->pcfr + (dbcnt + 1) * FRAME_SIZE + 8) ;
            memcpy (pdest, psrc, 4) ; /* update last sample value */

            if (parc->total_frames >= q660->arc_frames)
                flush_archive (q660, q) ; /* totally full dude */
            else if (parc->incremental) {
                parc->leave_in_buffer = TRUE ;
                flush_archive (q660, q) ; /* write update to record, don't clear */
            }
        } else {
            /* new record */
            memcpy(&(parc->hdr_buf), &(pbuf->hdr_buf), sizeof(seed_header)) ; /* copy header */
            parc->hdr_buf.dob.rec_length = q660->par_create.amini_exponent ;
            parc->hdr_buf.sequence.seed_num = parc->records_written + 1 ;
            (parc->records_written)++ ;
            psrc = (pointer)((PNTRINT)&(pbuf->rec) + FRAME_SIZE) ;
            pdest = (pointer)((PNTRINT)parc->pcfr + FRAME_SIZE) ;

            if ((bcnt + fcnt) > 0)
                memcpy(pdest, psrc, (bcnt + fcnt) * FRAME_SIZE) ;

            parc->total_frames = 1 + bcnt + fcnt ;
            parc->appended = TRUE ;
            parc->existing_record = FALSE ;

            if (parc->incremental) {
                parc->leave_in_buffer = TRUE ;
                flush_archive (q660, q) ; /* write new record, but don't clear */
            }
        }

        break ;

    case PKC_MESSAGE :
        if (pbuf->hdr_buf.samples_in_record == 0)
            return ;

        if (((pbuf->hdr_buf.samples_in_record + parc->hdr_buf.samples_in_record) > (q660->arc_size - NONDATA_OVERHEAD)) ||
                (pbuf->hdr_buf.starting_time.seed_fpt > (parc->hdr_buf.starting_time.seed_fpt + 60)))
            /* won't fit or not the same time */
            flush_archive (q660, q) ;

        psrc = (pointer)((PNTRINT)&(pbuf->rec) + NONDATA_OVERHEAD) ;
        pdest = (pointer)((PNTRINT)parc->pcfr + NONDATA_OVERHEAD + parc->hdr_buf.samples_in_record) ;

        if (parc->hdr_buf.samples_in_record == 0) {
            /* new record */
            memcpy(&(parc->hdr_buf), &(pbuf->hdr_buf), sizeof(seed_header)) ; /* copy header */
            parc->hdr_buf.dob.rec_length = q660->par_create.amini_exponent ;
            parc->hdr_buf.samples_in_record = 0 ; /* don't count first record twice! */
            parc->hdr_buf.sequence.seed_num = parc->records_written + 1 ;
            (parc->records_written)++ ;
            parc->appended = FALSE ;
            parc->existing_record = FALSE ;
        }

        memcpy(pdest, psrc, pbuf->hdr_buf.samples_in_record) ;
        parc->hdr_buf.samples_in_record = parc->hdr_buf.samples_in_record + (pbuf->hdr_buf.samples_in_record) ;
        parc->appended = TRUE ;
        break ;

    case PKC_TIMING : /* Note: incoming will only have one blockette */
        if ((TIMING_BLOCKETTE_SIZE + parc->total_frames) > q660->arc_size)
            flush_archive (q660, q) ; /* new one won't fit */

        if (parc->total_frames > 0) {
            if ((lib_round(pbuf->hdr_buf.starting_time.seed_fpt) / 3600) !=
                    (lib_round(parc->hdr_buf.starting_time.seed_fpt) / 3600))

                flush_archive (q660, q) ; /* different hour, start new record */
        }

        psrc = (pointer)((PNTRINT)&(pbuf->rec) + NONDATA_OVERHEAD) ;

        if (parc->total_frames > 0) {
            /* append to existing record, put data starting at total_frames */
            pdest = (pointer)((PNTRINT)parc->pcfr + parc->total_frames) ;
            memcpy (pdest, psrc, TIMING_BLOCKETTE_SIZE) ;
            psrc = (pointer)((PNTRINT)parc->pcfr + parc->total_frames - TIMING_BLOCKETTE_SIZE + 2) ; /* previous blockette */
            storeword (&(psrc), parc->total_frames) ; /* extend link */
            parc->total_frames = parc->total_frames + (TIMING_BLOCKETTE_SIZE) ;
            (parc->hdr_buf.number_of_following_blockettes)++ ; /* a blockette was added */
            parc->appended = TRUE ;
        } else {
            /* new record */
            memcpy(&(parc->hdr_buf), &(pbuf->hdr_buf), sizeof(seed_header)) ; /* copy header */
            parc->hdr_buf.dob.rec_length = q660->par_create.amini_exponent ;
            parc->hdr_buf.sequence.seed_num = parc->records_written + 1 ;
            (parc->records_written)++ ;
            pdest = (pointer)((PNTRINT)parc->pcfr + NONDATA_OVERHEAD) ;
            memcpy (pdest, psrc, TIMING_BLOCKETTE_SIZE) ;
            parc->total_frames = NONDATA_OVERHEAD + TIMING_BLOCKETTE_SIZE ;
            parc->appended = TRUE ;
            parc->existing_record = FALSE ;
        }

        break ;

    case PKC_OPAQUE : /* note : blockette_index is the next free blockette location */
        bcnt = pbuf->hdr_buf.number_of_following_blockettes - 1 ; /* to be added */

        if (bcnt == 0)
            return ; /* nothing to do */

        size = q->com->blockette_index - NONDATA_OVERHEAD ; /* always a multiple of 4 bytes */

        if ((size + parc->total_frames) > q660->arc_size)
            flush_archive (q660, q) ; /* new one won't fit or must preserve time */

        if (parc->total_frames > 0) {
            if ((lib_round(pbuf->hdr_buf.starting_time.seed_fpt) / 3600) !=
                    (lib_round(parc->hdr_buf.starting_time.seed_fpt) / 3600))

                flush_archive (q660, q) ; /* different hour, start new record */
        }

        psrc = (pointer)((PNTRINT)&(pbuf->rec) + NONDATA_OVERHEAD) ;

        if (parc->total_frames == 0) {
            /* new record */
            memcpy(&(parc->hdr_buf), &(pbuf->hdr_buf), sizeof(seed_header)) ; /* copy header */
            parc->hdr_buf.dob.rec_length = q660->par_create.amini_exponent ;
            parc->hdr_buf.sequence.seed_num = parc->records_written + 1 ;
            (parc->records_written)++ ;
            pdest = (pointer)((PNTRINT)parc->pcfr + NONDATA_OVERHEAD) ;
            memcpy (pdest, psrc, size) ; /* copy blockettes in as they are */
            parc->total_frames = q->com->blockette_index ;
            parc->appended = TRUE ;
            parc->existing_record = FALSE ;
        } else {
            /* need to extend an existing record */
            plink = NIL ;
            plast = plink ;
            dest = parc->hdr_buf.dob.next_blockette ;

            while (dest) { /* find the last blockette, plast will have it's link address */
                plink = (pointer)((PNTRINT)parc->pcfr + dest + 2) ;
                plast = plink ;
                dest = loadword (&(plink)) ;
            }

            pdest = (pointer)((PNTRINT)parc->pcfr + parc->total_frames) ;
            memcpy (pdest, psrc, size) ; /* move in blockettes */
            storeword (&(plast), parc->total_frames) ; /* adding to the list, need to rebuild links */
            src = NONDATA_OVERHEAD ;
            dest = parc->total_frames ; /* where we currently are */

            for (i = 1 ; i <= bcnt - 1 ; i++) {
                plink = (pointer)((PNTRINT)&(pbuf->rec) + src + 2) ;
                next = loadword (&(plink)) ; /* get old starting offset of next frame */
                offset = next - src ; /* amount to jump to get to next frame */
                pdest = (pointer)((PNTRINT)parc->pcfr + dest + 2) ;
                storeword (&(pdest), dest + offset) ; /* new starting offset of next blockette */
                src = next ;
                dest = dest + offset ;
            }

            parc->total_frames = parc->total_frames + (size) ;
            parc->hdr_buf.number_of_following_blockettes = parc->hdr_buf.number_of_following_blockettes + (bcnt) ;
            parc->appended = TRUE ;
        }

        break ;

    case PKC_EVENT :
    case PKC_CALIBRATE :
        break ; /* Something need here? */
    }
}

/* ask the client for the last record. If onelcq is NIL then read all normal or dp lcqs
  based on the from660 flag, else read that one lcq */
void preload_archive (pq660 q660, BOOLEAN from660, plcq onelcq)
{
    plcq q ;
    PU8 p ;
    int fcnt ;
    tarc *parc ;

    if (onelcq)
        q = onelcq ;
    else if (from660)
        q = q660->lcqs ;
    else
        q = q660->dplcqs ;

    while (q) {
        if (q->arc.amini_filter) {
            parc = &(q->arc) ;
            q660->miniseed_call.context = q660 ;
            memcpy(&(q660->miniseed_call.station_name), &(q660->station_ident), sizeof(string9)) ;
            strcpy(q660->miniseed_call.location, q->slocation) ;
            strcpy(q660->miniseed_call.channel, q->sseedname) ;
            q660->miniseed_call.chan_number = q->lcq_num ;
            q660->miniseed_call.rate = q->rate ;
            q660->miniseed_call.filter_bits = parc->amini_filter ;
            q660->miniseed_call.packet_class = q->pack_class ;
            q660->miniseed_call.miniseed_action = MSA_GETARC ;
            q660->miniseed_call.data_size = q660->arc_size ;
            q660->miniseed_call.data_address = parc->pcfr ;

            if (q660->par_create.call_aminidata) {
                q660->par_create.call_aminidata (&(q660->miniseed_call)) ;

                if (q660->miniseed_call.miniseed_action == MSA_RETARC) {
                    /* extract seed header and set flags */
                    p = (pointer)parc->pcfr ;
                    loadseedhdr (&(p), &(parc->hdr_buf), (q660->miniseed_call.packet_class == PKC_DATA)) ;
                    fcnt = parc->hdr_buf.deb.frame_count + parc->hdr_buf.number_of_following_blockettes - 1 ;

                    if ((q->pack_class == PKC_DATA) && (fcnt < q660->arc_frames) && (q660->contingood)) {
                        /* try to extend */
                        parc->hdr_buf.sequence.seed_num = parc->records_written ;
                        parc->hdr_buf.starting_time.seed_fpt =
                            extract_time(&(parc->hdr_buf.starting_time), parc->hdr_buf.deb.usec99) ;
                        parc->existing_record = TRUE ;
                        parc->total_frames = fcnt ;
                    } else {
                        /* not data record or can't extend existing record */
                        memset(&(parc->hdr_buf), 0, sizeof(seed_header)) ; /* throw away the header */
                        memset(parc->pcfr, 0, q660->arc_size) ; /* and the data */
                        q->backup_tag = 0 ;
                        q->backup_qual = 0 ; /* if setup by continuity */
                    }
                }
            }
        }

        if (onelcq)
            break ;
        else
            q = q->link ;
    }
}

