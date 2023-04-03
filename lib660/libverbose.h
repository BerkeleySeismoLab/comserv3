



/*   Lib660 Status Dump Definitions
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
#ifndef libverbose_h
/* Flag this file as included */
#define libverbose_h
#define VER_LIBVERBOSE 6

#include "libtypes.h"
#include "libstrucs.h"

typedef string31 tgstats[] ;

extern const tgstats GPWRS ;
extern const tgstats GFIXS ;
extern const tgstats PSTATS ;

extern void log_all_info (pq660 q660) ;
extern U32 print_generated_rectotals (pq660 q660) ;

#endif
