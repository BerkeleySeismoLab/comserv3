



/*   Lib660 Seed Compression Routines
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
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libcompress.h"
#include "libmsgs.h"
#include "libseed.h"
#include "libdetect.h"

#define B7X4 (2 << 2) /* level 2 compression code bits, within each block */
#define B6X5 1
#define B5X6 0
#define B3X10 3
#define B2X15 2
#define B1X30 1
#define LARGE 1073741823
#define BIG 536870911

typedef struct
{
    int scan ;
    U16 bc ;
    I32 cbits ;
    int shift ;
    I32 mask ;
    I32 disc ;
} compseqtype ;
typedef compseqtype compseq_table_type[7] ;
typedef struct
{
    int samps ;
    int postshift ;
    I32 mask ;
    I32 hibit ;
    I32 neg ;
} decompbittype ;
typedef decompbittype decomarray[7] ;

const compseq_table_type compseq = {
    {/*scan*/7, /*bc*/3,  /*cbits*/B7X4,  /*shift*/4,    /*mask*/15,     /*disc*/7},
    {/*scan*/6, /*bc*/3,  /*cbits*/B6X5,  /*shift*/5,    /*mask*/31,    /*disc*/15},
    {/*scan*/5, /*bc*/3,  /*cbits*/B5X6,  /*shift*/6,    /*mask*/63,    /*disc*/31},
    {/*scan*/4, /*bc*/1,     /*cbits*/0,  /*shift*/8,   /*mask*/255,   /*disc*/127},
    {/*scan*/3, /*bc*/2, /*cbits*/B3X10, /*shift*/10,  /*mask*/1023,   /*disc*/511},
    {/*scan*/2, /*bc*/2, /*cbits*/B2X15, /*shift*/15, /*mask*/32767, /*disc*/16383},
    {/*scan*/1, /*bc*/2, /*cbits*/B1X30, /*shift*/30, /*mask*/LARGE,   /*disc*/BIG}
} ;

const decomarray decomptab = {
    {/*samps*/4, /*postshift*/8,  /*mask*/255,        /*hibit*/128,       /*neg*/256},
    {/*samps*/1, /*postshift*/0,  /*mask*/1073741823, /*hibit*/536870912, /*neg*/1073741824},
    {/*samps*/2, /*postshift*/15, /*mask*/32767,      /*hibit*/16384,     /*neg*/32768},
    {/*samps*/3, /*postshift*/10, /*mask*/1023,       /*hibit*/512,       /*neg*/1024},
    {/*samps*/5, /*postshift*/6,  /*mask*/63,         /*hibit*/32,        /*neg*/64},
    {/*samps*/6, /*postshift*/5,  /*mask*/31,         /*hibit*/16,        /*neg*/32},
    {/*samps*/7, /*postshift*/4,  /*mask*/15,         /*hibit*/8,         /*neg*/16}
} ;

void no_previous (pq660 q660)
{
    plcq q ;
    pdet_packet det ;
    con_common *pcc ;

    q = q660->lcqs ;

    while (q) {
        q->precomp.prev_value = 0 ; /* we will have a break in the data */
        det = q->det ;

        while (det) {
            pcc = det->cont ;
            pcc->first_detection = TRUE ;
            det = det->link ;
        }

        q = q->link ;
    }
}

int compress_block (pq660 q660, plcq q, pcom_packet pcom)
{
    int i ;
    int block_code ;
    BOOLEAN done ;
    I32 samp_1 ;
    I32 accum, value ;
    int hiscan, ctabw, ctabfit, ctablo ;
    const compseqtype *sp ;
    int t_scan ;
    int t_shift ;
    I32 t_mask ;
    I32 t_disc ;
    PU8 p ;
    string15 s ;

    if (pcom->ring == NIL) {
        seed2string(q->location, q->seedname, s) ;
        libmsgadd (q660, LIBMSG_NILRING, s) ;
        return 0 ;
    }

    samp_1 = pcom->peeks[pcom->next_out] ;

    if ((pcom->frame == (pcom->blockette_count + 1)) && (pcom->block <= 1)) {
        pcom->frame_buffer[1] = samp_1 ;
        pcom->frame_buffer[2] = 0 ;
        pcom->block = 3 ;
        pcom->flag_word = 0 ;
        pcom->ctabx = 0 ;
    }

    /*
     * scan the appropriate number of differences based
     * on the roaming table pointer, and update the roaming table pointer.
     */
    pcom->sc[1] = pcom->last_sample ;

    /*
     * now start the scan loop for this block. "hiscan" is the number of differences in
     * the local difference buffer, which is increased as required.
     * "ctabfit" is > 0 if a table entry has been found in which all differences fit;
     * when starting the scan for this block, we don't know what fits.
     * "ctabw" is the table pointer at entry, which if it necessary to scan beyond, means
     * that once a fit is found, there is not point in looking backwards to see if any
     * more will fit.
     */
    if (pcom->peek_total < MAXSAMPPERWORD) {
        ctablo = MAXSAMPPERWORD - pcom->peek_total ;

        if (pcom->ctabx < ctablo)
            pcom->ctabx = ctablo ;
    } else
        ctablo = 0 ;

    hiscan = 0 ;
    done = FALSE ;
    ctabfit = -1 ;
    ctabw = pcom->ctabx ;

    do {
        sp = &(compseq[pcom->ctabx]) ;
        t_scan = sp->scan ;
        t_disc = sp->disc ;

        while (hiscan < t_scan) {
            value = pcom->peeks[(pcom->next_out + hiscan) & PEEKMASK] ;
            pcom->sc[hiscan + 2] = value ;
            pcom->diffs[hiscan] = value - pcom->sc[hiscan + 1] ;
            (hiscan)++ ;
        }

        for (i = 0 ; i <= t_scan - 1 ; i++)
            if (abs(pcom->diffs[i]) > t_disc) {
                /*
                 * at least one difference in the scan does not fit. if there has been
                 * no table entry found yet for which the data will fit, go to the
                 * next larger storage size. otherwise, take the one that fits, and
                 * quit scanning. this may be the case if the table is probed toward
                 * the beginning, looking for more efficient fits.
                 */
                if (ctabfit < 0)
                    if (pcom->ctabx >= 6) {
                        seed2string(q->location, q->seedname, s) ;
                        libmsgadd(q660, LIBMSG_UNCOMP, s) ;
                        done = TRUE ;
                    } else
                        (pcom->ctabx)++ ;
                else {
                    pcom->ctabx = ctabfit ;
                    done = TRUE ;
                }

                break ;
            } else if (i == (t_scan - 1)) {
                /*
                 * here, all differences within the scan fit. if the table is scanning
                 * toward larger storage sizes, then smaller ones cannot possibly fit,
                 * so the scan is done.
                 */
                if (pcom->ctabx > ctabw)
                    done = TRUE ;
                else

                    /*
                     * here, note that at least the current table entry is a fit,
                     * but if the start of the table has not been reached, try scanning
                     * toward smaller storage sizes. if the smaller storage size won't
                     * work, the size selected by "ctabfit" will be used (see above).
                     */
                    if (pcom->ctabx > ctablo) {
                        ctabfit = pcom->ctabx ;
                        pcom->ctabx = pcom->ctabx - 1 ;
                    } else
                        done = TRUE ;

                break ;
            }

        /*
         * continue scanning this block until a fit is found or the end of the table (maximum storage size)
         * is hit and the differences still don't fit.
         */
    } while (! done) ;

    /*
     * using the selected storage unit, pack the differences into the current block and
     * update various counters and indices
     */
    sp = &(compseq[pcom->ctabx]) ;
    t_scan = sp->scan ;
    t_shift = sp->shift ;
    t_mask = sp->mask ;
    pcom->peek_total = pcom->peek_total - t_scan ;
    pcom->next_out = (pcom->next_out + t_scan) & PEEKMASK ;
    block_code = sp->bc ;
    accum = sp->cbits ;

    for (i = 0 ; i <= t_scan - 1 ; i++)
        accum = (accum << t_shift) | (t_mask & pcom->diffs[i]) ;

    pcom->frame_buffer[pcom->block] = accum ;
    pcom->flag_word = (pcom->flag_word << 2) + block_code ;
    pcom->last_sample = pcom->sc[t_scan + 1] ;
    (pcom->block)++ ;

    if (pcom->block >= WORDS_PER_FRAME) {
        /* have a frame */
        pcom->frame_buffer[0] = pcom->flag_word ;
        p = (pointer)((PNTRINT)&(pcom->ring->rec) + pcom->frame * FRAME_SIZE) ;
        storeframe (&(p), &(pcom->frame_buffer)) ;
        memset(&(pcom->frame_buffer), 0, sizeof(compressed_frame)) ;
        (pcom->frame)++ ;
        pcom->block = 1 ;
        pcom->flag_word = 0 ;
#ifdef asdfadsfdsfs

        if ((! q->data_written) && (pcom->frame < (pcom->maxframes + 1)))
            evaluate_detector_stack (q660, q) ;

#endif
    }

    return t_scan ;
}

int decompress_blockette (pq660 q660, plcq q)
{
    int ptridx, subcode, k, dblocks, midx, crate ;
    I32 curval, work, accum ;
    const decompbittype *dcp ;
    PU8 pd, pm ;
    tprecomp *pcmp ;
    I32 unpacked[MAXSAMPPERWORD] ;
    string95 s ;
    string15 s1 ;
    int v1, v2 ;

    pcmp = &(q->precomp) ;
    ptridx = 0 ; /* first databuf entry */
    pcmp->block_idx = 0 ; /* first compressed block */
    curval = pcmp->prev_sample ;

    if ((pcmp->prev_value) && (curval != pcmp->prev_value)) {
        v1 = curval ;
        v2 = pcmp->prev_value ;
        sprintf(s, "%s %d %d", seed2string(q->location, q->seedname, s1), v1, v2) ;
        libdatamsg (q660, LIBMSG_CONTERR, s) ;
    }

    dblocks = pcmp->blocks ;
    pd = pcmp->pdata ;
    midx = pcmp->mapidx ;
    pm = pcmp->pmap ;
    pcmp->curmap = loadword (&(pm)) ;

    if ((q->gen_src >= GDS_MD) && (q->gen_src <= GDS_AC) && (q->rate == 1))
        crate = 10 ;
    else
        crate = q->rate ;

    while ((dblocks > 0) && (ptridx < crate)) {
        accum = loadlongint(&(pd)) ;
        subcode = 0 ; /* default */

        switch ((pcmp->curmap >> (14 - midx)) & 3) {
        case 2 :
            subcode = (accum >> 30) & 3 ;
            break ;

        case 3 :
            subcode = 4 + ((accum >> 30) & 3) ;
            break ;
        }

        if (subcode > 6) {
            /* not a valid subcode */
            pcmp->blocks = 0 ; /* nothing valid */
            break ;
        }

        dcp = &(decomptab[subcode]) ;

        if (q->idxbuf)
            (*(q->idxbuf))[pcmp->block_idx] = ptridx ;

        (pcmp->block_idx)++ ;

        for (k = dcp->samps - 1 ; k >= 0 ; k--) {
            work = accum & dcp->mask ;

            if (work & dcp->hibit)
                work = work - dcp->neg ;

            unpacked[k] = work ;
            accum = accum >> dcp->postshift ;
        }

        for (k = 0 ; k <= dcp->samps - 1 ; k++) {
            curval = curval + unpacked[k] ;

            if (ptridx >= crate) {
                pcmp->blocks = 0 ; /* nothing valid */
                break ;
            }

            (*(q->databuf))[ptridx] = curval ;
            (ptridx)++ ;
        }

        (dblocks)-- ;
        midx = midx + (2) ;

        if (midx > 14) {
            midx = 0 ;
            pcmp->curmap = loadword (&(pm)) ;
        }
    }

    pcmp->prev_value = curval ;

    if (q->idxbuf)
        (*(q->idxbuf))[pcmp->block_idx] = ptridx ;

    pcmp->block_idx = 0 ; /* for build_frames later on */
    return ptridx ;
}

int build_blocks (pq660 q660, plcq q, pcom_packet pcom)
{
    int block_code ;
    int number_of_samples ;
    PU8 p ;
    tprecomp *pcmp ;
    string15 s ;

    pcmp = &(q->precomp) ;

    if (pcom->ring == NIL) {
        seed2string(q->location, q->seedname, s) ;
        libmsgadd (q660, LIBMSG_NILRING, s) ;
        return 1 ;
    }

    if ((pcom->frame == (pcom->blockette_count + 1)) && (pcom->block <= 1)) {
        /* beginning of new record */
        pcom->frame_buffer[1] = (*(q->databuf))[(*(q->idxbuf))[pcmp->block_idx]] ; /* first sample */
        pcom->frame_buffer[2] = 0 ; /* for now */
        pcom->block = 3 ; /* first read block */
    }

    number_of_samples = (*(q->idxbuf))[pcmp->block_idx] ; /* at entry */

    while ((pcmp->blocks > 0) && (pcom->block < WORDS_PER_FRAME)) {
        if (pcmp->mapidx == 0)
            pcmp->curmap = loadword (&(pcmp->pmap)) ;

        block_code = (pcmp->curmap >> (14 - pcmp->mapidx)) & 3 ;
        pcom->frame_buffer[pcom->block] = loadlongword (&(pcmp->pdata)) ;
        (pcmp->blocks)-- ;
        (pcmp->block_idx)++ ;
        pcmp->mapidx = pcmp->mapidx + (2) ;

        if (pcmp->mapidx > 14)
            pcmp->mapidx = 0 ;

        pcom->flag_word = (pcom->flag_word << 2) + block_code ;
        (pcom->block)++ ;
    }

    if (pcom->block >= WORDS_PER_FRAME) {
        /* have a frame */
        pcom->frame_buffer[0] = pcom->flag_word ;
        p = (pointer)((PNTRINT)&(pcom->ring->rec) + pcom->frame * FRAME_SIZE) ;
        storeframe (&(p), &(pcom->frame_buffer)) ;
        memset(&(pcom->frame_buffer), 0, sizeof(compressed_frame)) ;
        (pcom->frame)++ ;
        pcom->block = 1 ;
        pcom->flag_word = 0 ;
#ifdef asdfasdfsdf

        if ((! q->data_written) && (pcom->frame < (pcom->maxframes + 1)))
            evaluate_detector_stack (q660, q) ;

#endif
    }

    pcom->last_sample = (*(q->databuf))[(*(q->idxbuf))[pcmp->block_idx] - 1] ; /* last value put in frame */
    return (*(q->idxbuf))[pcmp->block_idx] - number_of_samples ; /* number of samples used */
}

