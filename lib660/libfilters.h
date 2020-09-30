/*   Lib660 Filter Definitions
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
    0 2017-06-06 rdr Created
*/
#ifndef libfilters_h
/* Flag this file as included */
#define libfilters_h
#define VER_LIBFILTERS 0

#include "utiltypes.h"
#include "xmlseed.h"
#include "libtypes.h"
#include "libsampglob.h"
#include "libclient.h"
#include "libstrucs.h"

typedef double *pdouble ;

extern void load_firfilters (pq660 q660) ;
extern void append_firfilters (pq660 q660, pfilter src) ;
extern pfir_packet create_fir (pq660 q660, pfilter src) ;
extern piirfilter create_iir (pq660 q660, piirdef src, integer points) ;
extern void allocate_lcq_filters (pq660 q660, plcq q) ;
extern tfloat mac_and_shift (pfir_packet pf) ;
extern pfilter find_fir (pq660 q660, pchar name) ;
extern double multi_section_filter (piirfilter resp, double s) ;
extern void calc_section (tsect_base *sect) ;
extern void bwsectdes (pdouble a, pdouble b, integer npoles, boolean high, tfloat ratio) ;

#endif
