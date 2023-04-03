



/*   Lib660 Command Processing headers
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
    0 2017-06-07 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef libcmds_h
/* Flag this file as included */
#define libcmds_h
#define VER_LIBCMDS 20

#include "utiltypes.h"
#include "libtypes.h"
#include "libstrucs.h"

extern void lib_start_registration (pq660 q660) ;
extern void lib_continue_registration (pq660 q660) ;
extern void read_cmd_socket (pq660 q660) ;
extern void lib_timer (pq660 q660) ;
extern void start_deallocation (pq660 q660) ;
extern void change_status_request (pq660 q660) ;
extern void send_user_message (pq660 q660, pchar msg) ;
extern void tcp_error (pq660 q660, pchar msgsuf) ;
extern void disconnect_me (pq660 q660) ;

#endif
