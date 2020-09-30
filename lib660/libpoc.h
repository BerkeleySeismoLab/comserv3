/*   Lib660 POC Receiver definitions
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
    0 2017-06-10 rdr Created
*/
#ifndef libpoc_h
/* Flag this file as included */
#define libpoc_h
#define VER_LIBPOC 0

#include "utiltypes.h"
#include "readpackets.h"
#include "libtypes.h"
#include <errno.h>

typedef struct { /* Format of callback parameter */
  tip_v6 ip6_address ; /* new dynamic IP address for IPV6 */
  longword ip_address ; /* new dynamic IP address for IPV4 */
  boolean ipv6 ; /* TRUE for IPV6 */
  tpoc_message msg ; /* Message received from Q660 */
} tpoc_recvd ;
enum tpocstate {PS_NEWPOC, PS_CONNRESET} ;
typedef void (*tpocproc)(enum tpocstate pocstate, tpoc_recvd *poc_recv) ;
typedef struct { /* parameters for POC receiver */
  boolean ipv6 ; /* TRUE to use IPV6 */
  word poc_port ; /* UDP port to listen on */
  tpocproc poc_callback ; /* procedure to call when poc received */
} tpoc_par ;

extern pointer lib_poc_start (tpoc_par *pp) ;
extern void lib_poc_stop (pointer ctr) ;

#endif
