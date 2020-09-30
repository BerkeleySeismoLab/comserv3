/*  Lib660 Command Processing headers

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
    Copyright 2020 Berkeley Seismological Laboratory, University of California

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
*/

#ifndef LIBCMDS_h
#define LIBCMDS_h
#define VER_LIBCMDS 9

#include "libtypes.h"
#include "libstrucs.h"

extern void lib_start_registration (pmsmcast msmcast) ;
extern void lib_continue_registration (pmsmcast msmcast) ;
extern void read_mcast_socket (pmsmcast msmcast) ;
extern void lib_timer (pmsmcast msmcast) ;
extern void start_deallocation (pmsmcast msmcast) ;
extern void change_status_request (pmsmcast msmcast) ;
extern void send_user_message (pmsmcast msmcast, pchar msg) ;
extern void tcp_error (pmsmcast msmcast, pchar msgsuf) ;

#endif
