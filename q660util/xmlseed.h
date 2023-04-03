



/*   XML Read Configuration definitions
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
    0 2016-01-05 rdr Created
    1 2016-05-08 rdr Swap positions of GDS_DUST and GDS_ENG to match docs.
    2 2016-07-27 rdr SOHF_GPOUT added.
    3 2017-04-17 rdr SOHF_PKTPERC added.
    4 2018-04-06 rdr Description added to tchan.
    5 2018-05-07 rdr Add BLOCK_MASK for client requested blocking of data packets.
    6 2018-05-23 rdr Change parameters for STA/LTA detector.
    7 2018-06-25 rdr Remove Ext GPS current, change Ext GPS voltage to antenna volts.
    8 2021-02-03 rdr Add new GPS fields to engineering data.
    9 2021-04-04 rdr Add DUST data fields.
   10 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef XMLSEED_H
#define XMLSEED_H
#define VER_XMLSEED 19

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif
#ifndef XMLCFG_H
#include "xmlcfg.h"
#endif

#ifdef SECT_SEED

#define MAXSECTIONS 3 /* Maximum number of sections in recursive filters, practically it's 2 */
#define MAX_DET 2 /* Maximum number of detectors per channel */
#define MAX_DEC 26 /* Maximum number of decimated channels */
#define IIRCNT 11
#define DLCHCNT 5

/* tiirdef is a definition of an IIR filter */
typedef struct
{
    U8 poles ;
    BOOLEAN highpass ;
    U16 spare1 ;
    float ratio ; /* ratio * sampling_frequency = corner */
} tsection_base ;
typedef struct tiirdef
{
    struct tiirdef *link ;  /* room for forward link */
    U8 sects ;
    U8 spare1 ;
    U16 spare2 ;
    float gain ; /* filter gain */
    float rate ; /* reference frequency */
    string31 fname ;
    tsection_base filt[MAXSECTIONS] ;
} tiirdef ;
typedef tiirdef *piirdef ;
extern piirdef xiirchain ; /* Linked list of IIR detectors */
extern tiirdef curiir ;
extern const txmldef XIIR[IIRCNT] ;
extern const txmldef XDLCHAN[DLCHCNT] ;

enum tdettype
{DT_NONE, DT_STA_LTA, DT_THRESHOLD} ;

/* tstaltacfg defines operating constants for sta-lta detectors */
#define SLCNT 11
typedef struct
{
    float ratio ;      /* Trigger ratio */
    float quiet ;      /* More trigger info */
    U16 sta_win ;      /* STA window in seconds */
    U16 lta_mult ;     /* LTA window as a multiple of sta_win */
    U16 pre_event ;    /* Pre-event time in seconds */
    U16 post_event ;   /* Post-event time in seconds */
} tstaltacfg ;
extern const txmldef XSTALTA[SLCNT] ;
/* tthreshcfg defines operating constants for threshold detectors */
#define THRCNT 11
typedef struct
{
    I32 lo_thresh ; /* Lower threshold in counts */
    I32 hi_thresh ; /* Upper threshold in counts */
    I32 hysteresis ;/* Hysteresis in counts */
    I32 window ;    /* Minimum duration in samples */
    U16 pre_event ;    /* Pre-event time in seconds */
    U16 post_event ;   /* Post-event time in seconds */
    U32 spare ;
} tthreshcfg ;
extern const txmldef XTHRESH[THRCNT] ;
/* tdetector defines a detector of either type */
typedef struct tdetector
{
    string31 dname ;    /* detector name */
    string31 fname ;    /* Filter name */
    piirdef fptr ;      /* Pointer to IIR filter */
    tthreshcfg cfg ;    /* detector parameters, can also alias to tstaltacfg */
    enum tdettype dtype ; /* detector type */
    BOOLEAN run ;       /* Run this detector */
    U8 std_num ;      /* Non-zero for a standard detector (such as OVERTEMP) */
    U8 spare1 ;
} tdetector ;
typedef tdetector *pdetector ;
extern tdetector curdet ;
extern tstaltacfg slcfg ;
extern tthreshcfg thrcfg ;

/* GDS_TIM sub-fields */
enum ttimfld
{TIMF_PH,   /* Phase */
 TIMF_CQP,  /* Clock Quality Percent */
 TIMF_CLM,  /* Clock Loss Minutes */
 TIMF_SIZE
};/* To get number of fields */

typedef string31 ttimfields[TIMF_SIZE] ;
extern const ttimfields TIMFIELDS ;

/* GDS_MD, GDS_CM, GDS_AC are in the same blockette format, channels
   and frequencies are encoded in the second byte of the blockette */

/* GDS_SOH sub-fields */
enum tsohfld
{SOHF_ANTCUR,   /* Antenna Current */
 SOHF_SENSACUR, /* Sensor A Current */
 SOHF_SENSBCUR, /* Sensor B Current */
 SOHF_BOOM1,    /* Channel 1 Boom input */
 SOHF_BOOM2,    /* Channel 2 Boom input */
 SOHF_BOOM3,    /* Channel 3 Boom input */
 SOHF_BOOM4,    /* Channel 4 Boom input */
 SOHF_BOOM5,    /* Channel 5 Boom input */
 SOHF_BOOM6,    /* Channel 6 Boom input */
 SOHF_SYSTMP,   /* System Temperature */
 SOHF_HUMIDITY, /* Humidity */
 SOHF_INPVOLT,  /* Input Voltage */
 SOHF_VCOCTRL,  /* VCO Control */
 SOHF_NEGAN,    /* Negative Analog Voltage */
 SOHF_ISODC,    /* Isolated DC Voltage */
 SOHF_GPIN1,    /* General Purpose Input 1 */
 SOHF_GPIN2,    /* General Purpose Input 2 */
 SOHF_SYSCUR,   /* System Current */
 SOHF_UPSVOLT,  /* UPS Voltage */
 SOHF_ANTVOLT,  /* Antenna Voltage */
 SOHF_GPOUT,    /* General Purpose Output */
 SOHF_PKTPERC,  /* Packet Buffer Percent */
 SOHF_SIZE
} ;   /* To get number of fields */

typedef string31 tsohfields[SOHF_SIZE] ;
extern const tsohfields SOHFIELDS ;

/* GDS_ENG sub-fields */
enum tengfld
{ENGF_GPSSENS,  /* GPS/Sensor Power Bits */
 ENGF_GPSCTRL,  /* GPS Control Bits */
 ENGF_SIOCTRLA, /* Sensor A Digital I/O Bits */
 ENGF_SIOCTRLB, /* Sensor B Digital I/O Bits */
 ENGF_SENSIO,   /* Sensor Serial I/O Bits */
 ENGF_MISCIO,   /* Misc. I/O Bits */
 ENGF_DUSTIO,   /* Dust I/O Bits */
 ENGF_GPSHAL,   /* GPS HAL I/O Bits */
 ENGF_GPSSTATE, /* GPS State */
 ENGF_SIZE
} ;   /* To get number of fields */

typedef string31 tengfields[ENGF_SIZE] ;
extern const tengfields ENGFIELDS ;

/* GDS_GPS sub-fields */
enum tgpsfld
{GPSF_USED,       /* Number of satellites used */
 GPSF_FIXTYPE,    /* Fix Type */
 GPSF_LAT,        /* Latitude */
 GPSF_LON,        /* Longitude */
 GPSF_ELEV,       /* Elevation */
 GPSF_SIZE
} ;     /* To get number of fields */

typedef string31 tgpsfields[GPSF_SIZE] ;
extern const tgpsfields GPSFIELDS ;

/* GDS_LOG sub-fields */
enum tlogfld
{LOGF_MSGS,      /* Messages */
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
 LOGF_SIZE
} ;    /* To get number of fields */
typedef string31 tlogfields[LOGF_SIZE] ;
extern const tlogfields LOGFIELDS ;

/* tchan define one SEED channel. */
typedef struct tchan
{
    struct tchan *link ;  /* room for forward link */
    struct tchan *sortlink ; /* For program specific linking */
    struct tchan *decptr ;/* Decimation source channel pointer */
    pointer auxinfo ;     /* Program specfic auxiliary information pointer */
    pdetector detptrs[MAX_DET] ; /* Location of detector parameters */
    string7 seedname ;    /* Seed Channel Name and optional location */
    string15 source ;     /* Source string */
    string31 desc ;       /* Description */
    string7 evt_list ;    /* Event only channel list in text */
    string7 decsrc ;      /* Decimation source seed name */
    string31 excl_list ;  /* Exclusion list in text */
    string31 decfilt ;    /* Decimation FIR filter name */
    string23 units ;      /* Conversion from counts to native units */
    string47 fir_chain ;  /* Fir Filter chain */
    double delay ;        /* Filter delay in seconds */
    float rate ;         /* sample rate */
    enum tgdsrc gen_src ; /* General Source */
    U8 sub_field ;      /* sub field, any source, channel for MD and AC */
    U8 freqnum ;        /* Frequency number in FREQTAB */
    U8 evt_map ;        /* Event only channels in bitmap */
    U8 excl_map ;       /* Exclusion map */
    BOOLEAN cal_trig ;    /* TRUE to trigger on calibration */
    BOOLEAN no_output ;   /* TRUE to disable data output */
    BOOLEAN disable ;     /* TRUE to disable this channel completely */
    U8 mask ;           /* If not zero, why data is not sent to client */
    U8 spare1 ;
    U8 spare3 ;
    U16 latency ;        /* If not zero, low latency target in ms */
    U32 spare2 ;
} tchan ;
typedef tchan *pchan ;
extern tchan curchan ;
extern pchan chanchain ; /* Linked list of channels */
extern pchan dlchain ;  /* And for data logger channels */

#define PRI_MASK 1     /* channel mask value if not sent due to priority limitation */
#define CLIENT_MASK 2  /* channel mask value if not sent due to client request */
#define LOG_MASK 4     /* channel mask value if not sent due to logger configuration for this prio. */
#define BLOCK_MASK 8   /* channel mask value if not sent due to block request from client */
/* tseed is for the outer seed structure */
#define SEEDCNT 3
typedef struct
{
    string cfgname ;     /* Configuration name */
    string3 network ;    /* Network name */
    string7 station ;    /* Station name */
} tseed ;
extern tseed seedbuf ; /* Current seed buffer */
extern const txmldef XSEED[SEEDCNT] ;

#define MAXDISP 128 /* Dust is worst case, 8 devices time 16 channels */
typedef pchan txdispatch[GDS_SIZE][MAXDISP] ; /* handlers for non-main data */
typedef pchan txmdispatch[TOTAL_CHANNELS][FREQS] ; /* for main data */

extern txdispatch xdispatch ;
extern txmdispatch xmdispatch ;
extern int chan_count ;
extern int xml_file_size ;

extern BOOLEAN read_seed (void) ;
extern void add_dispatch (pchan pl) ;
extern void add_mdispatch (pchan pl, int offset) ;
extern void set_dispatch (pchan pl) ;
#endif

typedef struct          /* Structure used to keep a thread copy of XML configuration */
{
#ifdef SECT_SENS
    tsensors *psensors ;
#endif
#if defined(SECT_MAIN) || defined(SECT_ACCL)
    tdigis *pdigis ;
#endif
#ifdef SECT_SINF
    tsysinfo *psysinfo ;
#endif
#ifdef SECT_WRIT
    twritercfg *pwritercfg ;
#endif
#ifdef SECT_TIME
    ttiming *ptiming ;
#endif
#ifdef SECT_OPTS
    toptions *poptions ;
#endif
#ifdef SECT_NETW
    tnetwork *pnetwork ;
#endif
#ifdef SECT_ANNC
    tannc *pannounce ;
#endif
#ifdef SECT_AUTO
    tamass *pautomass ;
#endif
#ifdef SECT_SEED
    txdispatch *pdispatch ;
    txmdispatch *pmdispatch ;
    tseed *pseed ;
    pchan pchanchain ;
    pchan pdlchain ;
    piirdef piirchain ;
#endif
} txmlcopy ;

/* These don't depend on SECT_SEED being defined, but we need everything
   defined. */
extern void initialize_xml_reader (pmeminst pmem, BOOLEAN initmem) ;
extern BOOLEAN load_cfg_xml (pmeminst pmem, pchar name, pchar buffer) ;
extern void make_xml_copy (txmlcopy *pcpy) ;

#endif
