



/*   Lib660 common type definitions
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
    0 2017-06-05 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
    2 2022-02-23 jms remove pascal header.
*/
/* Flag this file as included */
#ifndef libtypes_h
#define libtypes_h
#define VER_LIBTYPES 2

#include "platform.h"
#include "utiltypes.h"
#include "readpackets.h"

#define PRI_COUNT 4 /* Number of data port priorities */
#define PORT_OS 65535 /* OS assigned web/data/net server port */
#define MAXLINT 2147483647 /* maximum positive value for a longint */
#define MAX_RATE 1000 /* highest frequency for detector */
#define DETECTOR_NAME_LENGTH 31 /* Maximum number of characters in a detector name */
#define FILTER_NAME_LENGTH 31 /* Maximum number of characters in an IIR filter name */
#define FIRMAXSIZE 400

#ifndef X86_WIN32
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

/* Unknown Seismo Temperature */
#define TEMP_UNKNOWN 666

typedef char string2[3] ;
typedef char string9[10] ;
typedef double tfloat ;
typedef U8 dms_type[13] ; /* elements 1 .. 12 used for days per month */

#ifdef USE_GCC_PACKING
enum tpacket_class
{PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE}  __attribute__ ((__packed__)) ;
#else
enum tpacket_class
{PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE} ;
#endif

/*
  Tfilter_base/tfilter is a definition of a FIR filter which may be used
  multiple places
*/
typedef struct tfilter   /* coefficient storage for FIR filters */
{
    struct tfilter *link ; /* list link */
    char fname[FILTER_NAME_LENGTH + 1] ; /* name of this FIR filter */
    double coef[FIRMAXSIZE] ; /* IEEE f.p. coef's */
    I32 len ; /* actual length of filter */
    double gain ; /* gain factor of filter */
    double dly ; /* delay in samples */
    I32 dec ; /* decimation factor of filter */
} tfilter ;
typedef tfilter *pfilter ;

typedef struct   /* for gregorian calcs */
{
    U16 wyear ;
    U16 wmonth ;
    U16 dayofweek ;
    U16 wday ;
    U16 whour ;
    U16 wminute ;
    U16 wsecond ;
    U16 wmilliseconds ;
} tsystemtime ;

enum tliberr
{LIBERR_NOERR, /* No error */
 LIBERR_NOTR, /* You are not registered */
 LIBERR_INVREG, /* Invalid Registration Request */
 LIBERR_ENDTIME, /* Reached End Time */
 LIBERR_XMLERR, /* Error reading XML */
 LIBERR_THREADERR, /* Could not create thread */
 LIBERR_REGTO, /* Registration Timeout */
 LIBERR_STATTO, /* Status Timeout */
 LIBERR_DATATO, /* Data Timeout */
 LIBERR_NOSTAT, /* Your requested status is not yet available */
 LIBERR_INVSTAT, /* Your requested status in not a valid selection */
 LIBERR_CFGWAIT, /* Your requested configuration is not yet available */
 LIBERR_BUFSHUT, /* Shutdown due to reaching buffer percentage */
 LIBERR_CONNSHUT, /* Shutdown due to reaching buffer percentage */
 LIBERR_CLOSED, /* Closed by host */
 LIBERR_NETFAIL, /* Networking Failure */
 LIBERR_INVCTX
} ; /* Invalid Context */
enum tlibstate
{LIBSTATE_IDLE, /* Not connected to Q330 */
 LIBSTATE_TERM, /* Terminated */
 LIBSTATE_CONN, /* TCP Connect wait */
 LIBSTATE_REQ, /* Requesting Registration */
 LIBSTATE_RESP, /* Registration Response */
 LIBSTATE_XML, /* Reading Configuration */
 LIBSTATE_CFG, /* Configuring */
 LIBSTATE_RUNWAIT, /* Waiting for command to run */
 LIBSTATE_RUN, /* Running */
 LIBSTATE_DEALLOC, /* De-allocating structures */
 LIBSTATE_WAIT
} ; /* Waiting for a new registration */
#endif
