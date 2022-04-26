



/*   Lib660 Opaque Blockette Routines
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
    0 2017-06-10 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libopaque.h"
#include "libmsgs.h"
#include "libseed.h"
#include "libsample.h"
#include "libsupport.h"
#include "libsampcfg.h"
#include "libarchive.h"

void write_ocf (pq660 q660)
{
    int sizeleft, sz ;
    int rnum, lth, size ;
    pchar ps ;
    U8 flag ;
    int maxsz ;
    topaque_hdr hdr ;
    PU8 p ;
    pchar pc ;
    plcq pcl ;
    pcom_packet pcom ;
    seed_header *phdr ;

    pcl = q660->cfg_lcq ;

    if (pcl == NIL)
        return ;

    pcom = pcl->com ;
    phdr = &(pcom->ring->hdr_buf) ;
    pcl->data_written = FALSE ;
    maxsz = NONDATA_AREA - OPAQUE_HDR_SIZE ;
    q660->cfg_lastwritten = now () ;
    sizeleft = q660->xmlsize ;
    ps = (pchar)q660->xmlbuf ;
    rnum = 0 ;

    while (sizeleft > 0) {
        flag = 1 ; /* a stream */

        if (sizeleft > maxsz)
            sz = maxsz ;
        else
            sz = sizeleft ;

        if (q660->xmlsize > maxsz) {
            /* have a series */
            if (rnum == 0)
                flag = flag | 4 ; /* first of a series */
            else if (sizeleft == sz)
                flag = flag | 8 ; /* last of a series */
            else
                flag = flag | 0xC ; /* middle of a series */
        }

        size = 0 ;
        /* Start copying to opaque buffer */
        pc = (pchar)&(q660->opaque_buf) ;

        while ((size + (int)strlen(ps) + 2) <= sz) {
            lth = (int)strlen(ps) ;
            memcpy (pc, ps, lth) ;
            pc = pc + (lth) ;
            *pc++ = 0xd ;
            *pc++ = 0xa ;
            size = size + lth + 2 ;
            ps = (pchar)((PNTRINT)ps + lth + 2) ; /* string, NULL, and LF */
        }

        if (size == 0)
            break ; /* Invalid XML, avoid infinite loop */

        phdr->seed_record_type = 'D' ;
        phdr->continuation_record = ' ' ;
        phdr->sequence.seed_num = pcom->records_written + 1 ;
        (pcom->records_written)++ ;
        memcpy (&(phdr->location_id), &(pcl->location), sizeof(tlocation)) ;
        memcpy (&(phdr->channel_id), &(pcl->seedname), sizeof(tseed_name)) ;
        memcpy (&(phdr->station_id_call_letters), &(q660->station), sizeof(tseed_stn)) ;
        memcpy (&(phdr->seednet), &(q660->network), sizeof(tseed_net)) ;
        phdr->sample_rate_multiplier = 1 ;
        phdr->number_of_following_blockettes = 2 ;
        phdr->first_blockette_byte = 48 ;
        phdr->starting_time.seed_fpt = q660->data_timetag ;
        phdr->dob.blockette_type = 1000 ;
        phdr->dob.word_order = 1 ;
        phdr->dob.rec_length = RECORD_EXP ;
        phdr->dob.next_blockette = 56 ;
        p = (PU8)((PNTRINT)&(pcom->ring->rec) + 56) ;
        hdr.blockette_type = 2000 ;
        hdr.next_blockette = 0 ;
        hdr.blk_lth = OPAQUE_HDR_SIZE + size ;
        hdr.data_off = OPAQUE_HDR_SIZE ;
        hdr.recnum = rnum ;
        hdr.word_order = 1 ; /* big endian */
        hdr.opaque_flags = flag ;
        hdr.hdr_fields = 1 ;
        hdr.rec_type[0] = 'X' ;
        hdr.rec_type[1] = 'M' ;
        hdr.rec_type[2] = 'L' ;
        hdr.rec_type[3] = 'C' ;
        hdr.rec_type[4] = '~' ;
        storeopaque (&(p), &(hdr), &(q660->opaque_buf), size) ;
        pcom->last_blockette = pcom->blockette_index ;
        pcom->blockette_index = (56 + OPAQUE_HDR_SIZE + size + 3) & 0xFFFC ;
        p = (PU8)&(pcom->ring->rec) ;
        storeseedhdr (&(p), (pvoid)&(pcom->ring->hdr_buf), FALSE) ;
        (pcl->records_generated_session)++ ;
        pcl->last_record_generated = secsince () ;
        send_to_client (q660, q660->cfg_lcq, pcom->ring, SCD_BOTH) ;
        memclr (&(pcom->ring->rec), LIB_REC_SIZE) ;
        memclr (&(pcom->ring->hdr_buf), sizeof(seed_header)) ;
        pcom->blockette_index = 56 ;
        pcom->last_blockette = 48 ;
        pcom->blockette_count = 0 ;
        (rnum)++ ;
        sizeleft = sizeleft - size ;
    }

    if ((pcl->arc.amini_filter) && (pcl->arc.total_frames > 1))
        flush_archive (q660, q660->cfg_lcq) ;
}


