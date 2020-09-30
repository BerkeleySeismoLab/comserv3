/*$Id: os9stuff.h,v 1.1.1.1 2004/06/15 19:08:01 isti Exp $*/
/*   System V shared memory and semaphore simulation on OS9
     Copyright 1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0  3 Jun 96 WHO Created.
    1  7 Dec 96 WHO inet_addr added.
    2 22 Oct 97 WHO Add vopttable definitions.
*/

#ifndef OS9STUFF_H
#define OS9STUFF_H

#include <types.h>
#include <sgstat.h>

#ifndef cs_dpstruc_h
#include "dpstruc.h"
#endif

#ifndef _MODULE_H
#include <module.h>
#endif

#define HIGH_PID 127

typedef struct
  begin
    byte verbosity ;
    byte option1 ;
    byte option2 ;
    byte option3 ;
    char modname[28] ; /* module name and optional modifier */
  end voptentry ;
    
/* this is data part of data module "VOPTTABLE" */
typedef voptentry vopttable[HIGH_PID+1] ;

#define IPC_CREAT 0x80000000
#define IPC_RMID 0x40000000
#define IPC_PRIVATE -1
struct sembuf
  begin
    unsigned short sem_num ;
    short sem_op ;
    short sem_flg ;
  end ;

/* some definitions that Microware apparently forgot to put into
   any know header file. */
int getpid (void) ; /* should be built in */
int getuid (void) ; /* should be built in */
int _gs_opt (int path, struct sgbuf *buffer) ; /* should be in sgstat.h */
int _ss_opt (int path, struct sgbuf *buffer) ; /* should be in sgstat.h */
int _gs_size (int path) ;
int tsleep (unsigned svalue) ; /* should be built in */
int open(char *name, short mode) ; /* should be built in */
int close(int path) ; /* should be built in */
int read(int path, char *buffer, unsigned count) ; /* should be built in */
int write(int path, char *buffer, unsigned count) ; /* should be built in */
int kill (int pid, short sigcode) ; /* should be built in */

/* return memory id. If key is IPC_PRIVATE than a unique number
   is generate, else the key is used. A data module will be
   created if shmflag has IPC_CREAT set, else it is simply
   linked to. A data module is used to simulate System V shared
   memory with a name of "SHMxxxxx" where xxxxx is the value of
   the key */
int shmget (int key, int size, int shmflag) ;

/* based on memory id, return data address of the data module.
   This address is the module address + $34. shmaddr and
   shmflag are ignored */
char *shmat (int shmid, char *shmaddr, int shmflag) ;

/* based on the data address, unlink from the data module. The
   module address is the data address - $34. */
int shmdt (char *shmaddr) ;

/* based on the memory id, unload the data module from memory.
   cmd and buf are ignored. */
int shmctl (int shmid, int cmd, char *buf) ;

/* if IPC_CREAT is set in semflag then create new event, else
   link to existing event. The event has a name of "SEMxxxxx"
   where xxxxx is the value of the key. nsems is ignored. */
int semget (int key, int nsems, int semflag) ;

/* opsptr is a point to array of 3 values, if the middle value
   is +1 then this is a ev_signl, if it is -1 then this is a
   ev_wait. nops is ignored. */
int semop (int semid, struct sembuf *opsptr, unsigned int nops) ;  

/* compare two strings, ignoring case. For some reason, Microware forgot this one */
int strcasecmp(const char *string1, const char *string2) ;

/* compare two strings, ignoring case, for a maximum of "n" characters */
int strncasecmp(const char *string1, const char *string2, int n) ;

/* change size of interrupt input buffer, returns error code */
int resetp (int path, int size) ;

/* Enable hardware flow control, returns error code */
int hardon (int path) ;

/* Try to a block read from the path. returns error code */
int blockread (int path, int bytecnt, pchar buf) ;

/* convert ip address string into number */
unsigned long inet_addr(char *pc) ;

/* returns pointer to module header in location pointed to by hdr */
/* returns pointer to my entry in location pointed to by pve */
int vopt_open (mh_com **hdr, voptentry **pve) ;

/* passed address of module header, closes access to vopttable */
void vopt_close (mh_com *hdr) ;

#endif
