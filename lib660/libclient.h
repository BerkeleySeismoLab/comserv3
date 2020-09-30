/*   Lib660 structures relating to Q660 communications
     Copyright 2017 Certified Software Corporation

    This file is part of Lib660

    Lib660 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Lib660 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of,
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2017-06-06 rdr Created
}
*/
#ifndef libclient_h
/* Flag this file as included */
#define libclient_h
#define VER_LIBCLIENT 3

#include "utiltypes.h"
#include "readpackets.h"
#include "xmlseed.h"
#include "libtypes.h"

/* miniseed filtering bit masks */
#define OMF_ALL 1 /* bit set to send all miniseed data */
#define OMF_CFG 4 /* pass configuration opaque blockettes */
#define OMF_TIM 8 /* pass timing records */
#define OMF_MSG 16 /* pass message records */
/* Staring time reseverd values */
#define OST_NEW 0  /* Start with newest data */
#define OST_LAST 1 /* Start with last data received, if available */

#define MAX_LCQ 200 /* maximum number of lcqs that can be reported */
#define MAX_DETSTAT 40 /* maximum number that library will return */
#define MAX_MODULES 28

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
  t64 q660id_serial ; /* serial number */
  word q660id_priority ; /* Priority from 1 to 4 */
  string7 q660id_station ; /* initial station name */
  string95 host_software ; /* host software type and version */
  string15 host_ident ; /* short host software identification */
  word opt_verbose ; /* VERB_xxxx bitmap */
  word opt_client_msgs ; /* Number of client message buffers */
  word opt_minifilter ; /* OMF_xxx bits */
  word opt_aminifilter ; /* OMF_xxx bits */
  word amini_exponent ; /* 2**exp size of archival miniseed, range of 9 to 14 */
  integer amini_512highest ; /* rates up to this value are updated every 512 bytes */
  word mini_embed ; /* 1 = embed calibration and event blockettes into data */
  word mini_separate ; /* 1 = generate separate calibration and event records */
  pfilter mini_firchain ; /* FIR filter chain for decimation */
  tcallback call_minidata ; /* address of miniseed data callback procedure */
  tcallback call_aminidata ; /* address of archival miniseed data callback procedure */
  enum tliberr resp_err ; /* non-zero for error code creating context */
  tcallback call_state ; /* address of state change callback procedure */
  tcallback call_messages ; /* address of messages callback procedure */
  tcallback call_secdata ; /* address of one second data callback procedure */
  tcallback call_lowlatency ; /* address of low latency data callback procedure */
  pfile_owner file_owner ; /* For continuity file handling */
} tpar_create ;

typedef struct { /* parameters for lib_register call */
  string63 q660id_pass ; /* password */
  string250 q660id_address ; /* domain name or IPV4 or IPV6 address */
  word q660id_baseport ; /* base port number */
  boolean low_lat ; /* TRUE to enable Low Latency handling */
  boolean prefer_ipv4 ; /* If both IPV6 and IPV4 addresses available, use IPV4 */
  boolean start_newer ; /* TRUE to accept newer data than what is in opt_start */
  word opt_dynamic_ip ; /* 1 = dynamic IP address */
  word opt_hibertime ; /* hibernate time in minutes if non-zero */
  word opt_conntime ; /* maximum connection time in minutes if non-zero */
  word opt_connwait ; /* wait this many minutes after connection time or buflevel shutdown */
  word opt_regattempts ; /* maximum registration attempts before hibernate if non-zero */
  word opt_ipexpire ; /* dynamic IP address expires after this many minutes since last POC */
  word opt_disc_latency ; /* terminate connection when data latency reaches this value if
                             non-zero */
  word opt_q660_cont ; /* Determines how often Q660 continuity is written to disk in minutes */
  word opt_maxsps ; /* Highest SPS desired */
  word opt_token ; /* POC Token if non-zero */
  longword opt_start ; /* Determines when to start */
  longword opt_end ; /* Determines when to end if non-zero */
#ifdef BSL
  longword opt_limit_last_backfill ; /* Limit seconds of backfill when opt_start = OST_LAST. 0 -> no limit */
#endif
  string15 opt_lookback ; /* Low Latency Lookback value in string form */
} tpar_register ;

typedef struct { /* format of lib_change_conntiming */
  word opt_conntime ; /* maximum connection time in minutes if non-zero */
  word opt_connwait ; /* wait this many minutes after connection time or buflevel shutdown */
  word opt_disc_latency ; /* terminate connection when data latency reaches this value if
                             non-zero */
  word data_timeout ; /* timeout in minutes for data timeout (default is 10) */
  word data_timeout_retry ; /* minutes to wait after data timeout (default is 10) */
  word status_timeout ; /* timeout in minutes for status timeout (default is 5) */
  word status_timeout_retry ; /* minutes to wait after status timeout (default is 5) */
  word piu_retry ; /* minutes to wait after port in use timeout (default is 5 */
} tconntiming ;

#define AC_FIRST LOGF_DATA_GAP
#define AC_LAST LOGF_CHKERR
#define INVALID_ENTRY -1 /* no data for this time period */
#define INVALID_LATENCY -66666666 /* not yet available */

enum taccdur {AD_MINUTE, AD_HOUR, AD_DAY} ;
/* Compiler doesn't understand this typedef longint taccstats[tacctype][taccdur] ; */
typedef longint taccstats[AC_LAST + 1][AD_DAY + 1] ;

typedef struct { /* operation status */
  string9 station_name ; /* network and station */
  word station_prio ; /* data priority */
  longword station_tag ; /* tagid */
  t64 station_serial ; /* Q660 serial number */
  longword station_reboot ; /* time of last reboot */
  taccstats accstats ; /* accumulated statistics */
  word minutes_of_stats ; /* how many minutes of data available to make hour */
  word hours_of_stats ; /* how many hours of data available to make day */
  byte gpio1 ; /* General Purpose Input 1 percent active */
  byte gpio2 ; /* General Purpose Input 2 percent active */
  longint data_latency ; /* data latency in seconds or INVALID_LATENCY */
  longint status_latency ; /* seconds since received status from Q660 or INVALID_LATENCY */
  longint runtime ; /* running time since current connection (+) or time it has been down (-) */
  longword totalgaps ; /* total number of data gaps since context created */
  byte pkt_full ; /* percent of Q660 packet buffer full */
  word clock_qual ; /* Percent clock quality */
  word active_digis ; /* Bitmap of active digitizers */
  longint clock_drift ; /* Clock drift from GPS in microseconds */
  integer mass_pos[BOOM_COUNT] ; /* mass positions */
  byte pgagains[SENS_CHANNELS] ; /* A/D PGA gain */
  integer calibration_errors ; /* calibration error bitmap */
  single sys_temp ; /* Q660 temperature in degrees C */
  single pwr_volt ; /* Q660 power supply voltage in volts */
  single pwr_cur ; /* Q660 power supply current in amps */
  longint gps_age ; /* age in seconds of last GPS clock update, -1 for never updated */
  enum tgpspwr gps_pwr ; /* GPS Power Status */
  enum tgpsfix gps_fix ; /* GPS Fix */
  enum tpllstat pll_stat ; /* PLL Status */
  single gps_lat ; /* Latitude */
  single gps_long ; /* Longitude */
  single gps_elev ; /* Elevation */
  longword last_data_time ; /* Latest data received, 0 for none */
  longword current_ip4 ; /* current IPV4 Address of Q660 */
  tip_v6 current_ipv6 ; /* current IPV6 Address of Q660 */
  word current_port ; /* current Q660 UDP Port */
  boolean ipv6 ; /* TRUE if using IPV6 */
} topstat ;

typedef struct { /* For low latency callback */
  longword total_size ; /* number of bytes in buffer passed */
  tcontext context ;
  string9 station_name ; /* network and station */
  string2 location ;
  string3 channel ;
  word sample_count ; /* Number of samples */
  integer rate ; /* sampling rate */
  longword reserved ; /* must be zero */
  longword reserved2 ;
  double timestamp ; /* Time of data, corrected for any filtering */
  word qual_perc ; /* time quality percentage */
  word activity_flags ; /* same as in Miniseed */
  word io_flags ; /* same as in Miniseed */
  word data_quality_flags ; /* same as in Miniseed */
  enum tgdsrc src_gen ; /* source general type */
  byte src_subchan ; /* source blockette sub-channel */
  longint samples[MAX_RATE] ; /* decompressed samples */
} tlowlat_call ;
typedef tlowlat_call tonesec_call ; /* Same for one second data */

enum tminiseed_action {MSA_512, /* new 512 byte packet */
                      MSA_ARC, /* new archival packet, non-incremental */
                      MSA_FIRST, /* new archival packet, incremental */
                      MSA_INC, /* incremental update to archival packet */
                      MSA_FINAL, /* final incremental update */
                      MSA_GETARC, /* request for last archival packet written */
                      MSA_RETARC} ; /* client is returning last packet written */

typedef struct { /* format for miniseed and archival miniseed */
  tcontext context ;
  string9 station_name ; /* network and station */
  string2 location ;
  byte chan_number ; /* channel number according to tokens */
  string3 channel ;
  integer rate ; /* sampling rate */
  double timestamp ; /* Time of data, corrected for any filtering */
  word filter_bits ; /* OMF_xxx bits */
  enum tpacket_class packet_class ; /* type of record */
  enum tminiseed_action miniseed_action ; /* what this packet represents */
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
  string9 station_name ;
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

typedef struct { /* format of data provided by received POC */
  tip_v6 new_ipv6_address ; /* new dynamic IP address if IPV6 */
  longword new_ip_address ; /* new dynamic IP address if IPV4 */
  boolean ipv6 ; /* TRUE to use IPV6 */
  word new_base_port ; /* Q660 base port */
  word poc_token ; /* to be passed in the registration call */
  string95 log_info ; /* any additional information the POC receiver wants logged */
} tpocmsg ;

typedef struct { /* format of one detector status entry */
  char name[DETECTOR_NAME_LENGTH + 11] ;
  boolean ison ; /* last record was detected on */
  boolean declared ; /* ison filtered with first */
  boolean first ; /* if this is the first detection after startup */
  boolean enabled ; /* currently enabled */
} tonedetstat ;
typedef struct { /* format of the result */
  integer count ; /* number of valid entries */
  tonedetstat entries[MAX_DETSTAT] ;
} tdetstat ;
typedef struct { /* record to change detector enable */
  char name[DETECTOR_NAME_LENGTH + 11] ;
  boolean run_detector ;
} tdetchange ;

typedef struct { /* format of one lcq status entry */
  string2 location ;
  byte chan_number ; /* channel number according to XML */
  string3 channel ;
  longint rec_cnt ; /* number of records */
  longint rec_age ; /* number of seconds since update */
  longint rec_seq ; /* current record sequence */
  longint det_count ; /* number of detections */
  longint cal_count ; /* number of calibrations */
  longint arec_cnt ; /* number of archive new records */
  longint arec_over ; /* number of archive overwritten records */
  longint arec_age ; /* since last update */
  longint arec_seq ; /* current record sequence */
  string31 desc ; /* description of channel */
} tonelcqstat ;
typedef struct { /* format of the result */
  integer count ; /* number of valid entries */
  tonelcqstat entries[MAX_LCQ] ;
} tlcqstat ;

typedef struct { /* format of essential items from XML */
  string9 station_name ;
  word buffer_counts[MAX_LCQ] ; /* pre-event buffers + 1 */
} tdpcfg ;

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
extern void lib_request_status (tcontext ct, longword bitmap, word interval) ;
extern enum tliberr lib_get_status (tcontext ct, enum tstype stat_type, pointer buf) ;
extern enum tliberr lib_get_sysinfo (tcontext ct, tsysinfo *sinfo, tseed *sseed) ;
extern word lib_change_verbosity (tcontext ct, word newverb) ;
extern void lib_send_usermessage (tcontext ct, pchar umsg) ;
extern void lib_poc_received (tcontext ct, tpocmsg *poc) ;
extern enum tliberr lib_get_detstat (tcontext ct, tdetstat *detstat) ;
extern enum tliberr lib_get_lcqstat (tcontext ct, tlcqstat *lcqstat) ;
extern void lib_change_enable (tcontext ct, tdetchange *detchange) ;
extern enum tliberr lib_get_dpcfg (tcontext ct, tdpcfg *dpcfg) ;
extern void lib_msg_add (tcontext ct, word msgcode, longword dt, pchar msgsuf) ;
extern pmodules lib_get_modules (void) ;
extern enum tliberr lib_conntiming (tcontext ct, tconntiming *conntiming, boolean setter) ;
extern longint lib_crccalc (pbyte p, longint len) ;
extern enum tliberr lib_set_freeze_timer (tcontext ct, integer seconds) ;
extern enum tliberr lib_flush_data (tcontext ct) ;
#endif
