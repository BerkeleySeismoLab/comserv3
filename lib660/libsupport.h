



/*   Lib660 Support routines definitions
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
#ifndef libsupport_h
/* Flag this file as included */
#define libsupport_h
#define VER_LIBSUPPORT 5

#include "libtypes.h"
#include "libclient.h"

/* lib_file_open modes bit values */
#define LFO_CREATE 1 /* create new file, overwrite any existing file */
#define LFO_OPEN 2 /* open existing file */
#define LFO_READ 4 /* allow reading */
#define LFO_WRITE 8 /* allow writing */

extern const dms_type days_mth ;

extern pchar zpad (pchar s, int lth) ;
extern double now (void) ;
extern U32 lib_uround (double r) ;
extern U16 day_julian (U16 yr, U16 wmth, U16 day) ;
extern I32 lib660_julian (tsystemtime *greg) ;
extern void day_gregorian (U16 yr, U16 jday, U16 *mth, U16 *day) ;
extern pchar jul_string (I32 jul, pchar result) ;
extern pchar packet_time (I32 jul, pchar result) ;
extern void lib660_gregorian (I32 jul, tsystemtime *greg) ;
extern pchar t64tostring (t64 *val, pchar result) ;
extern BOOLEAN stringtot64 (pchar s, t64 *value) ;
extern U32 get_ip_address (pchar pc, tip_v6 *ip6, BOOLEAN *is6, BOOLEAN prefer4) ;
extern U16 newrand (U16 *sum) ;

extern BOOLEAN lib_file_open (pfile_owner powner, pchar path, int mode, tfile_handle *desc) ;
extern BOOLEAN lib_file_close (pfile_owner powner, tfile_handle desc) ;
extern BOOLEAN lib_file_seek (pfile_owner powner, tfile_handle desc, int offset) ;
extern BOOLEAN lib_file_read (pfile_owner powner, tfile_handle desc, pointer buf, int size) ;
extern BOOLEAN lib_file_write (pfile_owner powner, tfile_handle desc, pointer buf, int size) ;
extern BOOLEAN lib_file_delete (pfile_owner powner, pchar path) ;
extern BOOLEAN lib_file_size (pfile_owner powner, tfile_handle desc, int *size) ;

#endif
