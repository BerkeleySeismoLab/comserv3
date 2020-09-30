/*
    Q660 Data & Status Record Constant and Type definitions
    Copyright 2016, 2017 by Certified Software Corporation
    John Day, Oregon, USA

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
*/

#ifndef READPACKETS_H
#define READPACKETS_H
#define VER_READPACKETS 12

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
#define DT_LLDATA 1         /* Low Latency Data */
#define DT_STATUS 2         /* Status */
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

typedef struct {     /* common header for all UDP messages */
  byte cmd_ver ;     /* Command (bits 0-4) and Version (bits 5-7) */
  byte seqs ;        /* Sequences */
  byte dlength ;     /* Data Length in longwords - 1 */
  byte complth ;     /* 1's complement of dlength */
} tqdp ;
typedef tqdp *pqdp ;

/* Data Blockettes */
typedef struct {         /* Format of timing blockette */
  enum tgdsrc gds ;      /* GDS_TIM */
  byte clock_qual ;      /* Clock quality in percent */
  word since_loss ;      /* Minutes since GPS loss */
  longint usec_offset ;  /* Microseconds offset */
  byte flags ;           /* Future flags */
} ttimeblk ;

typedef struct {
  enum tgdsrc gds ;      /* GDS_MD, GDS_CM, or GDS_AC */
  byte chan ;            /* Channel within that group */
  byte offset_flag ;     /* Offset to data and flags */
  byte blk_size ;        /* Blockette Size */
  longint prev_offset ;  /* Previous value if flag set */
  word sampoff ;         /* Sample offset if flag set */
} tdatahdr ;

typedef struct {         /* Format of State of Health Blockette */
  enum tgdsrc gds ;      /* GDS_SOH */
  byte flags ;           /* Bitmap */
  byte spare1 ;
  byte blk_size ;        /* Blockette Size */
  /* Following is also used in station monitor status */
  byte gpio1 ;           /* General Purpose Input 1 % */
  byte gpio2 ;           /* General Purpose Input 2 % */
  byte pkt_buf ;         /* Packet Buffer Percent */
  byte humidity ;        /* Sensor Humidity % */
  word sensor_cur[SENSOR_COUNT] ; /* Sensor currents - 1ma (100ua) */
  int16 booms[BOOM_COUNT] ; /* Booms - 1mv */
  word sys_cur ;         /* System Current - 1ma (100ua) */
  word ant_cur ;         /* GPS antenna current - 100ua */
  word input_volts ;     /* Input Voltage - 10mv (2mv) */
  int16 sys_temp ;       /* System temperature - 0.1C */
  longint spare3 ;
  /* End of common area */
  word neg_analog ;      /* Negative Analog Voltage - 10mv (100uv) */
  word iso_dc ;          /* Isolated DC - 100mv (1mv) */
  int16 vco_control ;    /* VCO Control - -5000 to 5000 */
  word ups_volts ;       /* UPS Voltage - 10mv (2mv) */
  word ant_volts ;       /* Antenna Voltage - 10mv (2mv) */
  word spare6 ; 
  word spare4 ;
  word spare5 ;
} tsohblk ;

typedef struct {         /* Format of Engineering Blockette */
  enum tgdsrc gds ;      /* GDS_ENG */
  byte gps_sens ;        /* GPS/Sensor Power Bits */
  byte gps_ctrl ;        /* GPS Control Bits */
  byte blk_size ;        /* Blockette Size */
  word sensa_dig ;       /* Sensor A Digital I/O Bits */
  word sensb_dig ;       /* Sensor B Digital I/O Bits */
  byte sens_serial ;     /* Sensor Serial I/O Bits */
  byte misc_io ;         /* Misc. I/O Bits */
  word dust_io ;         /* DUST I/O Bits */
  longword spare ;
} tengblk ;

typedef struct {         /* Format of GPS Data Blockette */
  enum tgdsrc gds ;      /* GDS_GPS */
  byte sat_used ;        /* Number of satellites used */
  byte fix_type ;        /* Fix type 0-3 */
  byte blk_size ;        /* Blockette Size */
  longint lat_udeg ;     /* Latitude in micro degrees */
  longint lon_udeg ;     /* Longitude in micro degrees */
  longint elev_dm ;      /* Elevation in deci-meters */
} tgpsblk ;

enum tstype {ST_SM,        /* Station Monitor Status */
             ST_GPS,       /* GPS Extended Status */
             ST_PLL,       /* PLL Status */
             ST_LS,        /* Logger Status */
             ST_IDL,       /* IDL Status */
             ST_SIZE};     /* To Get number of entries */
enum tgpspwr {GPWR_OFF,    /* Off, this normally doesn't happen */
              GPWR_OFFP,   /* Powered off due to PLL Lock */
              GPWR_OFFT,   /* Powered off due to time limit */
              GPWR_OFFC,   /* Powered off by command */
              GPWR_OFFF,   /* Powered off due to a fault */
              GPWR_ONA,    /* Powered on automatically */
              GPWR_ONC,    /* Powered on by command */
              GPWR_COLD,   /* Coldstart */
              GPWR_SIZE};  /* To get number of entries */
enum tgpsfix {GFIX_OFF,    /* Off, unknown fix */
              GFIX_OFFNL,  /* Off, no fix */
              GFIX_OFF1,   /* Off, last fix was 1D */
              GFIX_OFF2,   /* Off, last fix was 2D */
              GFIX_OFF3,   /* Off, last fix was 3D */
              GFIX_ONNF,   /* On, no fix */
              GFIX_ONF,    /* On, unknown fix */
              GFIX_ON1,    /* On, 1D fix */
              GFIX_ON2,    /* On, 2D fix */
              GFIX_ON3,    /* On, 3D fix */
              GFIX_SIZE} ; /* To get number of entries */
enum tpllstat {PLL_OFF,
               PLL_HOLD,
               PLL_TRACK,
               PLL_LOCK,
               PLL_SIZE} ;

/* Start of Status Blockettes */
enum tadgain {AG_HIGH,      /* Programmed for High Voltage */
              AG_VERIFY,    /* Wants low, checking signal */
              AG_LOW,       /* Low Voltage mode enabled */
              AG_LOCKOUT} ; /* Wants low, but over-range */

/* Logger Status */
#define BS_MANUAL 1      /* Manual On */
#define BS_AUTO 2        /* Automatic On */
#define BS_CONT 3        /* Continuous */

typedef struct {         /* Format of Station Monitor Status Blockette */
  enum tstype stype ;    /* ST_SM */
  byte flags ;           /* SMF_xxxx bits */
  enum tpllstat pll_status ; /* PLL_xx */
  byte blk_size ;        /* Blockette Size */
  /* Following is also used in SOH Blockette */
  byte gpio1 ;           /* General Purpose Input 1 % */
  byte gpio2 ;           /* General Purpose Input 2 % */
  byte pkt_buf ;         /* Packet Buffer Percent */
  byte humidity ;        /* Sensor Humidity % */
  word sensor_cur[SENSOR_COUNT] ; /* Sensor currents - 1ma (100ua) */
  int16 booms[BOOM_COUNT] ; /* Booms - 1mv */
  word sys_cur ;         /* System Current - 1ma (100ua) */
  word ant_cur ;         /* GPS antenna current - 100ua */
  word input_volts ;     /* Input Voltage - 10mv (2mv) */
  int16 sys_temp ;       /* System temperature - 0.1C */
  longint spare3 ;
  /* End of common area */
  longword sec_offset ;  /* Seconds since 2016 */
  longint usec_offset ;  /* Microseconds offset */
  longword total_time ;  /* Total time in seconds */
  longword power_time ;  /* Total power on time in seconds */
  longword last_resync ; /* Time of last resync */
  longword resyncs ;     /* Total number of resyncs */
  word clock_loss ;      /* Minutes since lock */
  enum tadgain gain_status[SENSOR_COUNT] ; /* Tadgain entries */
  word sensor_ctrl_map ; /* Sensor Control Bitmap */
  byte fault_code ;      /* Two 4 bit fields */
  byte spare4 ;
  enum tgpspwr gps_pwr ; /* GPS Power State */
  enum tgpsfix gps_fix ; /* GPS Fix State */
  byte clock_qual ;      /* Clock quality in percent */
  byte cal_stat ;        /* Calibrator Status */
  single elevation ;     /* Decimal elevation in metres */
  single latitude ;      /* Decimal latitude */
  single longitude ;     /* Decimal longitude */
  word ant_volts ;       /* Antenna Voltage - 10mv (2mv) */
  word spare5 ;
} tstat_sm ;

typedef struct {         /* Format of one GPS satellite entry */
  word num ;             /* satellite number */
  int16 elevation ;      /* elevation in meters */
  int16 azimuth ;        /* azimuth in degrees */
  word snr ;             /* signal to noise ratio */
} tstat_gpssat ;

typedef struct {         /* Format of GPS Extended Status */
  enum tstype stype ;    /* ST_GPS */
  byte spare1 ;
  byte sat_count ;       /* Number of satellites */
  byte blk_size ;        /* Blockette Size */
  byte sat_used ;        /* Number of satellite used */
  byte sat_view ;        /* Number of satellites in view */
  word check_errors ;    /* Checksum Errors, clipped at 65535 */
  word gpstime ;         /* GPS Power on/off time in seconds */
  word spare2 ;
  longword last_good ;   /* Time of last good 1PPS */
  tstat_gpssat gps_sats[MAX_SAT] ; /* One entry for each satellite, only 1st sat_used satellites reported */
} tstat_gps ;

typedef struct {         /* Format of PLL Status */
  enum tstype stype ;    /* ST_PLL */
  byte spare1 ;
  byte spare2 ;
  byte blk_size ;        /* Blockette Size */
  single start_km ;      /* Initial VCO */
  single time_error ;
  single best_vco;
  longword ticks_track_lock ; /* ticks since last track or lock */
  int16 km ;
  int16 cur_vco ;
} tstat_pll ;

typedef struct {         /* Format of Logger Status */
  enum tstype stype ;    /* ST_LS */
  byte id_count ;        /* Total number of Logger ID's */
  byte status ;          /* Manual, Automatic, Continuous */
  byte blk_size ;        /* Blockette Size */
  longword last_on ;     /* Time last turned on */
  longword powerups ;    /* Total number of power ups since reset */
  word timeouts ;        /* Total number of timeouts */
  word baler_time ;      /* minutes since baler was activated */
  char idents[MAX_IDENTS_LTH] ; /* Maximum size of all idents */
} tstat_logger ;

typedef struct {         /* Format of IDL Status */
  enum tstype stype ;    /* ST_IDL */
  byte spare1 ;
  byte spare2 ;
  byte blk_size ;        /* Blockette Size */
  char idlstat[256] ;    /* Up to 256 bytes, usually shorter */
} tstat_idl ;

typedef struct {         /* One User message blockette */
  byte blk_type ;        /* Blockette Type = IB_UMSG */
  enum tsource source ;  /* Who sent it */
  byte msglth ;          /* Actual valid characters in msg */
  byte blk_size ;        /* Blockette Size */
  char msg[80] ;
} tuser_message ;

typedef struct {
  byte ptype ;           /* 0 */
  byte spare1 ;
  byte spare2 ;
  byte blk_size ;        /* Blockette Size */
  t64 q660_sn ;          /* Serial Number */
  word baseport ;        /* Base Port */
  word token ;           /* Registration token */
} tpoc_message ;

typedef struct {
  byte chan ;
  byte amplitude ;       /* in shifts */
  byte waveform ;        /* type of waveform and flags */
  byte blk_size ;        /* Blockette Size */
  word freqdiv ;         /* frequency divider */
  word duration ;        /* duration in seconds */
  word calbit_map ;      /* which channels are being calibrated */
  word spare ;
  word settle ;          /* settling time in seconds */
  word trailer ;         /* trailing time in seconds */
  longword spare2 ;
  longword spare3 ;
} tcalstart ;

typedef struct {
  byte req_type ;        /* 0 = info, 1 = status */
  byte spare ;
  word req_map ;         /* Bitmask of request, should just be bit zero */
} tsm_req ;

typedef struct {
  byte resp_type ;       /* 1 */
  byte spare1 ;
  byte spare2 ;
  byte blk_size ;
  t64 serial ;           /* System Serial Number */
  longword prop_tag ;    /* Property Tag */
  word be_ver ;          /* Back end version */
  word fe_ver ;          /* Front end version */
} tsm_info ;

typedef struct {
  byte resp_type ;       /* 2 */
  byte spare1 ;
  byte spare2 ;
  byte blk_size ;
  longword last_boot ;   /* Time of last FE reboot */
  longword ss_boot ;     /* Time of SS Boot */
  longword be_boot ;     /* Time of BE Boot */
  longword reboots ;     /* Number of FE reboots */
  longword resp_map ;    /* Status bitmap of what follows */
} tsm_stat ;

extern byte loadbyte (pbyte *p) ;
extern shortint loadshortint (pbyte *p) ;
extern word loadword (pbyte *p) ;
extern int16 loadint16 (pbyte *p) ;
extern longword loadlongword (pbyte *p) ;
extern longint loadlongint (pbyte *p) ;
extern single loadsingle (pbyte *p) ;
extern double loaddouble (pbyte *p) ;
extern void loadstring (pbyte *p, integer fieldwidth, pchar s) ;
extern void loadt64 (pbyte *p, t64 *six4) ;
extern void loadblock (pbyte *p, integer size, pointer pdest) ;

extern integer loadqdphdr (pbyte *p, tqdp *hdr) ;
extern void loadstatsm (pbyte *p, tstat_sm *psm) ;
extern void loadstatgps (pbyte *p, tstat_gps *pgp) ;
extern void loadstatpll (pbyte *p, tstat_pll *pll) ;
extern void loadstatlogger (pbyte *p, tstat_logger *plog) ;
extern void loadstatidl (pbyte *p, tstat_idl *pidl) ;
extern void loadumsg (pbyte *p, tuser_message *pu) ;
extern void loadtimehdr (pbyte *p, ttimeblk *pt) ;
extern void loaddatahdr (pbyte *p, tdatahdr *datahdr) ;
extern void loadsoh (pbyte *p, tsohblk *soh) ;
extern void loadeng (pbyte *p, tengblk *eng) ;
extern void loadgps (pbyte *p, tgpsblk *gps) ;
extern void loadcalstart (pbyte *p, tcalstart *cals) ;
extern void loadpoc (pbyte *p, tpoc_message *poc) ;

#endif

