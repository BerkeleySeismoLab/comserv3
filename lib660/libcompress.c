/*   Lib660 Seed Compression Routines
     Copyright 2006 Certified Software Corporation

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
#include "libcompress.h"
#include "libmsgs.h"
#include "libseed.h"
#include "libdetect.h"

#define B7X4 (2 shl 2) /* level 2 compression code bits, within each block */
#define B6X5 1
#define B5X6 0
#define B3X10 3
#define B2X15 2
#define B1X30 1
#define LARGE 1073741823
#define BIG 536870911

typedef struct {
  integer scan ;
  word bc ;
  longint cbits ;
  integer shift ;
  longint mask ;
  longint disc ;
} compseqtype ;
typedef compseqtype compseq_table_type[7] ;
typedef struct {
  integer samps ;
  integer postshift ;
  longint mask ;
  longint hibit ;
  longint neg ;
} decompbittype ;
typedef decompbittype decomarray[7] ;

const compseq_table_type compseq =
 {{/*scan*/7, /*bc*/3,  /*cbits*/B7X4,  /*shift*/4,    /*mask*/15,     /*disc*/7},
  {/*scan*/6, /*bc*/3,  /*cbits*/B6X5,  /*shift*/5,    /*mask*/31,    /*disc*/15},
  {/*scan*/5, /*bc*/3,  /*cbits*/B5X6,  /*shift*/6,    /*mask*/63,    /*disc*/31},
  {/*scan*/4, /*bc*/1,     /*cbits*/0,  /*shift*/8,   /*mask*/255,   /*disc*/127},
  {/*scan*/3, /*bc*/2, /*cbits*/B3X10, /*shift*/10,  /*mask*/1023,   /*disc*/511},
  {/*scan*/2, /*bc*/2, /*cbits*/B2X15, /*shift*/15, /*mask*/32767, /*disc*/16383},
  {/*scan*/1, /*bc*/2, /*cbits*/B1X30, /*shift*/30, /*mask*/LARGE,   /*disc*/BIG}} ;

const decomarray decomptab =
  {{/*samps*/4, /*postshift*/8,  /*mask*/255,        /*hibit*/128,       /*neg*/256},
   {/*samps*/1, /*postshift*/0,  /*mask*/1073741823, /*hibit*/536870912, /*neg*/1073741824},
   {/*samps*/2, /*postshift*/15, /*mask*/32767,      /*hibit*/16384,     /*neg*/32768},
   {/*samps*/3, /*postshift*/10, /*mask*/1023,       /*hibit*/512,       /*neg*/1024},
   {/*samps*/5, /*postshift*/6,  /*mask*/63,         /*hibit*/32,        /*neg*/64},
   {/*samps*/6, /*postshift*/5,  /*mask*/31,         /*hibit*/16,        /*neg*/32},
   {/*samps*/7, /*postshift*/4,  /*mask*/15,         /*hibit*/8,         /*neg*/16}} ;

void no_previous (pq660 q660)
begin
  plcq q ;
  pdet_packet det ;
  con_common *pcc ;

  q = q660->lcqs ;
  while (q)
    begin
      q->precomp.prev_value = 0 ; /* we will have a break in the data */
      det = q->det ;
      while (det)
        begin
          pcc = det->cont ;
          pcc->first_detection = TRUE ;
          det = det->link ;
        end
      q = q->link ;
    end
end

integer compress_block (pq660 q660, plcq q, pcom_packet pcom)
begin
  integer i ;
  integer block_code ;
  boolean done ;
  longint samp_1 ;
  longint accum, value ;
  integer hiscan, ctabw, ctabfit, ctablo ;
  const compseqtype *sp ;
  integer t_scan ;
  integer t_shift ;
  longint t_mask ;
  longint t_disc ;
  pbyte p ;
  string15 s ;

  if (pcom->ring == NIL)
    then
      begin
        seed2string(q->location, q->seedname, s) ;
        libmsgadd (q660, LIBMSG_NILRING, s) ;
        return 0 ;
      end
  samp_1 = pcom->peeks[pcom->next_out] ;
  if ((pcom->frame == (pcom->blockette_count + 1)) land (pcom->block <= 1))
    then
      begin
        pcom->frame_buffer[1] = samp_1 ;
        pcom->frame_buffer[2] = 0 ;
        pcom->block = 3 ;
        pcom->flag_word = 0 ;
        pcom->ctabx = 0 ;
      end
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
  if (pcom->peek_total < MAXSAMPPERWORD)
    then
      begin
        ctablo = MAXSAMPPERWORD - pcom->peek_total ;
        if (pcom->ctabx < ctablo)
          then
            pcom->ctabx = ctablo ;
      end
    else
      ctablo = 0 ;
  hiscan = 0 ;
  done = FALSE ;
  ctabfit = -1 ;
  ctabw = pcom->ctabx ;
  repeat
    sp = addr(compseq[pcom->ctabx]) ;
    t_scan = sp->scan ;
    t_disc = sp->disc ;
    while (hiscan < t_scan)
      begin
        value = pcom->peeks[(pcom->next_out + hiscan) and PEEKMASK] ;
        pcom->sc[hiscan + 2] = value ;
        pcom->diffs[hiscan] = value - pcom->sc[hiscan + 1] ;
        inc(hiscan) ;
      end
    for (i = 0 ; i <= t_scan - 1 ; i++)
      if (abs(pcom->diffs[i]) > t_disc)
        then
          begin
            /*
             * at least one difference in the scan does not fit. if there has been
             * no table entry found yet for which the data will fit, go to the
             * next larger storage size. otherwise, take the one that fits, and
             * quit scanning. this may be the case if the table is probed toward
             * the beginning, looking for more efficient fits.
             */
            if (ctabfit < 0)
              then
                if (pcom->ctabx >= 6)
                  then
                    begin
                      seed2string(q->location, q->seedname, s) ;
                      libmsgadd(q660, LIBMSG_UNCOMP, s) ;
                      done = TRUE ;
                    end
                  else
                    inc(pcom->ctabx) ;
              else
                begin
                  pcom->ctabx = ctabfit ;
                  done = TRUE ;
                end
            break ;
          end
      else if (i == (t_scan - 1))
        then
          begin
            /*
             * here, all differences within the scan fit. if the table is scanning
             * toward larger storage sizes, then smaller ones cannot possibly fit,
             * so the scan is done.
             */
            if (pcom->ctabx > ctabw)
              then
                done = TRUE ;
              else
                /*
                 * here, note that at least the current table entry is a fit,
                 * but if the start of the table has not been reached, try scanning
                 * toward smaller storage sizes. if the smaller storage size won't
                 * work, the size selected by "ctabfit" will be used (see above).
                 */
                if (pcom->ctabx > ctablo)
                  then
                    begin
                      ctabfit = pcom->ctabx ;
                      pcom->ctabx = pcom->ctabx - 1 ;
                    end
                  else
                    done = TRUE ;
            break ;
          end
   /*
    * continue scanning this block until a fit is found or the end of the table (maximum storage size)
    * is hit and the differences still don't fit.
    */
  until done) ;
  /*
   * using the selected storage unit, pack the differences into the current block and
   * update various counters and indices
   */
  sp = addr(compseq[pcom->ctabx]) ;
  t_scan = sp->scan ;
  t_shift = sp->shift ;
  t_mask = sp->mask ;
  pcom->peek_total = pcom->peek_total - t_scan ;
  pcom->next_out = (pcom->next_out + t_scan) and PEEKMASK ;
  block_code = sp->bc ;
  accum = sp->cbits ;
  for (i = 0 ; i <= t_scan - 1 ; i++)
    accum = (accum shl t_shift) or (t_mask and pcom->diffs[i]) ;
  pcom->frame_buffer[pcom->block] = accum ;
  pcom->flag_word = (pcom->flag_word shl 2) + block_code ;
  pcom->last_sample = pcom->sc[t_scan + 1] ;
  inc(pcom->block) ;
  if (pcom->block >= WORDS_PER_FRAME)
    then
      begin /* have a frame */
        pcom->frame_buffer[0] = pcom->flag_word ;
        p = (pointer)((pntrint)addr(pcom->ring->rec) + pcom->frame * FRAME_SIZE) ;
        storeframe (addr(p), addr(pcom->frame_buffer)) ;
        memset(addr(pcom->frame_buffer), 0, sizeof(compressed_frame)) ;
        inc(pcom->frame) ;
        pcom->block = 1 ;
        pcom->flag_word = 0 ;
#ifdef asdfadsfdsfs
        if ((lnot q->data_written) land (pcom->frame < (pcom->maxframes + 1)))
          then
            evaluate_detector_stack (q660, q) ;
#endif
      end
  return t_scan ;
end

integer decompress_blockette (pq660 q660, plcq q)
begin
  integer ptridx, subcode, k, dblocks, midx, crate ;
  longint curval, work, accum ;
  const decompbittype *dcp ;
  pbyte pd, pm ;
  tprecomp *pcmp ;
  longint unpacked[MAXSAMPPERWORD] ;
  string95 s ;
  string15 s1 ;
  integer v1, v2 ;

  pcmp = addr(q->precomp) ;
  ptridx = 0 ; /* first databuf entry */
  pcmp->block_idx = 0 ; /* first compressed block */
  curval = pcmp->prev_sample ;
  if ((pcmp->prev_value) land (curval != pcmp->prev_value))
    then
      begin
        v1 = curval ;
        v2 = pcmp->prev_value ;
        sprintf(s, "%s %d %d", seed2string(q->location, q->seedname, s1), v1, v2) ;
        libdatamsg (q660, LIBMSG_CONTERR, s) ;
      end
  dblocks = pcmp->blocks ;
  pd = pcmp->pdata ;
  midx = pcmp->mapidx ;
  pm = pcmp->pmap ;
  pcmp->curmap = loadword (addr(pm)) ;
  if ((q->gen_src >= GDS_MD) land (q->gen_src <= GDS_AC) land (q->rate == 1))
    then
      crate = 10 ;
    else
      crate = q->rate ;
  while ((dblocks > 0) land (ptridx < crate))
    begin
      accum = loadlongint(addr(pd)) ;
      subcode = 0 ; /* default */
      switch ((pcmp->curmap shr (14 - midx)) and 3) begin
        case 2 :
          subcode = (accum shr 30) and 3 ;
          break ;
        case 3 :
          subcode = 4 + ((accum shr 30) and 3) ;
          break ;
      end
      if (subcode > 6)
        then
          begin /* not a valid subcode */
            pcmp->blocks = 0 ; /* nothing valid */
            break ;
          end
      dcp = addr(decomptab[subcode]) ;
      if (q->idxbuf)
        then
          (*(q->idxbuf))[pcmp->block_idx] = ptridx ;
      inc(pcmp->block_idx) ;
      for (k = dcp->samps - 1 ; k >= 0 ; k--)
        begin
          work = accum and dcp->mask ;
          if (work and dcp->hibit)
            then
              work = work - dcp->neg ;
          unpacked[k] = work ;
          accum = accum shr dcp->postshift ;
        end
      for (k = 0 ; k <= dcp->samps - 1 ; k++)
        begin
          curval = curval + unpacked[k] ;
          if (ptridx >= crate)
            then
              begin
                pcmp->blocks = 0 ; /* nothing valid */
                break ;
              end
          (*(q->databuf))[ptridx] = curval ;
          inc(ptridx) ;
        end
      dec(dblocks) ;
      incn(midx, 2) ;
      if (midx > 14)
        then
          begin
            midx = 0 ;
            pcmp->curmap = loadword (addr(pm)) ;
          end
    end
  pcmp->prev_value = curval ;
  if (q->idxbuf)
    then
      (*(q->idxbuf))[pcmp->block_idx] = ptridx ;
  pcmp->block_idx = 0 ; /* for build_frames later on */
  return ptridx ;
end

integer build_blocks (pq660 q660, plcq q, pcom_packet pcom)
begin
  integer block_code ;
  integer number_of_samples ;
  pbyte p ;
  tprecomp *pcmp ;
  string15 s ;

  pcmp = addr(q->precomp) ;
  if (pcom->ring == NIL)
    then
      begin
        seed2string(q->location, q->seedname, s) ;
        libmsgadd (q660, LIBMSG_NILRING, s) ;
        return 1 ;
      end
  if ((pcom->frame == (pcom->blockette_count + 1)) land (pcom->block <= 1))
    then
      begin /* beginning of new record */
        pcom->frame_buffer[1] = (*(q->databuf))[(*(q->idxbuf))[pcmp->block_idx]] ; /* first sample */
        pcom->frame_buffer[2] = 0 ; /* for now */
        pcom->block = 3 ; /* first read block */
      end
  number_of_samples = (*(q->idxbuf))[pcmp->block_idx] ; /* at entry */
  while ((pcmp->blocks > 0) land (pcom->block < WORDS_PER_FRAME))
    begin
      if (pcmp->mapidx == 0)
        then
          pcmp->curmap = loadword (addr(pcmp->pmap)) ;
      block_code = (pcmp->curmap shr (14 - pcmp->mapidx)) and 3 ;
      pcom->frame_buffer[pcom->block] = loadlongword (addr(pcmp->pdata)) ;
      dec(pcmp->blocks) ;
      inc(pcmp->block_idx) ;
      incn(pcmp->mapidx, 2) ;
      if (pcmp->mapidx > 14)
        then
          pcmp->mapidx = 0 ;
      pcom->flag_word = (pcom->flag_word shl 2) + block_code ;
      inc(pcom->block) ;
    end
  if (pcom->block >= WORDS_PER_FRAME)
    then
      begin /* have a frame */
        pcom->frame_buffer[0] = pcom->flag_word ;
        p = (pointer)((pntrint)addr(pcom->ring->rec) + pcom->frame * FRAME_SIZE) ;
        storeframe (addr(p), addr(pcom->frame_buffer)) ;
        memset(addr(pcom->frame_buffer), 0, sizeof(compressed_frame)) ;
        inc(pcom->frame) ;
        pcom->block = 1 ;
        pcom->flag_word = 0 ;
#ifdef asdfasdfsdf
        if ((lnot q->data_written) land (pcom->frame < (pcom->maxframes + 1)))
          then
            evaluate_detector_stack (q660, q) ;
#endif
      end
  pcom->last_sample = (*(q->databuf))[(*(q->idxbuf))[pcmp->block_idx] - 1] ; /* last value put in frame */
  return (*(q->idxbuf))[pcmp->block_idx] - number_of_samples ; /* number of samples used */
end

