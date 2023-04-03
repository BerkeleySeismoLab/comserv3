



/*   Lib660 Message Log definitions
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
    0 2017-06-08 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef liblogs_h
/* Flag this file as included */
#define liblogs_h
#define VER_LIBLOGS 5

#include "libtypes.h"
#include "libstrucs.h"
#include "libsampglob.h"

enum tclock_exception
{CE_VALID, CE_DAILY, CE_JUMP} ;

extern void lib660_padright (pchar s, pchar b, int fld) ;
extern void log_clock (pq660 q660, enum tclock_exception clock_exception, pchar jump_amount) ;
extern void finish_log_clock (pq660 q660) ;
extern void logevent (pq660 q660, pdet_packet det, tonset_mh *onset) ;
extern void log_cal (pq660 q660, PU8 pb, BOOLEAN start) ;
extern void log_message (pq660 q660, pchar msg) ;
extern void flush_messages (pq660 q660) ;
extern void flush_timing (pq660 q660) ;
extern pchar two (U16 w, pchar s) ;

#endif
