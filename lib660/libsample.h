



/*   Lib660 Time Series data routine definitions
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
    0 2017-06-11 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef libsample_h
/* Flag this file as included */
#define libsample_h
#define VER_LIBSAMPLE 7

#include "libtypes.h"
#include "libstrucs.h"
#include "libsampglob.h"

extern I32 sex (I32 l) ;
extern void process_one (pq660 q660, plcq q, I32 data) ;
extern void process_compressed (pq660 q660, PU8 p, int chan_offset,
                                U32 seq) ;
extern I32 seqspread (U32 new_, U32 last) ;
extern void finish_record (pq660 q660, plcq q, pcom_packet pcom) ;
extern void flush_lcq (pq660 q660, plcq q, pcom_packet pcom) ;
extern void flush_lcqs (pq660 q660) ;
extern void flush_dplcqs (pq660 q660) ;
extern void add_blockette (pq660 q660, plcq q, PU16 pw, double time) ;
extern void send_to_client (pq660 q660, plcq q, pcompressed_buffer_ring pbuf, U8 dest) ;
extern void build_separate_record (pq660 q660, plcq q, PU16 pw,
                                   double time, enum tpacket_class pclass) ;

#endif
