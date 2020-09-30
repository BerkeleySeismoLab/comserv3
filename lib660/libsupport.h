/*   Lib660 Support routines definitions
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
    0 2017-06-11 rdr Created
*/
#ifndef libsupport_h
/* Flag this file as included */
#define libsupport_h
#define VER_LIBSUPPORT 2

#include "libtypes.h"
#include "libclient.h"

/* lib_file_open modes bit values */
#define LFO_CREATE 1 /* create new file, overwrite any existing file */
#define LFO_OPEN 2 /* open existing file */
#define LFO_READ 4 /* allow reading */
#define LFO_WRITE 8 /* allow writing */

extern const dms_type days_mth ;

extern pchar zpad (pchar s, integer lth) ;
extern double now (void) ;
extern longword lib_uround (double r) ;
extern word day_julian (word yr, word wmth, word day) ;
extern longint lib660_julian (tsystemtime *greg) ;
extern void day_gregorian (word yr, word jday, word *mth, word *day) ;
extern pchar jul_string (longint jul, pchar result) ;
extern pchar packet_time (longint jul, pchar result) ;
extern void lib660_gregorian (longint jul, tsystemtime *greg) ;
extern pchar t64tostring (t64 *val, pchar result) ;
extern boolean stringtot64 (pchar s, t64 *value) ;
extern longword get_ip_address (pchar pc, tip_v6 *ip6, boolean *is6, boolean prefer4) ;
extern word newrand (word *sum) ;

extern boolean lib_file_open (pfile_owner powner, pchar path, integer mode, tfile_handle *desc) ;
extern boolean lib_file_close (pfile_owner powner, tfile_handle desc) ;
extern boolean lib_file_seek (pfile_owner powner, tfile_handle desc, integer offset) ;
extern boolean lib_file_read (pfile_owner powner, tfile_handle desc, pointer buf, integer size) ;
extern boolean lib_file_write (pfile_owner powner, tfile_handle desc, pointer buf, integer size) ;
extern boolean lib_file_delete (pfile_owner powner, pchar path) ;
extern boolean lib_file_size (pfile_owner powner, tfile_handle desc, integer *size) ;

#endif
