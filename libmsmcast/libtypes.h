/*  Libmcast common type definitions

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
    Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#ifndef LIBTYPES_H
#define LIBTYPES_H
#define VER_LIBTYPES 0

#include "platform.h"

#define PRI_COUNT 4 /* Number of data port priorities */
#define PORT_OS 65535 /* OS assigned web/data/net server port */
#define MAXLINT 2147483647 /* maximum positive value for a longint */
#define MAX_RATE 1000 /* highest frequency for detector */
#define FIRMAXSIZE 400

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

// Start include from pascal.h
//:: #define land &&
//:: #define lor ||
//:: #define lnot !
//:: #define div /
//:: #define mod %
//:: #define and &
//:: #define or |
//:: #define xor ^
//:: #define not ~
#define NIL 0
#define addr &
#define shl <<
#define shr >>
#define inc(val) (val)++
#define dec(val) (val)--
#define incn(val,off) (val = val + (off))
#define decn(val,off) (val = val - (off))
// End include from pascal.h

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
typedef char string2[3] ;
typedef char string9[10] ;
typedef double tfloat ;
typedef byte dms_type[13] ; /* elements 1 .. 12 used for days per month */

#ifdef USE_GCC_PACKING
enum tpacket_class {PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE}  __attribute__ ((__packed__)) ;
#else
enum tpacket_class {PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE} ;
#endif

typedef struct { /* for gregorian calcs */
    word wyear ;
    word wmonth ;
    word dayofweek ;
    word wday ;
    word whour ;
    word wminute ;
    word wsecond ;
    word wmilliseconds ;
} tsystemtime ;

enum tliberr {
    LIBERR_NOERR, /* No error */
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
    LIBERR_INVCTX, /* Invalid Context */
} ; 

enum tlibstate {
    LIBSTATE_IDLE, /* Not connected to Q330 */
    LIBSTATE_TERM, /* Terminated */
    LIBSTATE_CONN, /* TCP Connect wait */
    LIBSTATE_REQ, /* Requesting Registration */
    LIBSTATE_RESP, /* Registration Response */
    LIBSTATE_XML, /* Reading Configuration */
    LIBSTATE_CFG, /* Configuring */
    LIBSTATE_RUNWAIT, /* Waiting for command to run */
    LIBSTATE_RUN, /* Running */
    LIBSTATE_DEALLOC, /* De-allocating structures */
    LIBSTATE_WAIT, /* Waiting for a new registration */
} ;

// Start: from xmlseed.h
/* GDS_LOG sub-fields */
enum tlogfld {
    LOGF_MSGS,      /* Messages */
    LOGF_CFG,       /* Configuration */
    LOGF_TIME,      /* Timing Blockettes */
    LOGF_DATA_GAP,  /* Data Gaps */
    LOGF_REBOOTS,   /* Re-Boots */
    LOGF_RECVBPS,   /* Received Bps */
    LOGF_SENTBPS,   /* Sent Bps */
    LOGF_COMMATMP,  /* Communication Attepts */
    LOGF_COMMSUCC,  /* Communication Successes */
    LOGF_PACKRECV,  /* Packets Received */
    LOGF_COMMEFF,   /* Communications Efficiency */
    LOGF_POCSRECV,  /* POC's Received */
    LOGF_IPCHANGE,  /* IP Address Changes */
    LOGF_COMMDUTY,  /* Communications Duty Cycle */
    LOGF_THRPUT,    /* Throughput */
    LOGF_MISSDATA,  /* Missing Data */
    LOGF_SEQERR,    /* Sequence Errors */
    LOGF_CHKERR,    /* Checksum Errors */
    LOGF_NDATLAT,   /* Normal Data Latency */
    LOGF_LDATLAT,   /* Low Latency Data Latency */
    LOGF_STATLAT,   /* Status Latency */
    LOGF_SIZE,    /* To get number of fields */
} ;
typedef string31 tlogfields[LOGF_SIZE] ;
// End: from xmlseed.h

#endif
