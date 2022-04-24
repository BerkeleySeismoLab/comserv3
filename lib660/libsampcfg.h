/*   Lib660 time series configuration definitions
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
#ifndef libsampcfg_h
/* Flag this file as included */
#define libsampcfg_h
#define VER_LIBSAMPCFG 6

#include "libtypes.h"
#include "libstrucs.h"
#include "libsampglob.h"
#include "libclient.h"

extern void deallocate_sg (pq660 q660) ;
extern void set_gaps (plcq q) ;
extern void set_net_station (pq660 q660) ;
extern void init_lcq (pq660 q660) ;
extern void process_dplcqs (pq660 q660) ;
extern void init_dplcqs (pq660 q660) ;
extern pchar realtostr (double r, integer digits, pchar result) ;
extern longword secsince (void) ;
extern enum tliberr lib_lcqstat (pq660 q660, tlcqstat *lcqstat) ;
extern enum tliberr lib_getdpcfg (pq660 q660, tdpcfg *dpcfg) ;
extern void clear_calstat (pq660 q660) ;
extern void init_ll_lcq (pq660 q660) ;

#endif
