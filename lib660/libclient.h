



/*   Lib660 structures relating to Q660 communications
     Copyright 2017 by
     Kinemetrics, Inc.
     Pasadena, CA 91107 USA.

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
    0 2017-06-06 rdr Created.
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
    2 2022-03-01 jms implement throttle (V1 only) and BSL options. 
    3 2022-04-01 jms added BW fill    
}
*/
#ifndef libclient_h
/* Flag this file as included */
#define libclient_h
#define VER_LIBCLIENT 9

#include "utiltypes.h"
#include "readpackets.h"
#include "xmlseed.h"
#include "libtypes.h"

/* miniseed filtering bit masks */
#define OMF_ALL 1 /* bit set to send all miniseed data */
#define OMF_CFG 4 /* pass configuration opaque blockettes */
#define OMF_TIM 8 /* pass timing records */
#define OMF_MSG 16 /* pass message records */
/* Starting time reserved values */
#define OST_NEW 0  /* Start with newest data */
#define OST_LAST 1 /* Start with last data received, if available */
#define MAX_LCQ 200 /* maximum number of lcqs that can be reported */
#define MAX_DETSTAT 40 /* maximum number that library will return */
#define MAX_MODULES 28

#define LMODE_BSL 1 /* opt_client_mode: enable BSL start-time enhancements */

typedef char string250[250] ;
typedef string250 *pstring ;
typedef pointer tcontext ; /* a client doesn't need to know the contents of the context */
typedef void (*tcallback)(pointer p) ; /* prototype for all the callback procedures */

typedef struct   /* for file access */
{
    tcallback call_fileacc ; /* File access callback */
    pointer station_ptr ; /* opaque pointer */
} tfile_owner ;
typedef tfile_owner *pfile_owner ;

typedef struct   /* parameters for lib_create call */
{
    t64 q660id_serial ; /* serial number */
    U16 q660id_priority ; /* Priority from 1 to 4 */
    string7 q660id_station ; /* initial station name */
    string95 host_software ; /* host software type and version */
    string15 host_ident ; /* short host software identification */
    U16 opt_verbose ; /* VERB_xxxx bitmap */
    U16 opt_client_msgs ; /* Number of client message buffers */
    U16 opt_minifilter ; /* OMF_xxx bits */
    U16 opt_aminifilter ; /* OMF_xxx bits */
    U16 amini_exponent ; /* 2**exp size of archival miniseed, range of 9 to 14 */
    int amini_512highest ; /* rates up to this value are updated every 512 bytes */
    U16 mini_embed ; /* 1 = embed calibration and event blockettes into data */
    U16 mini_separate ; /* 1 = generate separate calibration and event records */
    pfilter mini_firchain ; /* FIR filter chain for decimation */
    tcallback call_minidata ; /* address of miniseed data callback procedure */
    tcallback call_aminidata ; /* address of archival miniseed data callback procedure */
    enum tliberr resp_err ; /* non-zero for error code creating context */
    tcallback call_state ; /* address of state change callback procedure */
    tcallback call_messages ; /* address of messages callback procedure */
    tcallback call_secdata ; /* address of one second data callback procedure */
    tcallback call_lowlatency ; /* address of low latency data callback procedure */
    tcallback call_eos ; /* address of end of second callback procedure */
    pfile_owner file_owner ; /* For continuity file handling */
} tpar_create ;

typedef struct   /* parameters for lib_register call */
{
    string63 q660id_pass ; /* password */
    string250 q660id_address ; /* domain name or IPV4 or IPV6 address */
    U16 q660id_baseport ; /* base port number */
    BOOLEAN spare1 ; /* was low_lat */
    BOOLEAN prefer_ipv4 ; /* If both IPV6 and IPV4 addresses available, use IPV4 */
    BOOLEAN start_newer ; /* TRUE to accept newer data than what is in opt_start */
    U16 opt_dynamic_ip ; /* 1 = dynamic IP address */
    U16 opt_hibertime ; /* hibernate time in minutes if non-zero */
    U16 opt_conntime ; /* maximum connection time in minutes if non-zero */
    U16 opt_connwait ; /* wait this many minutes after connection time or buflevel shutdown */
    U16 opt_regattempts ; /* maximum registration attempts before hibernate if non-zero */
    U16 opt_ipexpire ; /* dynamic IP address expires after this many minutes since last POC */
    U16 opt_disc_latency ; /* terminate connection when data latency reaches this value if
                             non-zero */
    U16 opt_q660_cont ; /* Determines how often Q660 continuity is written to disk in minutes */
    U16 opt_maxsps ; /* Highest SPS desired */
    U16 opt_token ; /* POC Token if non-zero */
    U16 opt_nro ; /* Network retry override */
    U16 spare2 ;
    U32 opt_start ; /* Determines when to start */
    U32 opt_end ; /* Determines when to end if non-zero */
    U32 opt_client_mode ; /* enable BSL alternate start time management */
    U32 opt_limit_last_backfill ; /* Limit backfill sec if opt_start = OST_LAST. 0 -> no limit */
    U32 opt_throttle_kbitpersec ; /* XDL only. if non-zero, throttle speed during streaming */

    U32 opt_bwfill_kbit_target ; /* XDL. gratuitous constant bandwidth target rate. kbit/s */
    U32 opt_bwfill_probe_interval ; /* XDL. interval (s) at which fil is retried after latency exceeded */
    U32 opt_bwfill_exceed_trigger ; /* XDL. N latency exceeds reduces to estimate of available rate */
    U32 opt_bwfill_increase_interval ; /* XDL. interval (s) of reduced rate at which max fill is retried */
    U32 opt_bwfill_max_latency ; /* XDL. max seconds latency at receiver allowed before fill stopped */
    
} tpar_register ;

typedef struct   /* format of lib_change_conntiming */
{
    U16 opt_conntime ; /* maximum connection time in minutes if non-zero */
    U16 opt_connwait ; /* wait this many minutes after connection time or buflevel shutdown */
    U16 opt_disc_latency ; /* terminate connection when data latency reaches this value if
                             non-zero */
    U16 data_timeout ; /* timeout in minutes for data timeout (default is 10) */
    U16 data_timeout_retry ; /* minutes to wait after data timeout (default is 10) */
    U16 status_timeout ; /* timeout in minutes for status timeout (default is 5) */
    U16 status_timeout_retry ; /* minutes to wait after status timeout (default is 5) */
    U16 piu_retry ; /* minutes to wait after port in use timeout (default is 5 */
    U16 opt_nro ; /* Network retry override */
} tconntiming ;

#define AC_FIRST LOGF_DATA_GAP
#define AC_LAST LOGF_CHKERR
#define INVALID_ENTRY -1 /* no data for this time period */
#define INVALID_LATENCY -66666666 /* not yet available */

enum taccdur
{AD_MINUTE, AD_HOUR, AD_DAY} ;
/* Compiler doesn't understand this typedef longint taccstats[tacctype][taccdur] ; */
typedef I32 taccstats[AC_LAST + 1][AD_DAY + 1] ;

typedef struct   /* operation status */
{
    string9 station_name ; /* network and station */
    U16 station_prio ; /* data priority */
    U32 station_tag ; /* tagid */
    t64 station_serial ; /* Q660 serial number */
    U32 station_reboot ; /* time of last reboot */
    taccstats accstats ; /* accumulated statistics */
    U16 minutes_of_stats ; /* how many minutes of data available to make hour */
    U16 hours_of_stats ; /* how many hours of data available to make day */
    U8 gpio1 ; /* General Purpose Input 1 percent active */
    U8 gpio2 ; /* General Purpose Input 2 percent active */
    I32 data_latency ; /* data latency in seconds or INVALID_LATENCY */
    I32 status_latency ; /* seconds since received status from Q660 or INVALID_LATENCY */
    I32 runtime ; /* running time since current connection (+) or time it has been down (-) */
    U32 totalgaps ; /* total number of data gaps since context created */
    U8 pkt_full ; /* percent of Q660 packet buffer full */
    U16 clock_qual ; /* Percent clock quality */
    U16 active_digis ; /* Bitmap of active digitizers */
    I32 clock_drift ; /* Clock drift from GPS in microseconds */
    int mass_pos[BOOM_COUNT] ; /* mass positions */
    U8 pgagains[SENS_CHANNELS] ; /* A/D PGA gain */
    int calibration_errors ; /* calibration error bitmap */
    float sys_temp ; /* Q660 temperature in degrees C */
    float pwr_volt ; /* Q660 power supply voltage in volts */
    float pwr_cur ; /* Q660 power supply current in amps */
    I32 gps_age ; /* age in seconds of last GPS clock update, -1 for never updated */
    enum tgpspwr gps_pwr ; /* GPS Power Status */
    enum tgpsfix gps_fix ; /* GPS Fix */
    enum tpllstat pll_stat ; /* PLL Status */
    float gps_lat ; /* Latitude */
    float gps_long ; /* Longitude */
    float gps_elev ; /* Elevation */
    U32 last_data_time ; /* Latest data received, 0 for none */
    U32 current_ip4 ; /* current IPV4 Address of Q660 */
    tip_v6 current_ipv6 ; /* current IPV6 Address of Q660 */
    U16 current_port ; /* current Q660 UDP Port */
    BOOLEAN ipv6 ; /* TRUE if using IPV6 */
} topstat ;

typedef struct   /* For low latency callback */
{
    U32 total_size ; /* number of bytes in buffer passed */
    tcontext context ;
    string9 station_name ; /* network and station */
    string2 location ;
    string3 channel ;
    U16 sample_count ; /* Number of samples */
    int rate ; /* sampling rate */
    U32 reserved ; /* must be zero */
    U32 reserved2 ;
    double timestamp ; /* Time of data, corrected for any filtering */
    U16 qual_perc ; /* time quality percentage */
    U16 activity_flags ; /* same as in Miniseed */
    U16 io_flags ; /* same as in Miniseed */
    U16 data_quality_flags ; /* same as in Miniseed */
    enum tgdsrc src_gen ; /* source general type */
    U8 src_subchan ; /* source blockette sub-channel */
    I32 samples[MAX_RATE] ; /* decompressed samples */
} tlowlat_call ;
typedef tlowlat_call tonesec_call ; /* Same for one second data */

enum tminiseed_action
{MSA_512, /* new 512 byte packet */
 MSA_ARC, /* new archival packet, non-incremental */
 MSA_FIRST, /* new archival packet, incremental */
 MSA_INC, /* incremental update to archival packet */
 MSA_FINAL, /* final incremental update */
 MSA_GETARC, /* request for last archival packet written */
 MSA_RETARC
} ; /* client is returning last packet written */

typedef struct   /* format for miniseed and archival miniseed */
{
    tcontext context ;
    string9 station_name ; /* network and station */
    string2 location ;
    U8 chan_number ; /* channel number according to tokens */
    string3 channel ;
    int rate ; /* sampling rate */
    double timestamp ; /* Time of data, corrected for any filtering */
    U16 filter_bits ; /* OMF_xxx bits */
    enum tpacket_class packet_class ; /* type of record */
    enum tminiseed_action miniseed_action ; /* what this packet represents */
    U16 data_size ; /* size of actual miniseed data */
    pointer data_address ; /* pointer to miniseed record */
} tminiseed_call ;
typedef tminiseed_call *pminiseed_call ;

enum tstate_type
{ST_STATE, /* new operational state */
 ST_STATUS, /* new status available */
 ST_CFG, /* new configuration available */
 ST_TICK, /* info has seconds, subtype has usecs */
 ST_OPSTAT
} ; /* new operational status minute */

enum tfileacc_type
{FAT_OPEN,      /* Open File */
 FAT_CLOSE,     /* Close File */
 FAT_DEL,       /* Delete File */
 FAT_SEEK,      /* Seek in File */
 FAT_READ,      /* Read from File */
 FAT_WRITE,     /* Write to File */
 FAT_SIZE,      /* Return File size */
 FAT_CLRDIR,    /* Clear Directory */
 FAT_DIRFIRST,  /* Get first entry in directory */
 FAT_DIRNEXT,   /* Following entries, -1 file handle = done */
 FAT_DIRCLOSE
} ;/* If user wants to stop the scan before done */

typedef struct   /* format for state callback */
{
    tcontext context ;
    enum tstate_type state_type ; /* reason for this message */
    string9 station_name ;
    U32 subtype ; /* to further narrow it down */
    U32 info ; /* new highest message for ST_MSG, new state for ST_STATE,
                      or new status available for ST_STATUS */
} tstate_call ;

typedef struct   /* format for messages callback */
{
    tcontext context ;
    U32 msgcount ; /* number of messages */
    U16 code ;
    U32 timestamp, datatime ;
    string95 suffix ;
} tmsg_call ;

typedef struct   /* format of file access callback */
{
    pfile_owner owner ; /* information to locate station */
    enum tfileacc_type fileacc_type ; /* reason for this message */
    BOOLEAN fault ; /* TRUE if could not complete operation */
    pchar fname ; /* pointer to filename for open & delete */
    pointer buffer ; /* buffer for read & write */
    int options ; /* open options, transfer size */
    tfile_handle handle ; /* File handle (descriptor) */
} tfileacc_call ;

typedef struct   /* format of data provided by received POC */
{
    tip_v6 new_ipv6_address ; /* new dynamic IP address if IPV6 */
    U32 new_ip_address ; /* new dynamic IP address if IPV4 */
    BOOLEAN ipv6 ; /* TRUE to use IPV6 */
    U16 new_base_port ; /* Q660 base port */
    U16 poc_token ; /* to be passed in the registration call */
    string95 log_info ; /* any additional information the POC receiver wants logged */
} tpocmsg ;

typedef struct   /* format of one detector status entry */
{
    char name[DETECTOR_NAME_LENGTH + 11] ;
    BOOLEAN ison ; /* last record was detected on */
    BOOLEAN declared ; /* ison filtered with first */
    BOOLEAN first ; /* if this is the first detection after startup */
    BOOLEAN enabled ; /* currently enabled */
} tonedetstat ;
typedef struct   /* format of the result */
{
    int count ; /* number of valid entries */
    tonedetstat entries[MAX_DETSTAT] ;
} tdetstat ;
typedef struct   /* record to change detector enable */
{
    char name[DETECTOR_NAME_LENGTH + 11] ;
    BOOLEAN run_detector ;
} tdetchange ;

typedef struct   /* format of one lcq status entry */
{
    string2 location ;
    U8 chan_number ; /* channel number according to XML */
    string3 channel ;
    I32 rec_cnt ; /* number of records */
    I32 rec_age ; /* number of seconds since update */
    I32 rec_seq ; /* current record sequence */
    I32 det_count ; /* number of detections */
    I32 cal_count ; /* number of calibrations */
    I32 arec_cnt ; /* number of archive new records */
    I32 arec_over ; /* number of archive overwritten records */
    I32 arec_age ; /* since last update */
    I32 arec_seq ; /* current record sequence */
    string31 desc ; /* description of channel */
} tonelcqstat ;
typedef struct   /* format of the result */
{
    int count ; /* number of valid entries */
    tonelcqstat entries[MAX_LCQ] ;
} tlcqstat ;

typedef struct   /* format of essential items from XML */
{
    string9 station_name ;
    U16 buffer_counts[MAX_LCQ] ; /* pre-event buffers + 1 */
} tdpcfg ;

typedef struct   /* one module */
{
    string15 name ;
    int ver ;
} tmodule ;

typedef tmodule tmodules[MAX_MODULES] ; /* Null name to indicate end of list */
typedef tmodules *pmodules ;

extern void lib_create_context (tcontext *ct, tpar_create *cfg) ; /* If ct = NIL return, check resp_err */
extern enum tliberr lib_destroy_context (tcontext *ct) ; /* Return error if any */
extern enum tliberr lib_register (tcontext ct, tpar_register *rpar) ;
extern enum tlibstate lib_get_state (tcontext ct, enum tliberr *err, topstat *retopstat) ;
extern void lib_change_state (tcontext ct, enum tlibstate newstate, enum tliberr reason) ;
extern void lib_request_status (tcontext ct, U32 bitmap, U16 interval) ;
extern enum tliberr lib_get_status (tcontext ct, enum tstype stat_type, pointer buf) ;
extern enum tliberr lib_get_sysinfo (tcontext ct, tsysinfo *sinfo, tseed *sseed) ;
extern U16 lib_change_verbosity (tcontext ct, U16 newverb) ;
extern void lib_send_usermessage (tcontext ct, pchar umsg) ;
extern void lib_poc_received (tcontext ct, tpocmsg *poc) ;
extern enum tliberr lib_get_detstat (tcontext ct, tdetstat *detstat) ;
extern enum tliberr lib_get_lcqstat (tcontext ct, tlcqstat *lcqstat) ;
extern void lib_change_enable (tcontext ct, tdetchange *detchange) ;
extern enum tliberr lib_get_dpcfg (tcontext ct, tdpcfg *dpcfg) ;
extern void lib_msg_add (tcontext ct, U16 msgcode, U32 dt, pchar msgsuf) ;
extern pmodules lib_get_modules (void) ;
extern enum tliberr lib_conntiming (tcontext ct, tconntiming *conntiming, BOOLEAN setter) ;
extern I32 lib_crccalc (PU8 p, I32 len) ;
extern enum tliberr lib_set_freeze_timer (tcontext ct, int seconds) ;
extern enum tliberr lib_flush_data (tcontext ct) ;
#endif
