/*   Lib660 Data Packet Processing headers
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
    0 2017-06-09 rdr Created
*/
#ifndef libdata_h
/* Flag this file as included */
#define libdata_h
#define VER_LIBDATA 3

#include "utiltypes.h"
#include "libtypes.h"
#include "libstrucs.h"

/* GPS Status values */
#define GPS_OFFLOCK 0 /* Powered off due to GPS lock */
#define GPS_OFFPLL 1 /* Powered off due to PLL lock */
#define GPS_OFFMAX 2 /* Powered off due to maximum time */
#define GPS_OFFCMD 3 /* Powered off due to command */
#define GPS_COLD 4 /* Coldstart, see parameter 2 for reason */
#define GPS_ONAUTO 5 /* Powered on automatically */
#define GPS_ONCMD 6 /* Powered on by command */

extern void process_data (pq660 q660, pbyte p, integer lth) ;
extern void process_low_latency (pq660 q660, pbyte p, integer lth) ;

#endif
