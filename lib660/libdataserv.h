/*   Lib660 Dataserver definitions
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
#ifndef libdataserv_h
/* Flag this file as included */
#define libdataserv_h
#define VER_LIBDATASERV 3

#include "libtypes.h"
#include "libstrucs.h"
#include "libclient.h"
#include "libseed.h"

#define MAX_DS_BUFFERS 16 /* around 65K */

typedef tlowlat_call tdsbuf[MAX_DS_BUFFERS] ;
typedef struct {
  word ds_port ;         /* TCP port number */
  integer server_number ;
  integer record_count ; /* number of records allocated */
  pointer stnctx ;       /* station context */
  tdsbuf *dsbuf ;        /* pointer to circular buffer */
} tds_par ;
typedef tlowlat_call *plowlat_call ;

extern pointer lib_ds_start (tds_par *dspar) ;
extern void lib_ds_stop (pointer ct) ;
extern void lib_ds_send (pointer ct, plowlat_call pbuf) ;
extern void process_ll (pq660 q660, pbyte p, integer chan_offset, 
                        longword sec) ;

#endif
