/*  Libmsmcast client interface

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

#ifndef LIBCLIENT_H
#define LIBCLIENT_H

#include "libtypes.h"

#define VER_LIBCLIENT 1

#define MAX_MODULES 12

typedef char string250[250] ;
typedef string250 *pstring ;
typedef pointer tcontext ; /* a client doesn't need to know the contents of the context */
typedef void (*tcallback)(pointer p) ; /* prototype for all the callback procedures */

typedef struct { /* for file access */
    tcallback call_fileacc ; /* File access callback */
    pointer station_ptr ; /* opaque pointer */
} tfile_owner ;
typedef tfile_owner *pfile_owner ;

typedef struct { /* parameters for lib_create call */
    string31 msmcastid_station ; /* initial station name */
    string95 host_software ; /* host software type and version */
    string15 host_ident ; /* short host software identification */
    word opt_verbose ; /* VERB_xxxx bitmap */
    word opt_client_msgs ; /* Number of client message buffers */
    enum tliberr resp_err ; /* non-zero for error code creating context */
    tcallback call_minidata ; /* address of miniseed data callback procedure */
    tcallback call_state ; /* address of state change callback procedure */
    tcallback call_messages ; /* address of messages callback procedure */
    pfile_owner file_owner ; /* For continuity file handling */
} tpar_create ;

typedef struct { /* parameters for lib_register call */
    string250 msmcastif_address ; /* domain name or IPV4 or IPV6 interface address */
    string250 msmcastid_address ; /* domain name or IPV4 or IPV6 multiast address */
    word msmcastid_udpport ; /* udp port number */
    boolean prefer_ipv4 ; /* If both IPV6 and IPV4 addresses available, use IPV4 */
    word opt_conntime ; /* maximum connection time in minutes if non-zero */
    word opt_connwait ; /* wait this many minutes after connection time or buflevel shutdown */
//::     string15 seed_station;
//::     string15 seed_network;
} tpar_register ;

#define AC_FIRST LOGF_DATA_GAP
#define AC_LAST LOGF_CHKERR
#define INVALID_ENTRY -1 /* no data for this time period */
#define INVALID_LATENCY -66666666 /* not yet available */

enum taccdur {AD_MINUTE, AD_HOUR, AD_DAY} ;
/* Compiler doesn't understand this typedef longint taccstats[tacctype][taccdur] ; */
typedef longint taccstats[AC_LAST + 1][AD_DAY + 1] ;

typedef struct { /* operation status */
    string15 station_name ; /* network and station */
    string15 seed_station ;
    string15 seed_network ;
    taccstats accstats ; /* accumulated statistics */
    word minutes_of_stats ; /* how many minutes of data available to make hour */
    word hours_of_stats ; /* how many hours of data available to make day */
    longint data_latency ; /* data latency in seconds or INVALID_LATENCY */
    longint status_latency ; /* seconds since received status from multicast interface */
    longint runtime ; /* running time since current connection (+) or time it has been down (-) */
    longword current_ip4 ; /* current IPV4 Address of MSMCAST */
    tip_v6 current_ipv6 ; /* current IPV6 Address of MSMCAST */
    word current_port ; /* current MSMCAST UDP Port */
    boolean ipv6 ; /* TRUE if using IPV6 */
} topstat ;

typedef struct { /* format for miniseed and archival miniseed */
    tcontext context ;
    string15 station_name ; /* network and station */
    string15 seed_station;
    string15 seed_network;
    string15 seed_channel;
    string15 seed_location;
    integer rate ; /* sampling rate */
    double timestamp ; /* Time of data, corrected for any filtering */
    enum tpacket_class packet_class ; /* type of record */
    word data_size ; /* size of actual miniseed data */
    pointer data_address ; /* pointer to miniseed record */
} tminiseed_call ;
typedef tminiseed_call *pminiseed_call ;

enum tstate_type {ST_STATE, /* new operational state */
		  ST_STATUS, /* new status available */
		  ST_CFG, /* new configuration available */
		  ST_TICK, /* info has seconds, subtype has usecs */
		  ST_OPSTAT} ; /* new operational status minute */

enum tfileacc_type {FAT_OPEN,      /* Open File */
                    FAT_CLOSE,     /* Close File */
                    FAT_DEL,       /* Delete File */
                    FAT_SEEK,      /* Seek in File */
                    FAT_READ,      /* Read from File */
                    FAT_WRITE,     /* Write to File */
                    FAT_SIZE,      /* Return File size */
                    FAT_CLRDIR,    /* Clear Directory */
                    FAT_DIRFIRST,  /* Get first entry in directory */
                    FAT_DIRNEXT,   /* Following entries, -1 file handle = done */
                    FAT_DIRCLOSE} ;/* If user wants to stop the scan before done */

typedef struct { /* format for state callback */
    tcontext context ;
    enum tstate_type state_type ; /* reason for this message */
    string15 station_name ;
    longword subtype ; /* to further narrow it down */
    longword info ; /* new highest message for ST_MSG, new state for ST_STATE,
		       or new status available for ST_STATUS */
} tstate_call ;

typedef struct { /* format for messages callback */
    tcontext context ;
    longword msgcount ; /* number of messages */
    word code ;
    longword timestamp, datatime ;
    string95 suffix ;
} tmsg_call ;

typedef struct { /* format of file access callback */
  pfile_owner owner ; /* information to locate station */
  enum tfileacc_type fileacc_type ; /* reason for this message */
  boolean fault ; /* TRUE if could not complete operation */
  pchar fname ; /* pointer to filename for open & delete */
  pointer buffer ; /* buffer for read & write */
  integer options ; /* open options, transfer size */
  tfile_handle handle ; /* File handle (descriptor) */
} tfileacc_call ;

typedef struct { /* one module */
    string15 name ;
    integer ver ;
} tmodule ;

typedef tmodule tmodules[MAX_MODULES] ; /* Null name to indicate end of list */
typedef tmodules *pmodules ;

extern void lib_create_context (tcontext *ct, tpar_create *cfg) ; /* If ct = NIL return, check resp_err */
extern enum tliberr lib_destroy_context (tcontext *ct) ; /* Return error if any */
extern enum tliberr lib_register (tcontext ct, tpar_register *rpar) ;
extern enum tlibstate lib_get_state (tcontext ct, enum tliberr *err, topstat *retopstat) ;
extern void lib_change_state (tcontext ct, enum tlibstate newstate, enum tliberr reason) ;
extern pmodules lib_get_modules (void) ;
extern enum tliberr lib_flush_data (tcontext ct) ;

#endif
