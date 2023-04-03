



/*   Lib660 internal data structures
     Copyright 2006-2021 by
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
    1 2018-08-29 rdr Dispatch tables moved to area cleared upon de-registration
                     to avoid holdover Q pointers from previous connection that are
                     now not defined.
    2 2021-01-27 rdr New variables in tq660 for XML to BE buffering.
    3 2021-04-04 rdr Add Dust variables.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
    5 2022-03-22 jms add local time stamp of last received packet to be used to 
                        inform be660 of receiver latency.

}*/
#ifndef libstrucs_h
/* Flag this file as included */
#define libstrucs_h
#define VER_LIBSTRUCS 5

#include "utiltypes.h"
#include "memutil.h"
#include "readpackets.h"
#include "xmlsup.h"
#include "xmlcfg.h"
#include "xmlseed.h"
#include "libtypes.h"
#include "libsampglob.h"
#include "libclient.h"
#include "libseed.h"

#define CMDQSZ 32 /* Maximum size of command queue */
#define TCPBUFSZ 2000
#define MAXRESPLINES 5
#define MAXXML 200000
#define MAXSPREAD 128 /* now that we have the reboot time saved.. */
#define INITIAL_ACCESS_TIMOUT 60 /* Less than 1 minute poweron is pointless */
#define DEFAULT_PIU_RETRY 5 * 60 /* Port in Use */
#define DEFAULT_DATA_TIMEOUT 5 * 60 /* Data timeout */
#define DEFAULT_DATA_TIMEOUT_RETRY 5 * 60 ;
#define DEFAULT_STATUS_TIMEOUT 5 * 60 /* Status timeout */
#define DEFAULT_STATUS_TIMEOUT_RETRY 5 * 60 ;
#define MIN_MSG_QUEUE_SIZE 10 /* Minimum number of client message buffers */
#define BUFSOCKETSIZE 4000 /* For building buffer of multiple XML strings */

typedef I32 taccminutes[60] ;
typedef struct   /* for building one dp statistic */
{
    I32 accum ;
    I32 accum_ds ; /* for datastream use */
    pointer ds_lcq ; /* lcq associated with this statistic, if any */
    taccminutes minutes ;
    I32 hours[24] ;
} taccmstat ;
typedef taccmstat taccmstats[AC_LAST + 1] ;
typedef U8 tcbuf[DEFAULT_MEM_INC] ; /* continuity buffer, can't exceed a memory segment */

typedef struct tcont_cache   /* Linked list of q660 continuity segments */
{
    struct tcont_cache *next ; /* next block */
    PU8 payload ; /* address of payload */
    int size ; /* current payload size */
    int allocsize ; /* size of this allocation */
    /* contents follow */
} tcont_cache ;
typedef tcont_cache *pcont_cache ;

typedef U16 tmymask[TOTAL_CHANNELS + 1] ; /* Last entry for engineering data */

typedef char txmlbuf[MAXXML + 1] ;

typedef struct   /* shared variables with client. Require mutex protection */
{
    enum tliberr liberr ; /* last error condition */
    enum tlibstate target_state ; /* state the client wants */
    int stat_minutes ;
    int stat_hours ;
    I32 total_minutes ;
    taccmstats accmstats ;
    topstat opstat ; /* operation status */
    U16 first_share_clear ; /* start of shared fields cleared after de-registration */
    U32 extra_status ; /* client wants for status than default */
    U32 have_status ; /* this status is currently available */
    U32 have_config ; /* have configuration info from q660 bitmap */
    U32 want_config ; /* bitmap of perishable configuration requested */
    int status_interval ; /* status request interval in seconds */
    int freeze_timer ; /* if > 0 then don't process data from q660 */
    BOOLEAN usermessage_requested ; /* client wants a user message sent */
    tstat_sm stat_sm ;  /* Station Monitor Status */
    tstat_gps stat_gps ; /* GPS Status */
    tstat_pll stat_pll ; /* PLL Status */
    tstat_logger stat_logger ; /* Logger Status */
    tstat_dust stat_dust ; /* Dust Status */
    tstat_dustlat stat_dustlat ; /* Dust newest data time */
    tsysinfo sysinfo ; /* System Info */
    tseed seed ; /* Station network, station, and configuration */
    tdigis digis ; /* Digitizer Info */
    tuser_message user_message ; /* as requested by client */
    tuser_message usermsg ; /* user message received */
    U16 last_share_clear ; /* last address cleared after de-registration */
} tshare ;

typedef struct   /* this is the actual context which is hidden from clients */
{
#ifdef X86_WIN32
    HANDLE mutex ;
    HANDLE msgmutex ;
    HANDLE threadhandle ;
    U32 threadid ;
#else
    pthread_mutex_t mutex ;
    pthread_mutex_t msgmutex ;
    pthread_t threadid ;
#endif
    enum tlibstate libstate ; /* current state of this station */
    BOOLEAN terminate ; /* set TRUE to terminate thread */
    BOOLEAN needtosayhello ; /* set TRUE to generate created message */
    BOOLEAN q660cont_updated ; /* Has been updated since writing */
    tseed_net network ;
    tseed_stn station ;
    tpar_create par_create ; /* parameters to create context */
    tpar_register par_register ; /* registration parameters */
    tmeminst connmem ; /* Connection oriented memory */
    tmeminst thrmem ; /* Station oriented memory */
    pcont_cache conthead ; /* head of active segments */
    pcont_cache contfree ; /* head of inactive but available segments */
    pcont_cache contlast ; /* last active segment during creation */
    tshare share ; /* variables shared with client */
    pointer lastuser ;
    U32 msg_count ; /* message count */
    int reg_wait_timer ; /* stay in wait state until decrements to zero */
    int dynip_age ; /* age in seconds */
    int reg_tries ; /* number of times we have tried to register */
    int minute_counter ; /* 6 * 10 seconds = 60 seconds */
    int data_timeout ; /* threshold seconds for data_timer */
    int data_timeout_retry ; /* seconds to wait before retrying after data timeout */
    int status_timeout ; /* threshold seconds for status_timer */
    int status_timeout_retry ; /* seconds to wait before retrying after status timeout */
    int piu_retry ; /* seconds to wait before retrying after port in use error */
    int retry_index ; /* To get number of seconds from RETRY_TAB */
    I32 last_ten_sec ; /* last ten second value */
    double last_100ms ; /* last time ran 100ms timer routine */
    double saved_data_timetag ; /* for latency calculations */
    double q660_cont_written ; /* last time q660 continuity was written to disk */
    double boot_time ; /* for DSS use */
    U32 dpstat_timestamp ; /* for dp statistics */
    U16 cur_verbosity ; /* current verbosity */
    U16 rsum ; /* random number static storage */
    tcbuf *cbuf ; /* continuity buffer */
    BOOLEAN media_error ; /* for continuity writing */
#ifdef X86_WIN32
    SOCKET cpath ; /* commands socket */
#else
    int cpath ; /* commands socket */
    int high_socket ; /* Highest socket number */
#endif
    struct sockaddr csock ; /* For IPV4 */
    struct sockaddr_in6 csock6 ; /* For IPV6 */
    tstate_call state_call ; /* buffer for building state callbacks */
    tmsg_call msg_call ; /* buffer for building message callbacks */
    tonesec_call onesec_call ; /* buffer for building one second callbacks */
    tlowlat_call lowlat_call ; /* buffer for building low latency callbacks */
    tminiseed_call miniseed_call ; /* buffer for building miniseed callbacks */
    int arc_size ; /* size of archival mini-seed records */
    int arc_frames ; /* number of frames in an archival record */
    double ref2016 ; /* for converting between system time and since 2016 time */
    double last_status_received ; /* last time status was received */
    string contmsg ; /* any errors from continuity checking */
    string9 station_ident ; /* network-station */
    pchan chanchain ;   /* Linked list of channels */
    pchan dlchain ;     /* And for data logger channels */
    pointer data_latency_lcq ; /* dp lcq for data latency */
    pointer low_latency_lcq ; /* dp lcq for low latency latency */
    pointer status_latency_lcq ; /* dp lcq for status latency */
    pmsgqueue msgqueue, msgq_in, msgq_out ;
    pfilter firchain ; /* start of fir filter chain */
    int log_timer ; /* count-down since last added message line */
    plcq msg_lcq ;
    plcq cfg_lcq ;
    plcq tim_lcq ;
    plcq dplcqs ;
    /* following are cleared after de-registering */
    U16 first_clear ; /* first byte to clear */
    U16 timercnt ; /* count up getting one second intervals from 100ms */
    U16 logstat_timer ; /* count up for 1 minute interval */
    U32 q660ip ; /* current working IPV4 address */
    tip_v6 q660ip6 ; /* current working IPV6 address */
    BOOLEAN ipv6 ; /* TRUE if using IPV6 */
    BOOLEAN registered ; /* registered with q660 */
    BOOLEAN balesim ; /* baler simulation */
    BOOLEAN stalled_link ; /* link is currently stalled */
    BOOLEAN link_recv ; /* have link parameters from q660 */
    BOOLEAN need_regmsg ; /* need a registration user message */
    BOOLEAN need_sats ; /* need satellite status for clock logging */
    BOOLEAN nested_log ; /* we are actually writing a log record */
    BOOLEAN flush_all ; /* flush DA and DP LCQ's */
    BOOLEAN initial_status ; /* Initial Status Request for status dump */
    BOOLEAN status_change_pending ; /* Change status after status received */
    BOOLEAN sensora_ok ; /* Valid Sensor A */
    BOOLEAN sensorb_ok ; /* Valid Sensor B */
    BOOLEAN within_second ; /* We are within a second */
    BOOLEAN discon_pending ; /* Waiting for end of second to disconnect */
    int comtimer ; /* timeout for above action */
    int reg_timer ; /* registration timeout */
    int data_timer ; /* since got data */
    I32 conn_timer ; /* seconds in run state */
    int status_timer ; /* count up for seconds since status */
    int calstat_clear_timer ; /* 100ms countdown to cal status clear */
    U32 last_sent_count, lastds_sent_count ; /* packets sent at start of this minute */
    U32 last_resent_count, lastds_resent_count ; /* packets resent at start of this minute */
    txmlline srclines[MAXRESPLINES] ;
    txmlcopy xmlcopy ;
    int linecount ;
    int xmlsize ;
    txmlbuf *xmlbuf ;
    tqdp qdphdr ;
    U8 qdp_data[MAXDATA] ;
    string250 hash ;
    char tcpbuf[TCPBUFSZ] ;
    int tcpidx ;
    int lastidx ;
    U16 last_packet ;
    double lasttime ;
    U32 lastseq ;
    BOOLEAN contingood ; /* continuity good */
    BOOLEAN first_data ;
    U32 dt_data_sequence ; /* global data record sequence number */
    plcq lcqs ;    /* first lcq from this server */
    U8 calerr_bitmap ;
    U8 highest_lcqnum ; /* highest LCQ number from tokens */
    U16 last_data_qual ;
    U16 data_qual ; /* 0-100% */
    U16 ll_data_qual ;
    double data_timetag ;
    double ll_data_timetag ;
    double ll_offset ;   /* From current second */
    BOOLEAN daily_done ; /*daily timemark has been done*/
    double last_update ; /* time of last clock update */
    I32 except_count ; /* for timing blockette exception_count */
    I32 max_spread ; /* Max seconds between data packets */
    piirdef xiirchain ;  /* linked list of IIR detectors */
    double cfg_lastwritten ;
    U16 total_detectors ;
    char opaque_buf[NONDATA_AREA] ;
    ttimeblk time_block ; /* Timing header from digitizer */
    ttimeblk ll_time_block ; /* Timing header for low latency */
    tlcq ll_lcq ; /* For decompressing LL blockettes */
    timing timing_buf ; /* need a place to keep this between log_clock and finish_log_clock */
    tcompressed_buffer_ring detcal_buf ; /* used for building event and calibration only records */
    tmymask mymask ; /* Bitmask of channels/freqs to turn off */
    ttimdispatch timdispatch ;
    tsohdispatch sohdispatch ;
    tengdispatch engdispatch ;
    tgpsdispatch gpsdispatch ;
    tdustdispatch dustdispatch ;
    tmdispatch mdispatch ;
    char xml_buf[BUFSOCKETSIZE] ; /* String buffer for XML sent to BE */
    int xml_buf_lth ; /* Length of string buffer */
    double last_data_packet_received_local_timestamp ;
    U32 last_data_packet_received_embedded_timestamp ;
    U16 last_clear ; /* end of clear */
} tq660 ;
typedef tq660 *pq660 ;

extern void lock (pq660 q660) ;
extern void unlock (pq660 q660) ;
extern void msglock (pq660 q660) ;
extern void msgunlock (pq660 q660) ;
extern void sleepms (int ms) ;
extern void lib_create_660 (tcontext *ct, tpar_create *cfg) ;
extern enum tliberr lib_destroy_660 (tcontext *ct) ;
extern enum tliberr lib_register_660 (pq660 q660, tpar_register *rpar) ;
extern void new_state (pq660 q660, enum tlibstate newstate) ;
extern void new_cfg (pq660 q660, U32 newbitmap) ;
extern void new_status (pq660 q660, U32 newbitmap) ;
extern void state_callback (pq660 q660, enum tstate_type stype, U32 val) ;
extern U32 make_bitmap (U32 bit) ;
extern void set_liberr (pq660 q660, enum tliberr newerr) ;
#endif
