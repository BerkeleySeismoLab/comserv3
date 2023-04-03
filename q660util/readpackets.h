



/*
    Q660 Data & Status Record Constant and Type definitions
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

REV  DATE        BY   CHANGE
-----------------------------------------------------------------------
  0  2016-07-09  rdr  Created.
  1  2017-02-21  rdr  Double sec_offset removed from ttimeblk and tstat_sm.
                      Replaced with usec_offset in ttimeblk and tstat_sm and
                      sec_offset also added to tstat_sm. GDS_TIM_LENGTH updated.
  2  2018-10-27  rdr  Add ST_IDL and tstat_idl.
  3  2019-01-08  rdr  Update station monitor flags bit definitions.
  4  2021-03-20  rdr  Add DUST definitions.
  5  2021-12-24  rdr  Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/

#ifndef READPACKETS_H
#define READPACKETS_H
#define VER_READPACKETS 17

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

#define BOOM_COUNT 6    /* Number of Boom Channels */
#define SENSOR_COUNT 2  /* Number of external sensors */
#define MAX_SAT 16      /* Maximum number of satellites for status reporting */

/* QDP Definitions */
#define QDP_VER 3
#define MAXDATA 1024   /* For actual data */
#define QDP_HDR_LTH 4  /* in actual network traffic bytes */
#define CMD_MASK 0x1F  /* To get command from QDP command/version field */

/* Data header flag bit */
#define DHOF_MASK 0x1F    /* To get offset bits */
#define DHOF_MORE 0x20    /* More data coming for this second */
#define DHOF_OFF 0x40     /* A sample offset is included */
#define DHOF_PREV 0x80    /* A previous value is included */
/* Timing header */
#define TIM_USEC_MASK 0xFFFFFF /* LS 24 bits used for usec_offset */
#define TIM_FLAG_LEAP 0x1000000 /* Bit 24 is leap second flag */
/* Intermittent Blockettes */
#define IB_GPSPH 0x10     /* GPS Status Change */
#define IB_DIGPH 0x11     /* Digitizer Phase Change */
#define IB_LEAP 0x12      /* Leap Second detected */
#define IB_CALEVT 0x18    /* Calibration Event */
#define IB_DRIFT 0x20     /* Drift out of range */
#define IB_CAL_START 0x30 /* Calibration Start */
#define IB_CAL_ABORT 0x31 /* Calibration Abort */
#define IB_UMSG 0x40      /* User Message */
#define IB_EOS 0xFF       /* End of second */
/* GPS Coldstart Reasons, second parameter */
#define COLD_CMD        0   /* Commanded */
#define COLD_TO         1   /* Tracking Timeout */
#define COLD_INTPH      2   /* Phaseing between GPS and internal clock */
#define COLD_JUMP       3   /* Too large a jump while running */
/* Digitizer Phase Change Constants, first parameter */
#define DPC_STARTUP     0
#define DPC_WAIT        1
#define DPC_INT         2
#define DPC_EXT         3
/* Digitizer Phase Change Constants, second parameter */
#define DPR_NORM        0
#define DPR_DRIFT       1
#define DPR_GOTCLK      2
#define DPR_CMD         3
/* Status Monitor Flags */
#define SMF_CFGERR 0x1      /* Error reading configuration from EEPROM */
#define SMF_RECERR 0x2      /* Error reading records from EEPROM */
#define SMF_MANERR 0x4      /* Error reading manuf. from EEPROM */
#define SMF_GPSERR 0x8      /* Error reading GPS config from EEPROM */
#define SMF_OPTO 0x10       /* Opto output is on */
#define SMF_EXTNMEA 0x20    /* External NMEA active */
#define SMF_SCHED 0x40      /* BE woken up by schedule */
/* Cal Status Bits */
#define CAL_ENON 0x1        /* Calibration enable on */
#define CAL_SGON 0x2        /* Calibration signal on */
#define CAL_ERROR 0x4       /* Calibration on */
/* Data Packet Codes */
#define DT_DATA 0           /* Normal Data */
#define DT_STATUS 2         /* Status */
#define DT_DISCON 3         /* Disconnect Request */
/* Blockette Lengths */
#define GDS_TIM_LENGTH 8    /* Timing */
#define DOOR_LENGTH 12      /* Drift Out of Range */
#define CALSTART_LENGTH 24  /* Calibration Start */
#define MAX_IDENTS_LTH 136  /* Worst case logger ident lengths */
/* Station Monitor Definitions */
#define SM_OFFSET 1         /* Station Monitor on BE is at baseport + 1 */
#define SMC_REQ 0x1C        /* Station Monitor Request */
#define SMC_RESP 0x1D       /* Station Monitor Response */
#define SM_REQ_SIZE 4       /* Station Monitor request data size */
/* DUST */
#define STF_FULL 1          /* Not all DUST status fit */
#define VCHAN_FLAG 0x80     /* If set is 0.1SPS aligned */
#define NOT_TIME_SERIES 0x40 /* If set is not time series */
#define DUST_CMD_MAX 72
#define DUST_STAT_MAX 68

typedef struct       /* common header for all UDP messages */
{
    U8 cmd_ver ;     /* Command (bits 0-4) and Version (bits 5-7) */
    U8 seqs ;        /* Sequences */
    U8 dlength ;     /* Data Length in longwords - 1 */
    U8 complth ;     /* 1's complement of dlength */
} tqdp ;
typedef tqdp *pqdp ;

/* Data Blockettes */
typedef struct           /* Format of timing blockette */
{
    enum tgdsrc gds ;      /* GDS_TIM */
    U8 clock_qual ;      /* Clock quality in percent */
    U16 since_loss ;      /* Minutes since GPS loss */
    I32 usec_offset ;  /* Microseconds offset */
    U8 flags ;           /* Future flags */
} ttimeblk ;

typedef struct
{
    enum tgdsrc gds ;      /* GDS_MD, GDS_CM, or GDS_AC */
    U8 chan ;            /* Channel within that group */
    U8 offset_flag ;     /* Offset to data and flags */
    U8 blk_size ;        /* Blockette Size */
    I32 prev_offset ;  /* Previous value if flag set */
    U16 sampoff ;         /* Sample offset if flag set */
} tdatahdr ;

typedef struct           /* Format of State of Health Blockette */
{
    enum tgdsrc gds ;      /* GDS_SOH */
    U8 flags ;           /* Bitmap */
    U8 spare1 ;
    U8 blk_size ;        /* Blockette Size */
    /* Following is also used in station monitor status */
    U8 gpio1 ;           /* General Purpose Input 1 % */
    U8 gpio2 ;           /* General Purpose Input 2 % */
    U8 pkt_buf ;         /* Packet Buffer Percent */
    U8 humidity ;        /* Sensor Humidity % */
    U16 sensor_cur[SENSOR_COUNT] ; /* Sensor currents - 1ma (100ua) */
    I16 booms[BOOM_COUNT] ; /* Booms - 1mv */
    U16 sys_cur ;         /* System Current - 1ma (100ua) */
    U16 ant_cur ;         /* GPS antenna current - 100ua */
    U16 input_volts ;     /* Input Voltage - 10mv (2mv) */
    I16 sys_temp ;       /* System temperature - 0.1C */
    I32 spare3 ;
    /* End of common area */
    U16 neg_analog ;      /* Negative Analog Voltage - 10mv (100uv) */
    U16 iso_dc ;          /* Isolated DC - 100mv (1mv) */
    I16 vco_control ;    /* VCO Control - -5000 to 5000 */
    U16 ups_volts ;       /* UPS Voltage - 10mv (2mv) */
    U16 ant_volts ;       /* Antenna Voltage - 10mv (2mv) */
    U16 spare6 ;
    U16 spare4 ;
    U16 spare5 ;
} tsohblk ;

typedef struct           /* Format of Engineering Blockette */
{
    enum tgdsrc gds ;      /* GDS_ENG */
    U8 gps_sens ;        /* GPS/Sensor Power Bits */
    U8 gps_ctrl ;        /* GPS Control Bits */
    U8 blk_size ;        /* Blockette Size */
    U16 sensa_dig ;       /* Sensor A Digital I/O Bits */
    U16 sensb_dig ;       /* Sensor B Digital I/O Bits */
    U8 sens_serial ;     /* Sensor Serial I/O Bits */
    U8 misc_io ;         /* Misc. I/O Bits */
    U16 dust_io ;         /* DUST I/O Bits */
    U8 gps_hal ;         /* GPS HAL bits */
    U8 gps_state ;       /* GPS State machine (tgps_state) */
    U16 spare ;
} tengblk ;

typedef struct           /* Format of GPS Data Blockette */
{
    enum tgdsrc gds ;      /* GDS_GPS */
    U8 sat_used ;        /* Number of satellites used */
    U8 fix_type ;        /* Fix type 0-3 */
    U8 blk_size ;        /* Blockette Size */
    I32 lat_udeg ;     /* Latitude in micro degrees */
    I32 lon_udeg ;     /* Longitude in micro degrees */
    I32 elev_dm ;      /* Elevation in deci-meters */
} tgpsblk ;

typedef struct           /* DUST_RDATA blockette */
{
    enum tgdsrc gds ;      /* GDS_DUST */
    U8 count ;           /* Channel Count */
    U8 flags_slot ;      /* FE will put slot in ls 3 bits */
    U8 blk_size ;        /* Blockette Size */
    U32 time ;        /* UTC Time */
    U32 map ;         /* Bitmap of samples */
    I32 samples[DUST_CHANNELS] ; /* Worst case if 32 bit samples */
} tdustblk ;

enum tstype
{ST_SM,        /* Station Monitor Status */
 ST_GPS,       /* GPS Extended Status */
 ST_PLL,       /* PLL Status */
 ST_LS,        /* Logger Status */
 ST_IDL,       /* IDL Status */
 ST_DUST,      /* Dust Status */
 ST_DUSTLAT,   /* Dust Data Latency (internal to Lib660) */
 ST_SIZE
};     /* To Get number of entries */

enum tgpspwr
{GPWR_OFF,    /* Off, this normally doesn't happen */
 GPWR_OFFP,   /* Powered off due to PLL Lock */
 GPWR_OFFT,   /* Powered off due to time limit */
 GPWR_OFFC,   /* Powered off by command */
 GPWR_OFFF,   /* Powered off due to a fault */
 GPWR_ONA,    /* Powered on automatically */
 GPWR_ONC,    /* Powered on by command */
 GPWR_COLD,   /* Coldstart */
 GPWR_SIZE
};  /* To get number of entries */
enum tgpsfix
{GFIX_OFF,    /* Off, unknown fix */
 GFIX_OFFNL,  /* Off, no fix */
 GFIX_OFF1,   /* Off, last fix was 1D */
 GFIX_OFF2,   /* Off, last fix was 2D */
 GFIX_OFF3,   /* Off, last fix was 3D */
 GFIX_ONNF,   /* On, no fix */
 GFIX_ONF,    /* On, unknown fix */
 GFIX_ON1,    /* On, 1D fix */
 GFIX_ON2,    /* On, 2D fix */
 GFIX_ON3,    /* On, 3D fix */
 GFIX_SIZE
} ; /* To get number of entries */
enum tpllstat
{PLL_OFF,
 PLL_HOLD,
 PLL_TRACK,
 PLL_LOCK,
 PLL_SIZE
} ;

/* Start of Status Blockettes */
enum tadgain
{AG_HIGH,      /* Programmed for High Voltage */
 AG_VERIFY,    /* Wants low, checking signal */
 AG_LOW,       /* Low Voltage mode enabled */
 AG_LOCKOUT
} ; /* Wants low, but over-range */

/* Logger Status */
#define BS_MANUAL 1      /* Manual On */
#define BS_AUTO 2        /* Automatic On */
#define BS_CONT 3        /* Continuous */

typedef struct           /* Format of Station Monitor Status Blockette */
{
    enum tstype stype ;    /* ST_SM */
    U8 flags ;           /* SMF_xxxx bits */
    enum tpllstat pll_status ; /* PLL_xx */
    U8 blk_size ;        /* Blockette Size */
    /* Following is also used in SOH Blockette */
    U8 gpio1 ;           /* General Purpose Input 1 % */
    U8 gpio2 ;           /* General Purpose Input 2 % */
    U8 pkt_buf ;         /* Packet Buffer Percent */
    U8 humidity ;        /* Sensor Humidity % */
    U16 sensor_cur[SENSOR_COUNT] ; /* Sensor currents - 1ma (100ua) */
    I16 booms[BOOM_COUNT] ; /* Booms - 1mv */
    U16 sys_cur ;         /* System Current - 1ma (100ua) */
    U16 ant_cur ;         /* GPS antenna current - 100ua */
    U16 input_volts ;     /* Input Voltage - 10mv (2mv) */
    I16 sys_temp ;       /* System temperature - 0.1C */
    I32 spare3 ;
    /* End of common area */
    U32 sec_offset ;  /* Seconds since 2016 */
    I32 usec_offset ;  /* Microseconds offset */
    U32 total_time ;  /* Total time in seconds */
    U32 power_time ;  /* Total power on time in seconds */
    U32 last_resync ; /* Time of last resync */
    U32 resyncs ;     /* Total number of resyncs */
    U16 clock_loss ;      /* Minutes since lock */
    enum tadgain gain_status[SENSOR_COUNT] ; /* Tadgain entries */
    U16 sensor_ctrl_map ; /* Sensor Control Bitmap */
    U8 fault_code ;      /* Two 4 bit fields */
    U8 spare4 ;
    enum tgpspwr gps_pwr ; /* GPS Power State */
    enum tgpsfix gps_fix ; /* GPS Fix State */
    U8 clock_qual ;      /* Clock quality in percent */
    U8 cal_stat ;        /* Calibrator Status */
    float elevation ;     /* Decimal elevation in metres */
    float latitude ;      /* Decimal latitude */
    float longitude ;     /* Decimal longitude */
    U16 ant_volts ;       /* Antenna Voltage - 10mv (2mv) */
    U16 spare5 ;
} tstat_sm ;

typedef struct           /* Format of one GPS satellite entry */
{
    U16 num ;             /* satellite number */
    I16 elevation ;      /* elevation in meters */
    I16 azimuth ;        /* azimuth in degrees */
    U16 snr ;             /* signal to noise ratio */
} tstat_gpssat ;

typedef struct           /* Format of GPS Extended Status */
{
    enum tstype stype ;    /* ST_GPS */
    U8 spare1 ;
    U8 sat_count ;       /* Number of satellites */
    U8 blk_size ;        /* Blockette Size */
    U8 sat_used ;        /* Number of satellite used */
    U8 sat_view ;        /* Number of satellites in view */
    U16 check_errors ;    /* Checksum Errors, clipped at 65535 */
    U16 gpstime ;         /* GPS Power on/off time in seconds */
    U16 spare2 ;
    U32 last_good ;   /* Time of last good 1PPS */
    tstat_gpssat gps_sats[MAX_SAT] ; /* One entry for each satellite, only 1st sat_used satellites reported */
} tstat_gps ;

typedef struct           /* Format of PLL Status */
{
    enum tstype stype ;    /* ST_PLL */
    U8 spare1 ;
    U8 spare2 ;
    U8 blk_size ;        /* Blockette Size */
    float start_km ;      /* Initial VCO */
    float time_error ;
    float best_vco;
    U32 ticks_track_lock ; /* ticks since last track or lock */
    I16 km ;
    I16 cur_vco ;
} tstat_pll ;

typedef struct           /* Format of Logger Status */
{
    enum tstype stype ;    /* ST_LS */
    U8 id_count ;        /* Total number of Logger ID's */
    U8 status ;          /* Manual, Automatic, Continuous */
    U8 blk_size ;        /* Blockette Size */
    U32 last_on ;     /* Time last turned on */
    U32 powerups ;    /* Total number of power ups since reset */
    U16 timeouts ;        /* Total number of timeouts */
    U16 baler_time ;      /* minutes since baler was activated */
    char idents[MAX_IDENTS_LTH] ; /* Maximum size of all idents */
} tstat_logger ;

typedef struct           /* Format of IDL Status */
{
    enum tstype stype ;    /* ST_IDL */
    U8 spare1 ;
    U8 spare2 ;
    U8 blk_size ;        /* Blockette Size */
    char idlstat[256] ;    /* Up to 256 bytes, usually shorter */
} tstat_idl ;

typedef struct           /* Format of one DUST status blockette */
{
    enum tstype stype ;    /* Packet type (DUST_RSTATUS/ST_DUST) */
    U8 string_count ;    /* Number of Strings */
    U8 flags_slot ;      /* LS 3 bits are slot number */
    U8 blk_size ;        /* Blockette Size */
    U32 time ;        /* UTC Time */
    char strings[DUST_STAT_MAX] ; /* One or more null terminated strings */
} tone_dust_stat ;

typedef struct           /* Format of DUST Status to BE */
{
    enum tstype stype ;    /* ST_DUST */
    U8 flags ;           /* STF_xxx entries */
    U8 count ;           /* Number of DUST_RSTATUS entries */
    U8 blk_size ;        /* Blockette Size */
    tone_dust_stat dust_stats[DUST_COUNT] ;
} tstat_dust ;

typedef U32 tstat_dustlat[DUST_COUNT] ; /* Format of Dust newest data time */

typedef struct           /* One User message blockette */
{
    U8 blk_type ;        /* Blockette Type = IB_UMSG */
    enum tsource source ;  /* Who sent it */
    U8 msglth ;          /* Actual valid characters in msg */
    U8 blk_size ;        /* Blockette Size */
    char msg[80] ;
} tuser_message ;

typedef struct
{
    U8 ptype ;           /* 0 */
    U8 spare1 ;
    U8 spare2 ;
    U8 blk_size ;        /* Blockette Size */
    t64 q660_sn ;          /* Serial Number */
    U16 baseport ;        /* Base Port */
    U16 token ;           /* Registration token */
} tpoc_message ;

typedef struct
{
    U8 chan ;
    U8 amplitude ;       /* in shifts */
    U8 waveform ;        /* type of waveform and flags */
    U8 blk_size ;        /* Blockette Size */
    U16 freqdiv ;         /* frequency divider */
    U16 duration ;        /* duration in seconds */
    U16 calbit_map ;      /* which channels are being calibrated */
    U16 spare ;
    U16 settle ;          /* settling time in seconds */
    U16 trailer ;         /* trailing time in seconds */
    U32 spare2 ;
    U32 spare3 ;
} tcalstart ;

typedef struct
{
    U8 req_type ;        /* 0 = info, 1 = status */
    U8 spare ;
    U16 req_map ;         /* Bitmask of request, should just be bit zero */
} tsm_req ;

typedef struct
{
    U8 resp_type ;       /* 1 */
    U8 spare1 ;
    U8 spare2 ;
    U8 blk_size ;
    t64 serial ;           /* System Serial Number */
    U32 prop_tag ;    /* Property Tag */
    U16 be_ver ;          /* Back end version */
    U16 fe_ver ;          /* Front end version */
} tsm_info ;

typedef struct
{
    U8 resp_type ;       /* 2 */
    U8 spare1 ;
    U8 spare2 ;
    U8 blk_size ;
    U32 last_boot ;   /* Time of last FE reboot */
    U32 ss_boot ;     /* Time of SS Boot */
    U32 be_boot ;     /* Time of BE Boot */
    U32 reboots ;     /* Number of FE reboots */
    U32 resp_map ;    /* Status bitmap of what follows */
} tsm_stat ;

extern U8 loadbyte (PU8 *p) ;
extern I8 loadshortint (PU8 *p) ;
extern U16 loadword (PU8 *p) ;
extern I16 loadint16 (PU8 *p) ;
extern U32 loadlongword (PU8 *p) ;
extern I32 loadlongint (PU8 *p) ;
extern float loadsingle (PU8 *p) ;
extern double loaddouble (PU8 *p) ;
extern void loadstring (PU8 *p, int fieldwidth, pchar s) ;
extern void loadt64 (PU8 *p, t64 *six4) ;
extern void loadblock (PU8 *p, int size, pointer pdest) ;

extern int loadqdphdr (PU8 *p, tqdp *hdr) ;
extern void loadstatsm (PU8 *p, tstat_sm *psm) ;
extern void loadstatgps (PU8 *p, tstat_gps *pgp) ;
extern void loadstatpll (PU8 *p, tstat_pll *pll) ;
extern void loadstatlogger (PU8 *p, tstat_logger *plog) ;
extern void loadstatidl (PU8 *p, tstat_idl *pidl) ;
extern void loadstatdust (PU8 *p, tstat_dust *pdst) ;
extern void loadumsg (PU8 *p, tuser_message *pu) ;
extern void loadtimehdr (PU8 *p, ttimeblk *pt) ;
extern void loaddatahdr (PU8 *p, tdatahdr *datahdr) ;
extern void loadsoh (PU8 *p, tsohblk *soh) ;
extern void loadeng (PU8 *p, tengblk *eng) ;
extern void loadgps (PU8 *p, tgpsblk *gps) ;
extern void loaddust (PU8 *p, tdustblk *dust) ;
extern void loadcalstart (PU8 *p, tcalstart *cals) ;
extern void loadpoc (PU8 *p, tpoc_message *poc) ;

#endif

