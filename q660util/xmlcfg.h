



/*   XML Read Configuration definitions
     Copyright 2015-2021 by
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
    0 2015-12-21 rdr Created
    1 2016-02-23 rdr High resolution option moved to sensor structure.
    2 2016-03-07 rdr Low Latency target added to tdigi. Sensor serial numbers
                     removed from sysinfo.
    3 2019-11-13 rdr low_lat in tsysinfo changed to spare.
   3b 2019-12-21 jms new sensor definitions added. SO_* option names changed
                       to disambiguate from SO... socket options.
                       removed OTHERSENS.
                       ST_STS5 changed to STS-5 and added STS-6 to facilitate
                       future StationXML generation.
                       changed SENSCNT from 13 to 14 to add slip IP to XML.
                       similarly, increased size of tsensor (but not on FE).
                       add desc1 and desc2 to sensor type r/w from xml
    4 2021-03-16 rdr Add DUST definitions.
    5 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef XMLCFG_H
#define XMLCFG_H
#define VER_XMLCFG 26

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif
#ifndef XMLSUP_H
#include "xmlsup.h"
#endif

/* Not every section is needed for every program, just compile the ones needed */
#ifdef PROG_WS /* Webserver needs almost everything in configuration XML */
#define SECT_WRIT 1
#define SECT_SENS 1
#define SECT_MAIN 1
#define SECT_ACCL 1
#define SECT_TIME 1
#define SECT_OPTS 1
#define SECT_NETW 1
#define SECT_ANNC 1
#define SECT_AUTO 1
#define SECT_DUST 1
#define SECT_SEED 1
#endif

#ifdef PROG_BE /* Back-End needs everything */
#define SECT_WRIT 1
#define SECT_SENS 1
#define SECT_MAIN 1
#define SECT_ACCL 1
#define SECT_TIME 1
#define SECT_OPTS 1
#define SECT_NETW 1
#define SECT_ANNC 1
#define SECT_AUTO 1
#define SECT_DUST 1
#define SECT_SEED 1
#endif

#ifdef PROG_SS /* System Supervisor needs networking and options */
#define SECT_NETW 1
#define SECT_OPTS 1
#endif

#ifdef PROG_DL /* Data Logger using Lib660 */
#define SECT_SINF 1
#define SECT_MAIN 1
#define SECT_ACCL 1
#define SECT_SEED 1
#endif

#ifdef SECT_SENS
#ifndef SENSOR_COUNT
#define SENSOR_COUNT 2 /* Two external sensors */
#endif
#define CONTROL_COUNT 5 /* 5 control lines */
#define SENSCNT 16 /* number of params per sensor in XML */
#define SENSTYPE_CNT 17
#define CTRLTYPE_CNT 23

enum tsenstype
{ST_NONE, ST_MULT, ST_GENERIC, ST_STS2, ST_STS2_5, ST_STS5, ST_CMG3T,
 ST_MBB2, ST_EPI, ST_TCOMP, ST_T120, ST_T240, ST_STS6, ST_STS2HG, ST_STS5360,
 ST_SPARE1, ST_SPARE2, ST_COUNT
} ;

enum tctrltype
{CT_NONE, CT_CTRHI, CT_CTRLO, CT_LCKHI, CT_LCKLO, CT_ULCKHI,
 CT_ULCKLO, CT_DEPHI, CT_DEPLO, CT_REMHI, CT_REMLO, CT_CALHI,
 CT_CALLO, CT_AUX0HI, CT_AUX0LO, CT_AUX1HI, CT_AUX1LO,
 CT_AUX2HI, CT_AUX2LO, CT_AUX3HI, CT_AUX3LO, CT_AUX4HI, CT_AUX4LO
} ;

/* Sensor Options bits */
#define SOPT_ACTLO 1     /* Control lines are active low on some CMG-3T's */
#define SOPT_SOCKTUN 2   /* Use socket tunnel instead of inbuilt handler */
#define SOPT_CTRLGND 4   /* Use control ground instead of power ground */
#define SOPT_SLIP 8      /* Use SLIP handler process with socket tunnel */
#define SOPT_BAUDMASK 0x70   /* 3 bits for user-defined baud rate setting */
#define SOPT_BAUDSHIFT 4   /* 3 bits for user-defined baud rate setting */
#define SOPT_XCTL 128      /* enable extended sensor controls GE & G4 that increase power consumption */

typedef struct         /* Definition of one sensor */
{
    string23 name ;      /* Such as STS-5 */
    string23 controls[CONTROL_COUNT] ; /* If needed, based on sensor type */
    BOOLEAN use_serial ; /* TRUE to use serial control, else individual lines */
    BOOLEAN highres ;    /* Only valid semspr A, set for high resolution input */
    enum tsenstype senstype ;
    enum tctrltype ctrltype[CONTROL_COUNT] ;
    U8 options ;       /* Some sensors may have multiple versions */
    U8 spare2 ;
    U32 ip_slip ;      /* IP Address in binary for use by SLIP handler process */
    string47 ip_slip_text ;    /* SLIP IP Address in text format */
    string63 desc1 ;     /* description 1, e.g. unique id info like serial number */
    string63 desc2 ;     /* description 2, e.g. response info to create stationxml */
} tsensor ;
typedef tsensor tsensors[SENSOR_COUNT] ;
extern tsensors sensors ;
extern BOOLEAN read_sensors (void) ;
extern const txmldef XSENSORS[SENSCNT] ;
extern const string23 FSENSTYPES[SENSTYPE_CNT] ;
extern const string23 FCTRLTYPES[CTRLTYPE_CNT] ;
#endif

#if defined(SECT_MAIN) || defined(SECT_ACCL)
#define LINTYPE_CNT 4
#define MAINCNT 11
enum tlinear
{LIN_ALL, LIN_100, LIN_40, LIN_20} ;
typedef struct      /* Definition of one main digitizer channel */
{
    string47 freqlist ; /* List of frequencies */
    string3 linfreq ; /* Linear below frequency */
    U16 freqmap ;    /* Bitmap of frequencies */
    U16 lowfreq ;    /* low latency frequency */
    U16 target ;     /* Low latency target time */
    BOOLEAN lowvolt ; /* Use low voltage input */
    U8 pgagain ;    /* PGA Gain */
    U8 lowlat ;     /* If non-zero, frequency index for low latency output */
    enum tlinear linear ; /* Linear filters below value */
    BOOLEAN quick_ok ; /* Enable quickview on this channel */
} tdigi ;
typedef tdigi tdigis[TOTAL_CHANNELS] ;
extern tdigis digis ;
typedef U16 tfreqtab[FREQS] ;
extern const tfreqtab FREQTAB ;
extern const txmldef XMAINDIGIS[MAINCNT] ;
extern const string23 LINTYPES[LINTYPE_CNT] ;
extern int freq_to_bit (U16 freq) ;
#endif

#ifdef SECT_MAIN
extern BOOLEAN read_maindigis (void) ;
#endif

#ifdef SECT_ACCL
#define ACCLCNT 9
extern BOOLEAN read_accel (void) ;
extern const txmldef XACCEL[ACCLCNT] ;
#endif

#ifdef SECT_SINF
typedef struct
{
    I32 reboots ;    /* Number of reboots */
    U32 boot ;      /* Last Re-Boot time in binary */
    U32 first ;     /* First Boot Time in binary */
    U32 beboot ;    /* BE Boot Time in binary */
    U32 ssboot ;    /* SS Boot Time in binary */
    I32 spslimit ;   /* SPS limit based on priority */
    I32 clock_serial ; /* Clock Serial Number */
    U8 spare3 ;
    U8 spare1 ;
    U16 spare2 ;
    string15 be_ver ;    /* Back-End Version */
    string15 fe_ver ;    /* Front-End Version */
    string15 ss_ver ;    /* System Supervisor Version */
    string15 clk_typ ;   /* Clock Type */
    string15 sa_type ;   /* Sensor A Type */
    string15 sb_type ;   /* Sensor B Type */
    string15 pld_ver ;   /* PLD Versions */
    string31 prop_tag ;  /* Property Tag */
    string31 fe_ser ;    /* Front-End Serial Number, same as used for authentication */
    string63 clock_ver ; /* Clock Version */
    string47 last_rb ;   /* Last Re-Boot time */
    string47 first_bt ;  /* First Boot Time */
} tsysinfo ;
extern tsysinfo sysinfo ;
extern BOOLEAN read_sysinfo (void) ;
#endif

#ifdef SECT_WRIT
#define WRITCNT 7
typedef struct
{
    U16 proto_ver ;    /* Protocol version */
    U16 spare1 ;
    U32 last_upd ; /* Last updated in binary */
    string31 name ;     /* Program Name */
    string15 version ;  /* Program Version, such as "0.1.1" */
    string47 created ;  /* When file was crated */
    string47 updated ;  /* When file was updated */
} twritercfg ;
extern twritercfg writercfg ;
extern BOOLEAN read_writer (void) ;
extern const txmldef XWRIT[WRITCNT] ;
#endif

#ifdef SECT_TIME
#define TIMECNT 11
#define ENGTYPE_CNT 4
enum tenginetype
{ET_INTERNAL, ET_EXTERNAL, ET_IT530, ET_MAX8} ;
typedef struct       /* Definition of timing structure */
{
    string15 name ;    /* string version of engine */
    U32 hourmap ; /* Bitmap of forced-on hours */
    float temp_tol ;  /* Temperature change tolerance in degrees */
    BOOLEAN cycled ;   /* TRUE for power cycled */
    BOOLEAN intant ;   /* TRUE for internal GPS antenna */
    BOOLEAN boost ;    /* TRUE to boost voltage */
    enum tenginetype enginetype ; /* binary version of engine type */
    BOOLEAN dataon ;   /* Generate data streams */
    BOOLEAN tempon ;   /* Enable auto-on due to temperature change */
    I16 tcxo_off ;   /* TCXO Offset */
    string95 hours_text ; /* Hour list in text format */
} ttiming ;
extern ttiming timing ;
extern BOOLEAN read_timing (void) ;
extern const txmldef XTIME[TIMECNT] ;
extern const string23 FENGTYPES[ENGTYPE_CNT] ;
#endif

#ifdef SECT_OPTS
#define OPTCNT 12
#define OPT_ISO_CNT 2
#define ISO_COUNT 4 /* Up to 4 combinations of bitmaps */
#define ISO_CONT_MASK 1 /* Bit 0 is force continuous mode */
#define ISO_DHCP_MASK 2 /* Bit 1 is force DHCP */
typedef struct      /* Definition of options structure */
{
    BOOLEAN eng_on ;  /* Enable engineering packets */
    U8 spare1 ;
    U8 iso_mode ;   /* Isolated input mode. 0=discrete, 1=binary */
    U8 spare2 ;
    U16 iso_opts[ISO_COUNT] ; /* IO_xxxx bitmaps */
    U32 spare3 ;
    BOOLEAN wos_enable ; /* Enable WOS */
    U8 wos_filter ; /* IIR Filter */
    float wos_threshold ; /* Threshold in g */
    float wos_mindur ;    /* Minimum duration in seconds */
} toptions ;
extern toptions options ;
extern BOOLEAN read_options (void) ;
extern const txmldef XOPTS[OPTCNT] ;
extern const string15 FOPTISOS[OPT_ISO_CNT] ;
extern string iso_lists[ISO_COUNT] ; /* For reading and writing XML */
#endif

#ifdef SECT_NETW
#define NETCNT 16
typedef struct          /* Definition of network structure */
{
    BOOLEAN cycled ;      /* TRUE for power cycled */
    BOOLEAN ipv6 ;        /* Use IPV6 for networking if TRUE, else IPV4 */
    BOOLEAN manual ;      /* Manual addressing mode */
    BOOLEAN usemtu ;      /* Use MTU */
    BOOLEAN usedns ;      /* Use DNS */
    U8 spare1 ;
    U16 baseport ;       /* TCP & UDP base port number */
    U16 prefix_lth ;     /* Prefix length for IPV6 */
    U16 mtu ;            /* MTU */
    U32 hourmap ;    /* Bitmap of forced-on hours */
    U32 ip_v4 ;      /* IP Address in binary for IPV4 */
    U32 router_v4 ;  /* Router Address in binary for IPV4 */
    U32 mask ;       /* Netmask (IPV4 only) */
    U32 dns1_v4 ;    /* DNS server 1 in binary for IPV4 */
    U32 dns2_v4 ;    /* DNS server 2 in binary for IPV4 */
    tip_v6 ip_v6 ;        /* IP Address in binary for IPV6 */
    tip_v6 router_v6 ;    /* Router Address in binary for IPV6 */
    tip_v6 dns1_v6 ;      /* DNS server 1 in binary for IPV6 */
    tip_v6 dns2_v6 ;      /* DNS server 2 in binary for IPV6 */
    string47 ip_text ;    /* IP Address in text format */
    string47 router_text ; /* Router address in text format */
    string47 dns1_text ;  /* DNS server 1 in text format */
    string47 dns2_text ;  /* DNS server 2 in text format */
    string15 mask_text ;  /* IPV4 netmask in text format */
    string95 hours_text ; /* Hour list in text format */
} tnetwork ;
extern tnetwork network ;
extern BOOLEAN read_network (void) ;
extern const txmldef XNETWORK[NETCNT] ;
#endif

#ifdef SECT_ANNC
#define ANNCCNT 15
typedef struct     /* Format of one POC destination */
{
    tip_v6 ip_v6 ;   /* IP Address for IPV6 */
    U32 ip_v4 ; /* IP Address for IPV4 */
    U16 port ;      /* UDP Port number for receiver */
    U16 timeout ;   /* Timeout in minutes */
    U16 resume ;    /* Resume time in minutes */
    U16 interval ;  /* Interval between packets in seconds */
    BOOLEAN valid ;  /* TRUE if this entry is valid, ignore otherwise */
    BOOLEAN random ; /* TRUE to use random UDP port for source */
    BOOLEAN stop ;   /* Temporary stop */
    string ip_text ; /* IP Address (or domain) in text format */
} tdest ;
typedef struct    /* Format for entire announce destination */
{
    BOOLEAN ggate ;  /* Global timeouts enabled */
    U16 gtimeout ;  /* Global timeout */
    U16 gresume ;   /* Global resume */
    tdest dests[MAX_ANNC] ; /* Destinations */
} tannc ;
extern tannc announce ;
extern BOOLEAN read_announce (void) ;
extern const txmldef XANNC[ANNCCNT] ;
#endif

#if defined(SECT_NETW) || defined(SECT_ANNC)
extern BOOLEAN inet_pton6 (pchar src, tip_v6 *dst) ;
#endif

#ifdef SECT_AUTO
#ifndef SENSOR_COUNT
#define SENSOR_COUNT 2 /* Two external sensors */
#endif
#define AUTOCNT 11
typedef struct      /* one set of booms */
{
    U16 tol[3];      /* tolerance on 3 channels in this group */
    U16 max_try;     /* maximum number of tries */
    U16 norm_int;    /* normal interval in minutes */
    U16 squelch_int; /* squelch interval in minutes */
    float duration;  /* in seconds */
} taone;
typedef taone tamass[SENSOR_COUNT];   /* both sensors */
extern tamass automass ;
extern BOOLEAN read_automass (void) ;
extern const txmldef XAUTO[AUTOCNT] ;
#endif

#ifdef SECT_DUST
#define DUSTCNT 9
typedef struct      /* For one DUST device */
{
    BOOLEAN data ;    /* Enable Data */
    BOOLEAN status ;  /* Enable Status */
    U16 spare ;
    U32 tag_id ; /* Tag ID of device */
} tone_dust ;
typedef struct      /* For DUST definitions */
{
    U16 netid ;      /* Network ID, normally 1 */
    U8 flags ;
    U8 spare ;
    tone_dust dusts[DUST_COUNT] ;
} tdust_devs ;
extern tdust_devs dust_devs ;
extern BOOLEAN read_dust (void) ;
extern const txmldef XDUST[DUSTCNT] ;
#endif

extern BOOLEAN fatal ;

#endif
