/*   Lib660 Opaque Blockette Routines
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
    0 2017-06-10 rdr Created
*/
#include "libopaque.h"
#include "libmsgs.h"
#include "libseed.h"
#include "libsample.h"
#include "libsupport.h"
#include "libsampcfg.h"
#include "libarchive.h"

void write_ocf (pq660 q660)
begin
  integer sizeleft, sz ;
  integer rnum, lth, size ;
  pchar ps ;
  byte flag ;
  integer maxsz ;
  topaque_hdr hdr ;
  pbyte p ;
  pchar pc ;
  plcq pcl ;
  pcom_packet pcom ;
  seed_header *phdr ;

  pcl = q660->cfg_lcq ;
  if (pcl == NIL)
    then
      return ;
  pcom = pcl->com ;
  phdr = addr(pcom->ring->hdr_buf) ;
  pcl->data_written = FALSE ;
  maxsz = NONDATA_AREA - OPAQUE_HDR_SIZE ;
  q660->cfg_lastwritten = now () ;
  sizeleft = q660->xmlsize ;
  ps = (pchar)q660->xmlbuf ;
  rnum = 0 ;
  while (sizeleft > 0) 
    begin
      flag = 1 ; /* a stream */
      if (sizeleft > maxsz)
        then
          sz = maxsz ;
        else
          sz = sizeleft ;
      if (q660->xmlsize > maxsz)
        then
          begin /* have a series */
            if (rnum == 0)
              then
                flag = flag or 4 ; /* first of a series */
            else if (sizeleft == sz)
              then
                flag = flag or 8 ; /* last of a series */
              else
                flag = flag or 0xC ; /* middle of a series */
          end
      size = 0 ;
      /* Start copying to opaque buffer */
      pc = (pchar)addr(q660->opaque_buf) ;
      while ((size + (integer)strlen(ps) + 2) <= sz)
        begin
          lth = (integer)strlen(ps) ;
          memcpy (pc, ps, lth) ;
          incn(pc, lth) ;
          *pc++ = 0xd ;
          *pc++ = 0xa ;
          size = size + lth + 2 ;
          ps = (pchar)((pntrint)ps + lth + 2) ; /* string, NULL, and LF */
        end
      if (size == 0)
        then
          break ; /* Invalid XML, avoid infinite loop */
      phdr->seed_record_type = 'D' ;
      phdr->continuation_record = ' ' ;
      phdr->sequence.seed_num = pcom->records_written + 1 ;
      inc(pcom->records_written) ;
      memcpy (addr(phdr->location_id), addr(pcl->location), sizeof(tlocation)) ;
      memcpy (addr(phdr->channel_id), addr(pcl->seedname), sizeof(tseed_name)) ;
      memcpy (addr(phdr->station_id_call_letters), addr(q660->station), sizeof(tseed_stn)) ;
      memcpy (addr(phdr->seednet), addr(q660->network), sizeof(tseed_net)) ;
      phdr->sample_rate_multiplier = 1 ;
      phdr->number_of_following_blockettes = 2 ;
      phdr->first_blockette_byte = 48 ;
      phdr->starting_time.seed_fpt = q660->data_timetag ;
      phdr->dob.blockette_type = 1000 ;
      phdr->dob.word_order = 1 ;
      phdr->dob.rec_length = RECORD_EXP ;
      phdr->dob.next_blockette = 56 ;
      p = (pbyte)((pntrint)addr(pcom->ring->rec) + 56) ;
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
      storeopaque (addr(p), addr(hdr), addr(q660->opaque_buf), size) ;
      pcom->last_blockette = pcom->blockette_index ;
      pcom->blockette_index = (56 + OPAQUE_HDR_SIZE + size + 3) and 0xFFFC ;
      p = (pbyte)addr(pcom->ring->rec) ;
      storeseedhdr (addr(p), (pvoid)addr(pcom->ring->hdr_buf), FALSE) ;
      inc(pcl->records_generated_session) ;
      pcl->last_record_generated = secsince () ;
      send_to_client (q660, q660->cfg_lcq, pcom->ring, SCD_BOTH) ;
      memclr (addr(pcom->ring->rec), LIB_REC_SIZE) ;
      memclr (addr(pcom->ring->hdr_buf), sizeof(seed_header)) ;
      pcom->blockette_index = 56 ;
      pcom->last_blockette = 48 ;
      pcom->blockette_count = 0 ;
      inc(rnum) ;
      sizeleft = sizeleft - size ;
    end
  if ((pcl->arc.amini_filter) land (pcl->arc.total_frames > 1))
    then
      flush_archive (q660, q660->cfg_lcq) ;
end


