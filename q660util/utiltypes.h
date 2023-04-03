



/*   Q660 Utility Types
     Copyright 2016-2021 by
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
    0 2016-02-05 rdr Created.
    1 2017-06-09 rdr tsource moved here from ssifc.h
    2 2019-06-20 rdr Add GDS_GPS data source.
    3 2021-07-13 rdr Add Dust Devices to tsource.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef UTILTYPES_H
#define UTILTYPES_H
#define VER_UTILTYPES 4

#ifndef platform_h
#include "platform.h"
#endif

#define SENS_CHANNELS 6   /* Number of channels connected to an external sensor */
#define MAIN_CHANNELS 7   /* Number of digitizer input channels */
#define CAL_CHANNEL 6     /* Calibration channel is last one */
#define ACC_CHANNELS 3    /* Number of accelerometer input channels */
#define TOTAL_CHANNELS 10 /* 6 Main, 3 Acc., 1 CalMon */
#define FREQS 10          /* Maximum number of frequencies available */
#define DUST_COUNT 8      /* Max number of DUST devices */
#define DUST_CHANNELS 16  /* Maximum number of DUST channels per device */

typedef void *pointer ;
typedef char *pchar ;
typedef void *pvoid ;
typedef U8 *PU8 ;
typedef U16 *PU16 ;
typedef I32 *PI32 ;
typedef U32 *PU32 ;
typedef U32 t64[2] ;    /* sixty four unsigned bit fields */
typedef U32 t128[4] ;   /* 128 bit fields */
typedef U32 t512[16] ;  /* MD5 temporary buffer */
typedef U8 tip_v6[16] ;    /* 128 bit IPV6 address */
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

/* General Data sources, corresponds to first byte in a data blockette */
enum tgdsrc
{GDS_TIM,  /* Timing */
 GDS_MD,   /* Main Digitizer */
 GDS_CM,   /* Calibration Monitor */
 GDS_AC,   /* Accelerometer */
 GDS_SOH,  /* State of Health */
 GDS_ENG,  /* Engineering Data */
 GDS_DUST, /* Dust Channels */
 GDS_GPS,  /* GPS Data Streams */
 GDS_DEC,  /* Decimate */
 GDS_LOG,  /* Data Logger */
 GDS_SIZE
};/* To get number of entries */
/* Message Sources */
enum tsource
{SRC_SS,         /* System Supervisor */
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
 SRC_DD1,        /* Dust Device 1 */
 SRC_DD2,        /* Dust Device 2 */
 SRC_DD3,        /* Dust Device 3 */
 SRC_DD4,        /* Dust Device 4 */
 SRC_DD5,        /* Dust Device 5 */
 SRC_DD6,        /* Dust Device 6 */
 SRC_DD7,        /* Dust Device 7 */
 SRC_DD8,        /* Dust Device 8 */
 SRC_COUNT
} ;    /* To get count */


#endif
