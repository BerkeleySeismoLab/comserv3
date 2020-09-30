/*
    System Supervisor Constant and Type definitions Copyright 2016 - 2018 by
    Certified Software Corporation
    John Day, Oregon, USA

REV  DATE        BY   CHANGE
-----------------------------------------------------------------------
  0  2016-02-04  rdr  Created.
  1  2017-04-10  rdr  Status to SS modified.
  2  2017-04-27  rdr  BE and WS Specific info moved here to make SS easier.
  3  2018-02-25  jms  added INLIB660 for conx compilation of client storage alloc
  4  2018-04-01  rdr  In packets to BE/WS, etc. change enum to byte to avoid
                      implementation defined size.
  5  2018-04-03  rdr  Add client ID to status to SS.
  6  2018-04-24  rdr  Add FE_STAT_FLG_ISO flag bits from BE to SS.
  7  2018-06-08  rdr  Add MISC_STAT_NUM and tbemisc.
  8  2019-01-08  rdr  Add FE_STAT_FLG_SCHED.
  9  2019-01-15  rdr  Add ST_STAT_OPEN.
 10  2020-03-05  jms  add newxmlname to tacc. needed by BE660 for rename function.
*/

#ifndef SSIFC_H
#define SSIFC_H
#define VER_SSIFC 8

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

/* Sizes */
#define BUFSIZE 1020          /* Maximum message data size */
#define STAT_CNT 10           /* Max number of status entries to SS */
#define IDENT_LTH 16          /* Room for connected client identifier */
#define PASS_SIZE 64          /* Number of bytes allocated for passwords */
#define PRI_COUNT 4           /* Number of data port priorities */
/* Networking Flags */
#define NIF_NOPASS 1          /* No Passwords required */
#define NIF_NOTLS 2           /* TLS not available */
#define NIF_NONET 4           /* Networking not available */
#define NIF_CYCLED 8          /* Is a power cycled system */
#define NIF_NOREMOTE 0x10     /* Remote XML load not possible */
/* Security Flags */
#define SECF_CLEAR 1          /* Display Passwords as Cleartext */
#define SECF_REMSEC 2         /* Allow changing remote security */
/* BE Configuration Flags */
#define BEF_FORCE 1           /* BE Cfg Flags - force FE update */
#define BEF_NOUPDATE 2        /* BE Cfg Flags - no FE update */
/* WS Configuration Flags */
#define WSF_OFFLINE 1         /* Running in offline mode */
#define WSF_VERBOSE 2         /* Write verbose XML */
/* BE IPV6 flags */
#define BE6_SM 1              /* Use IPV6 for station monitor */
#define BE6_DATA 2            /* Use IPV6 for data */
#define BE6_POC 4             /* Use IPV6 for POCs */
/* SS -> BE Status Bits */
#define BE_STOP 2             /* Request Back End stop */
#define XML_CHANGED 8         /* XML File has been changed by WS */
#define BE_NET_CHANGED 0x20   /* Network (or security) Settings changed for back end */
#define BE_FREEZE_REQ 0x40    /* Request Freeze FE Input */
#define BE_FE_RECORDING 0x80  /* Recording Error, pass to FE */
#define IDLSTAT_VALID 0x100   /* idlstat field is there */
/* SS -> WS Status Bits */
#define WSS_STOP 1            /* Stop Web Server Request */
#define WS_NET_CHANGED 0x10   /* Network Settings changed for web server */
/* Other status records */
#define FE_STAT_NUM 8         /* Use tstat entry 8 for FE time */
#define MISC_STAT_NUM 9       /* Used for various items from BE */
/* BE -> SS Flag Bit in FE Status */
#define FE_STAT_FLG_FROZE 1   /* and this bit in flags field FE input is frozen */
#define FE_STAT_FLG_ISOIN 2   /* Isolated input is active */
#define FE_STAT_FLG_ISOINOUT 4 /* Isolated input/output is active */
#define FE_STAT_FLG_SCHED 8   /* Scheduled BE startup */
/* WS -> SS flags in AT_GET_SENS entry */
#define ST_STAT_OPEN 1        /* Socket tunnel is active */

enum tbemisc {TBM_FETIME} ;   /* If prio in tstats[MISC_STAT_NUM] is this then latest FE
                                 time is in time field and clock quality is in spare */

/* Commands between SS and clients */
enum tcmd {CMD_RQCFG,         /* Request Configuration Information */
           CMD_CFG,           /* Configuration Information */
           CMD_RQNET,         /* Request Networking Information */
           CMD_NET,           /* Networking Information */
           CMD_STAT,          /* Status update from non-SS programs */
           CMD_SSSTAT,        /* Returns with SS status */
           CMD_SSMSG,         /* Messsage to SS */
           CMD_RQACC,         /* Request XML file access */
           CMD_ACC,           /* XML file access response */
           CMD_ACCDONE,       /* Access done */
           CMD_ACK,           /* Acknowledgement from SS */
           CMD_SETSEC,        /* Set Security Sturcture */
           CMD_SETHIST,       /* Set Blockette File History */
           CMD_COUNT} ;       /* To get count */
/* Encryption Types */
enum tenc {ENC_NONE,          /* No encryption */
           ENC_TLS,           /* TLS encryption */
           ENC_COUNT} ;
/* WS660 only in Offline Mode */
enum tmode {TA_STN_TEMP,      /* New station from template */
            TA_EDIT_STN,      /* Edit station */
            TA_STN_STN,       /* New station from station */
            TA_NEW_TEMP,      /* New template */
            TA_TEMP_TEMP,     /* New template from template */
            TA_EDIT_TEMP,     /* Edit template */
            TA_COUNT} ;       /* To get count */

typedef char tpass[PASS_SIZE] ;

typedef struct {              /* Message format */
  byte /*enum tsource*/ source ; /* Who is sending the message */
  byte /*enum tcmd*/ command ; /* Command byte */
  word seq ;                  /* For debugging */
  word datalth ;              /* Data Length */
  word spare ;                /* longword align data */
  byte data[BUFSIZE] ;        /* varies depending on command */
} tmsg ;

#define MSG_OVERHEAD (sizeof(msg)-sizeof(msg.data))

typedef struct {              /* For CMD_RQACC */
  char nameinfo[96] ;         /* Suggestion for file naming */
  boolean wracc ;             /* Want write access if TRUE */
} treqacc ;

typedef struct {              /* For CMD_ACC */
  boolean granted ;           /* Access granted if TRUE */
  byte spare1;
  word spare2;
  char newxmlname[256];       /* XML file path and name */
} tacc ;

typedef struct {              /* One entry for CMD_STAT */
  longword time ;             /* Of last data packet or web access */
  byte prio ;                 /* For BE internal and external data clients */
  byte spare ;                /* Packet Buffer percent for FE entry from BE */
  word flags ;                /* None currently defined */
  char ident[IDENT_LTH] ;     /* Connected client ID */
} tstat ;
typedef tstat tstats[STAT_CNT] ; /* For CMD_STAT */

typedef struct {              /* For CMD_SSSTAT */
  longint flags ;
  char idlstat[256] ;         /* IDL Status (from SS to BE only) */
} tssstat ;

typedef struct {              /* For CMD_SETHIST */
  longint last_file ;
  boolean final ;             /* No Longer Used */
  byte spare1 ;
  word spare2 ;
  tblkcheck checksum ;        /* Checksum of last_file */
} tbe_sethist ;

typedef char tssmsg[96] ;     /* For CMD_SSMSG */

/* Start of Back-End specific structures. Note that security information
   is also part of WS because it has to setup the values */
typedef struct {              /* Back-End Configuration */
  char fname[256] ;           /* XML file path and name */
  char comname[128] ;         /* Communications to FE port name */
  char fefirm[128] ;          /* Front End firmware file name */
  char blkdir[128] ;          /* Directory for blockette files */
  longint blk_count ;         /* Number of blockette files */
  longint blk_size ;          /* Size of each blockette file in bytes */
  longint cache_count ;       /* Number of blockette files in cache */
  longint preload_count ;     /* Number of file to preload into cache */
  longint last_file ;         /* Last written blockette file number */
  longword cycle_baud ;       /* BE <-> FE baud for power cycled */
  longword ss_boot ;          /* System Supervisor Version in binary */
  word ss_version ;           /* System Supervisor Version */
  byte flags ;                /* BEF_xxxx */
  byte spare2 ;
  tblkcheck checksum ;        /* Checksum of last_file */
} tbe_cfg ;

typedef struct {              /* Format of one priority level */
  tpass password ;            /* Only first 60 characters used */
  word max_sps ;              /* Maximum SPS, 0 = SOH only */
  boolean low_lat ;           /* Low Latency OK */
  byte spare ;
} tdatalevel ;

typedef struct {              /* Back-End Security */
  byte /*enum tenc*/ encrypt ; /* Encryption type */
  byte sm_rpm ;               /* Maximum Station Monitor responses per minute */
  word spare2 ;
  tdatalevel levels[PRI_COUNT] ;
} tbe_sec ;

typedef struct {              /* Back-End Networking Info */
  byte ipv6 ;                 /* BE6_xxxx */
  byte flags ;                /* NIF_xxxx */
  word portnum ;              /* Base Port number */
  longword ip_v4 ;            /* Either IPV4 or IPV6, not both */
  tip_v6 ip_v6 ;
  tbe_sec security ;
} tbe_net ;

/* Start of Web-Server specific structures */
enum tws_colidx {TCI_BODY,    /* Body Color */
                 TCI_FSBORD,  /* Fieldset Border */
                 TCI_FSTEXT,  /* Fieldset Title Text */
                 TCI_SEVEN,   /* Striping Even */
                 TCI_SODD,    /* Striping Odd */
                 TCI_SLIDE,   /* Slider Background */
                 TCI_SIZE} ;
typedef char tcolspec[8] ;
typedef tcolspec twscolors[TCI_SIZE] ;
/* Indices for reporting page accesses in status from WS to SS */
enum tacctype {AT_GET_STAT, AT_GET_CMDS, AT_GET_CFG, AT_GET_PLOT, AT_GET_SEC,
               AT_GET_HELP, AT_PUT_CMD, AT_PUT_CFG, AT_PUT_SEC, AT_GET_SENS} ;

typedef struct {              /* Web-Server Configuration */
  char fname[256] ;           /* XML file path and name */
  twscolors colors ;          /* Web Page Colors */
  word ss_version ;           /* System Supervisor Version */
  byte flags ;                /* WSF_xxxx */
  byte /*enum tmode*/ omode ; /* Mode for offline configuration */
} tws_cfg ;

typedef struct {              /* Web-Server Security */
  byte /*enum tenc*/ encrypt ; /* Encryption type */
  byte secflags ;             /* SECF_xxxx */
  word spare2 ;
  tpass master ;              /* Master Password */
  tpass username ;            /* Web Server User Name */
  tpass password ;            /* Web Server Password */
} tws_sec ;

typedef struct {              /* Web Server Networking Info */
  boolean ipv6 ;              /* TRUE for IPV6, else IPV4 */
  byte flags ;                /* NIF_xxxx */
  word portnum ;              /* Base Port number */
  longword ip_v4 ;            /* Either IPV4 or IPV6, not both */
  tip_v6 ip_v6 ;
  tws_sec security ;          /* Web Server security */
  tbe_sec be_security ;       /* Back-End security */
} tws_net ;

typedef struct {              /* For CMD_SETSEC */
  tws_sec security ;
  tbe_sec be_security ;
} tws_setsec ;

/* Client specific routines */
#ifndef INLIB660
extern tmsg msg ;             /* Message Buffer */
extern integer ss_timeout ;   /* ms since packet sent */

extern void sleepms (integer ms) ;

extern void send_to_ss (void) ;
extern boolean recv_from_ss (void) ;
extern boolean open_ss_socket (pchar pc) ;
#endif

#endif


