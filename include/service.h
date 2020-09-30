/*$Id: service.h,v 1.2 2005/06/16 20:17:26 isti Exp $*/
/*   Server Access Include File
     Copyright 1994-1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 14 Mar 94 WHO Derived from test header file.
    1 27 Mar 94 WHO Separate data/blockette structure definitions replaced
                    by common structure. Blockette structures are only
                    as long as needed.
    2 31 Mar 94 WHO Client now receives data blocks and blockettes mixed
                    together in the order received by comserv.
    3 13 Apr 94 WHO CSCM_CLIENTS and CSCM_UNBLOCK commands added.
    4 30 May 94 WHO CSQ_TIME removed, now uses CSQ_FIRST.
    5  6 Jun 94 WHO The server's user ID and sleep timing counts for
                    both privileged and non-privileged users added to
                    tserver_struc. CSCR_PRIVILEGE added. PERM changed
                    to 666 to allow other users access.
    6 19 Jun 95 WHO CSCM_LINKSET added.
    7 21 Jun 96 WHO CSF_MM256 removed.
    8  3 Dec 96 WHO Add CSIM_BLK, BLKQ, and change CHAN.
    9 27 Jul 97 WHO Add CSCM_FLOOD_CTRL.
   10  3 Nov 97 WHO More stations for Unix version, add c++ cond.
   11 24 Aug 07 DSN Added cs_sig_alrm function.
   12 29 Sep 2020 DSN Updated for comserv3.
*/
/* NOTE : SEED data structure definitions (seedstrc.h) are not required
   to be used for gaining access to the server. This allows a client
   process to use it's own version of SEED data structure definitions.
*/
/* General purpose constants */
#ifndef SERVICE_H
#define SERVICE_H

#include "cslimits.h"
#include "dpstruc.h"

#define OK 0
#define ERROR -1
#define NOCLIENT -1

/* Macros for station and client operation.  true on success, false on failure. */
#ifdef COMSERV2
/* Number of bytes for string storage. */
/* COMSERV2 storage is limited to 4 bytes in complong structure. */
#define CLIENT_NAME_SIZE		5
#define SERVER_NAME_SIZE		5
#define empty_string(s1)		(s1.l == 0)
#define client_match(s1,s2)		(s1.l == s2.l)
#define site_match(s1,s2)		(s1.l == s2.l)
#define clear_cname_cs(s1)		s1.l = 0
#define clear_sname_cs(s1)		s1.l = 0
/* Copy station and client names. */
#define copy_cname_cs_cs(dest,src)	dest.l = src.l
#define copy_sname_cs_cs(dest,src)	dest.l = src.l
#define copy_cname_cs_str(dest,src)	dest.l = str_long(src)
#define copy_sname_cs_str(dest,src)	dest.l = str_long(src)
/* Return string version of comserv names. */
#define cname_str_cs(src)		long_str(src.l)
#define sname_str_cs(src)		long_str(src.l)
#else
/* Number of bytes for string storage. */
/* COMSERV3 structure storage is the same as string storage. */
#define CLIENT_NAME_SIZE		16
#define SERVER_NAME_SIZE		16
#define empty_string(s1)		(s1[0] == 0)
#define client_match(s1,s2)		(strncasecmp(s1,s2,CLIENT_NAME_SIZE) == 0)
#define site_match(s1,s2)		(strncasecmp(s1,s2,SERVER_NAME_SIZE) == 0)
#define clear_cname_cs(s1)		memset(s1,0,CLIENT_NAME_SIZE);
#define clear_sname_cs(s1)		memset(s1,0,SERVER_NAME_SIZE);
/* Copy station and client names. */
#define copy_cname_cs_cs(dest,src)	strncpy(dest,src,CLIENT_NAME_SIZE);
#define copy_sname_cs_cs(dest,src)	strncpy(dest,src,SERVER_NAME_SIZE);
#define copy_cname_cs_str(dest,src)	copy_cname_cs_cs(dest,src)
#define copy_sname_cs_str(dest,src)	copy_sname_cs_cs(dest,src)
/* Return string version of comserv names. */
#define cname_str_cs(src)		src
#define sname_str_cs(src)		src
#endif

/* Minimum interval for cs_check operations. */
/* If this is redefined, you must rebuild ALL programs and libraries. */
#ifdef COMSERV2
#ifndef CS_CHECK_INTERVAL
#define CS_CHECK_INTERVAL	10
#endif
#else
#ifndef CS_CHECK_INTERVAL
#define CS_CHECK_INTERVAL	5
#endif
#endif

/* Following are used as the command for the cs_svc call */
/* Local server commands (does not communicate with DA), returns immediately */
#define CSCM_ATTACH 0           /* Attach me to server if not already attached */
#define CSCM_DATA_BLK 1         /* Request combinations of data and blockettes */
#define CSCM_LINK 2             /* link format info */
#define CSCM_CAL 3              /* Calibrator info */
#define CSCM_DIGI 4             /* Digitizer info */
#define CSCM_CHAN 5             /* Channel recording status */
#define CSCM_ULTRA 6            /* Misc flags and Comm Event names */
#define CSCM_LINKSTAT 7         /* Accumulated link performance info */
#define CSCM_CLIENTS 8          /* Get information about clients */
#define CSCM_UNBLOCK 9          /* Unblock packets associated with client N */
#define CSCM_RECONFIGURE 10     /* Link reconfigure request */
#define CSCM_SUSPEND 11         /* Suspend link operation */
#define CSCM_RESUME 12          /* Resume link operation */
#define CSCM_CMD_ACK 13         /* Client acknowledges command finished */
#define CSCM_TERMINATE 14       /* Terminate server */
#define CSCM_LINKSET 15         /* Set server link parameters */

/* Ultra-Shear DP to DA commands */
#define CSCM_SHELL 20           /* Send shell command */
#define CSCM_VCO 21             /* Set VCO frequency */
#define CSCM_LINKADJ 22         /* Change link parameters */
#define CSCM_MASS_RECENTER 23   /* Mass recentering */
#define CSCM_CAL_START 24       /* Start calibration */
#define CSCM_CAL_ABORT 25       /* Abort calibration */
#define CSCM_DET_ENABLE 26      /* Detector on/off */
#define CSCM_DET_CHANGE 27      /* Change detector parameters */
#define CSCM_REC_ENABLE 28      /* Change recording status */
#define CSCM_COMM_EVENT 29      /* Set remote command mask */
#define CSCM_DET_REQUEST 30     /* Request detector parameters */
#define CSCM_DOWNLOAD 31        /* Start file download */
#define CSCM_DOWNLOAD_ABORT 32  /* Abort download */
#define CSCM_UPLOAD 33          /* Start file upload */
#define CSCM_UPLOAD_ABORT 34    /* Abort upload */
#define CSCM_FLOOD_CTRL 35      /* Turn flooding on and off */

/* Data/Blockettes datamask bits used with CSCM_DATA_BLK command */
#define CSIM_DATA 1             /* Return data */
#define CSIM_EVENT 2            /* Return event blockettes */
#define CSIM_CAL 4              /* Return cal blockettes */
#define CSIM_TIMING 8           /* Return timing blockettes */
#define CSIM_MSG 0x10           /* Return message records */
#define CSIM_BLK 0x20           /* Return blockette records */

/* Command return values */
#define CSCR_GOOD 0             /* Inquiry processed */
#define CSCR_ENQUEUE 1          /* Could not find an empty service queue */
#define CSCR_TIMEOUT 2          /* Timeout waiting for service */
#define CSCR_INIT 3             /* Server not intialized */
#define CSCR_REFUSE 4           /* Server refuses your attachment */
#define CSCR_NODATA 5           /* No data or blockettes available */
#define CSCR_BUSY 6             /* Cannot process command at this time */
#define CSCR_INVALID 7          /* Command not valid */
#define CSCR_DIED 8             /* Server has apparently died */
#define CSCR_CHANGE 9           /* Server has changed */
#define CSCR_PRIVATE 10         /* Could not create client's module */
#define CSCR_SIZE 11            /* Command output buffer not large enough */
#define CSCR_PRIVILEGE 12       /* Privileged command */

/* Link formats */
#define CSF_QSL 0               /* QSL Format - variable length up to 512 bytes */
#define CSF_Q512 1              /* Q512 Format - fixed length 512 bytes */

/* Command completion status */
#define CSCS_IDLE 0             /* No command being processed */
#define CSCS_INPROGRESS 20      /* Command in progress */
#define CSCS_FINISHED 21        /* Command done, waiting for client to clear status */
#define CSCS_REJECTED 22        /* Command not known by DA */
#define CSCS_ABORTED 23         /* File transfer aborted */
#define CSCS_NOTFOUND 24        /* File not found */
#define CSCS_TOOBIG 25          /* File to big to transfer */
#define CSCS_CANT 26            /* Can't create file on DA */

/* Sequence control */
#define CSQ_NEXT 0              /* Get next available data/blockettes */
#define CSQ_FIRST 1             /* Get first available data/blockettes */
#define CSQ_LAST 2              /* Get last available data/blockette */
#define CSQ_TIME 3              /* Get first data/blockettes at or after time */

/* Shared memory and semaphore permissions */
#define PERM 0666 /* Everybody can access server structures */

/* Queue ring indexes and bit positions */
#define DATAQ 0
#define DETQ 1
#define CALQ 2
#define TIMQ 3
#define MSGQ 4
#define BLKQ 5
#define NUMQ 6
#define CHAN 6

/* Definitions of client accessable portion of server's shared memory segment */
/* Service queue structure */
typedef struct
{
    int32_t clientseg ;
#ifdef COMSERV2
    complong clientname ;
#else
    string15 clientname ;
#endif
} tsvc ;
 
typedef struct
{
    char init ;                /* Is "I" if structure initialized */
    int32_t server_pid ;       /* PID of server */
    int32_t server_semid ;     /* Id of access semaphore */
    int32_t server_uid ;       /* UID of server */
    int32_t client_wait ;      /* Number of microseconds client should wait for service */
    int32_t privusec ;         /* Microseconds per wait for privileged users */
    int32_t nonusec ;          /* Microseconds per wait for non-privileged users */
    int32_t next_data ;        /* Next data packet number */
    double servcode ;          /* Unique server invocation code */
    tsvc svcreqs[MAXCLIENTS] ; /* Service queue */
} tserver_struc ;

typedef tserver_struc *pserver_struc ;

typedef struct
{
    int16_t first ;   /* first selector */
    int16_t last ;    /* last selector */
} selrange ;

/* 
   Definition of server accessable portion of client's shared memory segment. This
   section is repeated for each station 
*/
typedef struct
{
#ifdef COMSERV2
    complong name ;           /* station name - WAS complong */
#else
    string15 name ;           /* station name - WAS complong */
#endif
    int32_t seg_key ;         /* station segment key */
    int16_t command ;         /* Command to perform for this call */
    boolean blocking ;        /* Client is blocking */
    byte status ;             /* station status */
    int32_t next_data ;       /* Next data packet I want */
    double last_attempt ;     /* time at last attempt to talk to server */ 
    double last_good ;        /* time of last good access to server */
    double servcode ;         /* Server reference code for sequence validity */
    pserver_struc base ;      /* address of server's shared memory segment in my address space */
    int32_t cominoffset ;     /* Offset to command input buffer */
    int32_t comoutoffset ;    /* Offset to command output buffer */
    int32_t comoutsize ;      /* Size of command output buffer */
    int32_t dbufoffset ;      /* Offset to start of data buffers */
    int32_t dbufsize ;        /* Size of user data to be moved */
    int16_t maxdbuf ;         /* Maximum number of data buffers that can be filled */
    int16_t reqdbuf ;         /* Number of data packets requested in this call */
    int16_t valdbuf ;         /* Number of data packets valid after call */
    int16_t seqdbuf ;         /* Sequence control for data request */
    double startdbuf ;        /* Earliest time for packets */
    int32_t seloffset ;       /* Offset to start of selector array */
    int16_t maxsel ;          /* Number of selectors */
    selrange sels[CHAN+1] ;   /* Selector ranges for each type of data and channel request */
    int16_t datamask ;        /* Data/blockette request command bitmask */
/* 
   Command input buffer, command output buffer, data buffers, blockette buffers, and
   selector array follows.
*/
} tclient_station ;

typedef tclient_station *pclient_station ;

/* 
   This is the header for the client's shared memory segment, tclient_station structures
   follow, one for each station. Offsets contains zeroes for unsupported stations (valid
   entries are 0 to maxstation-1).
*/
typedef struct
{
#ifdef COMSERV2
    complong myname ;              /* Client's Name - WAS complong */
#else
    string15 myname ;              /* Client's Name - WAS complong */
#endif
    int32_t client_pid ;           /* Client's PID */
    int32_t client_shm ;           /* Client's shared memory */
    int32_t client_uid ;           /* Client's UID */
    boolean done ;                 /* Service has been performed */
    byte spare ;
    int16_t error ;                /* Error code for service */
    int16_t maxstation ;           /* Number of stations this client works with */
    int16_t curstation ;           /* Current station */
    int32_t offsets[MAXSTATIONS] ; /* Offsets from start of this structure to start
                                      of the tclient_station structures for each station */
} tclient_struc ;

typedef tclient_struc *pclient_struc ;

/* 
   Structures used to generate a shared memory segment for multiple stations
   using the cs_gen procedure.
*/
typedef struct
{
#ifdef COMSERV2
    complong stationname ;    /* Station's name - WAS complong */
#else
    string15 stationname ;    /* Station's name - WAS complong */
#endif
    int32_t comoutsize ;      /* Comoutbuf size */
    int16_t selectors ;       /* Number of selectors wanted */
    int16_t mask ;            /* Data request mask */
    boolean blocking ;        /* Blocking connection */
    int32_t segkey ;          /* Segment key for this station */
    char directory[120] ;     /* Directory for station */
} tstation_entry ;
  
typedef struct
{
#ifdef COMSERV2
    complong myname ;                           /* Client's name - WAS complong */
#else
    string15 myname ;                           /* Client's name - WAS complong */
#endif
    boolean shared ;                            /* TRUE if comoutbuffer is shared among all stations */
    int16_t station_count ;                     /* Number of stations in list */
    int16_t data_buffers ;                      /* Number of data buffers to allocate */
    tstation_entry station_list[MAXSTATIONS] ;  /* Array of station specific information */
} tstations_struc ;
  
typedef tstations_struc *pstations_struc ;
 
/* Data record structure returned to user */
typedef struct
{
    double reception_time ;
    double header_time ;
    byte data_bytes[512] ; /* Up to 512 byte data record, may be shorter */
} tdata_user ;

typedef tdata_user *pdata_user ;

typedef char seltype[6] ; /* selector definitions, LLSSS\0 format */

typedef seltype selarray[] ; /* for however many there are */

typedef selarray *pselarray ;

#ifdef __cplusplus
extern "C" {
#endif

/*
  This procedure will setup the tstations_struc for all comlink stations
  found in the "/etc/stations.ini" file or the specified station. You can then
  customize stations to suit your needs if you desire. For each station it puts 
  in the default number of data buffers and selectors specified. If shared is TRUE,
  then there is one shared command input/output buffer for all stations (of size
  comsize), else each station has it's own buffer. If blocking is TRUE, then
  a blocking connection will be requested.
*/
void cs_setup (pstations_struc stations, pchar name, pchar sname, boolean shared, 
      boolean blocking, short databufs, short sels, short mask, int32_t comsize) ;

/* 
   Remove a selected station from the tstations_struc structure, must be
   called before cs_gen to have any effect.
*/
void cs_remove (pstations_struc stations, short num) ;

/*
  This function takes your tstations_struc and builds your shared memory segment
  and returns it's address. This should be done when client starts. You can
  either setup the tstations_struc yourself, or use the cs_all procedure.
  It attaches to all servers that it can.
*/
pclient_struc cs_gen (pstations_struc stations) ;

/*
  This function detaches a client from the server shared memory segment.
*/
void cs_detach (pclient_struc client, short station_number);

/* 
  This function detaches from all stations and then removes your shared
  memory segment. This should be called before the client exits.
*/
void cs_off (pclient_struc client) ;

/*
  This is the basic access to the server to handle any command. The command
  must have already been setup in the "tclient_station" structure and the
  station number into "curstation" in the "tclient_struc" structure before
  calling. Returns one of the "CSCR_xxxx" values as status.
*/
short cs_svc (pclient_struc client, short station_number) ;

/* This is a polling routine used by cs_scan. The current time is the third
   parameter. It will check the station to see :
     1) If it does not have good status, then every 10 seconds it tries :
         a) cs_link, and if good, does :
         b) cs_attach.
     2) If a station has good status then it checks the server reference code
        to make sure the server hasn't changed invocations while the client
        was away. If it did it :
         a) Resets the next record counters and changes the sequencing to CSQ_FIRST.
         b) Sets the status to CSCR_CHANGE and returns the same.
     3) If the station has good status and the server has not changed then it
        checks to see if the server has newer data or blockettes than the ones
        the client has. If so it returns with CSCR_GOOD, if not returns CSCR_NODATA ;
        To keep the link alive it will return with CSCR_GOOD if no activity
        within 10 seconds.
*/
byte cs_check (pclient_struc client, short station_number, double now) ;

/* 
  For each station it calls cs_check, and it returns with CSCR_GOOD does a
  CSCM_DATA_BLK command to try to get data. If it gets data then it returns
  the station number, or NOCLIENT if no data. Also, if there is change in
  status of the server (goes away, or comes back for instance) then the station
  number is also returned, and the alert flag is set. You should then check the
  status byte for that station to find out what happened.
*/     
short cs_scan (pclient_struc client, boolean *alert) ;

/* try to link to server's shared memory segment. first copies server reference
   code into client's structure */
void cs_link (pclient_struc client, short station_number, boolean first) ;

/* try to send an attach request to the server */
void cs_attach (pclient_struc client, short station_number) ;

/* handle SIGARM signals. */
void cs_sig_alrm (int signo);

#ifdef __cplusplus
}
#endif
#endif
