/*   Server Include File
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 20 Mar 94 WHO Split off from server.c
    1 27 Mar 94 WHO Each blockette type has it's own ring. Circular ring
                    now used for all data.
    2  6 Jun 94 WHO Define tuser_privilege mask structure.
    3  9 Jun 94 WHO Pvoid removed, already defined.
    4  9 Jun 96 WHO Add xfersize field to tring to transfer only the
                    valid bytes to a client.
    5 29 Sep 2020 DSN Updated for comserv3.
*/

#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

#include "cslimits.h"
#include "cstypes.h"
#include "service.h"

#define BLOB 4096
#define CRC_POLYNOMIAL 1443300200
#define VERBOSE FALSE

/* bit position to stop acking */
#define STOPACK 5

/* Each element of a ring has the following format (links are used
   instead of a simple array since they have different lengths */
struct tring_elem
{
    struct tring_elem *next ; /* pointer to next in ring */
    int32_t blockmap ;           /* bitmap of blocking clients */
    int32_t packet_num ;         /* the packet number */
    tdata_user user_data ;    /* up to 512 bytes plus header */
}  ;
typedef struct tring_elem tring_elem;
typedef tring_elem *pring_elem ;
 
typedef struct
{
    pring_elem head ;  /* place to put newest data */
    pring_elem tail ;  /* location of oldest data, if head==tail, no data */
    short count ;      /* number of buffers in this ring */
    short spare ;
    int32_t size ;        /* size of each element */
    int32_t xfersize ;    /* size of data to transfer to client */
} tring ;

typedef struct
{
    pring_elem scan ;
    int32_t packet ;
} last_struc ;

typedef struct
{
#ifdef COMSERV2
    int client_memid ;               /* client's shared memory ID */
    int client_pid ;                 /* client's process ID */
    int client_uid ;                 /* client's user ID */
    complong client_name ;           /* client's name */
#else
    int client_memid ;               /* client's shared memory ID */
    pid_t client_pid ;                 /* client's process ID */
    uid_t client_uid ;                 /* client's user ID */
    tclientname client_name ;           /* client's name */
#endif
    pclient_struc client_address ;   /* where client's shared memory appears */
    pchar outbuf ;                   /* For reading detector parameters, etc. */
    int32_t outsize ;                   /* Size of destination buffer */
    boolean blocking ;               /* blocking if set */
    boolean active ;                 /* is current active */
    int32_t timeout ;                   /* blocking allowed if non-zero */
    double last_service ;            /* time of last blocking service */
    last_struc last[NUMQ] ;          /* Internal ring pointers for last data */
} tclients ;
        
typedef char widestring[120] ;
typedef char string3[4] ;

typedef int32_t tcrc_table[256] ;

typedef struct
{
    int user_id ;
    int32_t user_mask ;  /* Mask of which services are allowed */
} tone_user_privilege ;

typedef tone_user_privilege tuser_privilege [MAXUSERIDS] ;

/* Serial input state machine phase constants */
#define SOHWAIT 0
#define SYNWAIT 1
#define INBLOCK 2
#define ETXWAIT 3

/* Upload phase constants */
#define UP_IDLE 0
#define WAIT_CREATE_OK 1
#define SENDING_UPLOAD 2
#define WAIT_MAP 3

/* Link control characters */
#define NUL 0
#define SOH 1
#define ETX 3
#define SYN 21
#define DLE 16

#endif
