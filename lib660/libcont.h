/*   Lib660 Continuity definitions
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
    0 2017-06-08 rdr Created
*/
#ifndef libcont_h
/* Flag this file as included */
#define libcont_h
#define VER_LIBCONT 0

#include "libtypes.h"
#include "libstrucs.h"

extern void restore_thread_continuity (pq660 q660, boolean pass1, pchar result) ;
extern void save_continuity (pq660 q660) ;
extern void save_thread_continuity (pq660 q660) ;
extern void check_continuity (pq660 q660) ;
extern boolean restore_continuity (pq660 q660) ;
extern void purge_continuity (pq660 q660) ;
extern void purge_thread_continuity (pq660 q660) ;
extern void continuity_timer (pq660 q660) ;
#endif
