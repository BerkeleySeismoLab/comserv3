/*   Q660 Utility Types
     Copyright 2016, 2017, 2019 Certified Software Corporation

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
    0 2016-02-05 rdr Created.
    1 2017-06-09 rdr tsource moved here from ssifc.h
    2 2019-06-20 rdr Add GDS_GPS data source.
*/
#ifndef UTILTYPES_H
#define UTILTYPES_H
#define VER_UTILTYPES 2

#ifndef pascal_h
#include "pascal.h"
#endif

#define SENS_CHANNELS 6   /* Number of channels connected to an external sensor */
#define MAIN_CHANNELS 7   /* Number of digitizer input channels */
#define CAL_CHANNEL 6     /* Calibration channel is last one */
#define ACC_CHANNELS 3    /* Number of accelerometer input channels */
#define TOTAL_CHANNELS 10 /* 6 Main, 3 Acc., 1 CalMon */
#define FREQS 10          /* Maximum number of frequencies available */

typedef void *pointer ;
typedef char *pchar ;
typedef void *pvoid ;
typedef byte *pbyte ;
typedef word *pword ;
typedef longint *plong ;
typedef longword *plword ;
typedef longword t64[2] ;    /* sixty four unsigned bit fields */
typedef longword t128[4] ;   /* 128 bit fields */
typedef longword t512[16] ;  /* MD5 temporary buffer */
typedef byte tip_v6[16] ;    /* 128 bit IPV6 address */
typedef char string3[4] ;
typedef char string7[8] ;
typedef char string15[16] ;
typedef char string23[24] ;
typedef char string31[32] ;
typedef char string47[48] ;
typedef char string63[64] ;
typedef char string71[72] ;
typedef char string95[96] ;
typedef char string127[128] ;
typedef char string[256] ;

/* TEMPORARY - CBuilderX can't compile this program due to lack of
   IPV6 support, but try to get as far as possible */
#ifdef CBUILDERX
typedef struct sockaddr_in6 { /* Fake a sockaddr_in6 structure */
  word sin6_family ;
  word sin6_port ;
  longword sin6_flowinfo ;
  tip_v6 sin6_addr ;
} SOCKADDR_IN6 ;
#endif

/* General Data sources, corresponds to first byte in a data blockette */
enum tgdsrc {GDS_TIM,  /* Timing */
             GDS_MD,   /* Main Digitizer */
             GDS_CM,   /* Calibration Monitor */
             GDS_AC,   /* Accelerometer */
             GDS_SOH,  /* State of Health */
             GDS_ENG,  /* Engineering Data */
             GDS_DUST, /* Dust Channels */
             GDS_GPS,  /* GPS Data Streams */
             GDS_DEC,  /* Decimate */
             GDS_LOG,  /* Data Logger */
             GDS_SIZE};/* To get number of entries */
/* Message Sources */
enum tsource {SRC_SS,         /* System Supervisor */
              SRC_WS,         /* Web Server */
              SRC_BE,         /* Back-End */
              SRC_IDL1,       /* Internal Data Logger Priority 1 */
              SRC_IDL2,       /* Internal Data Logger Priority 2 */
              SRC_IDL3,       /* Internal Data Logger Priority 3 */
              SRC_IDL4,       /* Internal Data Logger Priority 4 */
              SRC_XDL1,       /* External Data Logger Priority 1 */
              SRC_XDL2,       /* External Data Logger Priority 2 */
              SRC_XDL3,       /* External Data Logger Priority 3 */
              SRC_XDL4,       /* External Data Logger Priority 4 */
              SRC_COUNT} ;    /* To get count */


#endif
