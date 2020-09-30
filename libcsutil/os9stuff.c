/*$Id: os9stuff.c,v 1.1.1.1 2004/06/15 19:08:07 isti Exp $*/
/*   System V shared memory and semaphore simulation on OS9
     Copyright 1996, 1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0  4 Jun 96 WHO Created.
    1  7 Dec 96 WHO Linking in netdb.l causes an undefined symbol, so
                    make a "inet_addr" here instead.
    2 17 Oct 97 WHO Add VER_OS9STUFF.
    3 22 Oct 97 WHO Add access routines for vopttable.
    4 9  Nov 99 IGD Set RCS ID, chec in the RCS comserv system
*/
#ifdef _OSK
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <module.h>
#include <types.h>
#include <events.h>

#ifndef cs_dpstruc_h
#include "dpstruc.h"
#endif

short VER_OS9STUFF = 4 ;

/* from cfgutil */
void comserv_split (pchar src, pchar right, char sep) ;

#include "os9stuff.h"

/* convert ip address string into number */
  unsigned long inet_addr(char *pc)
    begin
      unsigned long num ;
      char stemp[80] ;
      
      num = 0 ;
      while (pc[0])
        begin
          comserv_split (pc, stemp, '.') ; /* left digits in pc, remainder in stemp */
          if (pc[0])
            then
              num = (num << 8) + (atoi(pc) and 255) ;
          strcpy (pc, stemp) ; /* move remainder back into pc */
        end
      return num ;
    end

/* Internal routine to generate a string from a key */
  void shmname (int key, char *name)
    begin
      char s2[15] ;

      sprintf ((pchar) &s2, "%d", key) ;
      strcpy (name, "SHM") ;
      strcat (name, (pchar) &s2) ; /* form SHMxxxxxxx name */
    end

/* return memory id. If key is IPC_PRIVATE than a unique number
   is generate, else the key is used. A data module will be
   created if shmflag has IPC_CREAT set, else it is simply
   linked to. A data module is used to simulate System V shared
   memory with a name of "SHMxxxxx" where xxxxx is the value of
   the key */
  int shmget (int key, int size, int shmflag)
    begin
      u_int32 time, date, ticks, err ;
      mh_data *modhead ;
      void *data ;
      char *modname ;
      mh_com *modhead2 ;
      u_int16 day, attr_rev, type_lang ;
      char s1[15] ;
      
      if (key == IPC_PRIVATE)
        then
          begin
            _os9_getime (3, &time, &date, &day, &ticks) ;
            key = time + ((date and 63) << 17) + (getpid() << 23) ;
          end
      shmname (key, (pchar) &s1) ;
      if ((shmflag and IPC_CREAT) != 0)
        then
          begin
            attr_rev = 0x8000 ;
            err = _os_datmod((pchar) &s1, size, &attr_rev, &type_lang, 0x333, &data, &modhead) ;
            if (err != 0)
              then
                begin
                  modname = (pchar) &s1 ;
                  type_lang = 0 ;
                  err = _os_link(&modname, &modhead2, &data, &type_lang, &attr_rev) ;
                  if (err != 0)
                    then
                      return -1 ;
                    else
                      return key ;
                end
              else
                return key ;
          end
        else
          return key ;
    end

/* based on memory id, return data address of the data module.
   This address is the module address + $34. shmaddr and
   shmflag are ignored */
  char *shmat (int shmid, char *shmaddr, int shmflag)
    begin
      u_int32 err ;
      u_int16 attr_rev, type_lang ;
      void *data ;
      char *modname ;
      mh_com *modhead ;
      char s1[15] ;

      shmname (shmid, (pchar) &s1) ;
      modname = (pchar) &s1 ;
      type_lang = 0 ;
      err = _os_link(&modname, &modhead, &data, &type_lang, &attr_rev) ;
      if (err != 0)
        then
          return (pchar) -1 ;
        else
          return (pchar) data ;
    end

/* based on the data address, unlink from the data module. The
   module address is the data address - $34. */
  int shmdt (char *shmaddr)
    begin
      u_int32 err ;
      mh_com *modhead ;
      
      modhead = (pvoid) ((int) shmaddr - 0x34) ;
      err = _os_unlink (modhead) ;
      if (err != 0)
        then
          return -1 ;
        else
          return 0 ;
    end

/* based on the memory id, unload the data module from memory.
   cmd and buf are ignored. */
  int shmctl (int shmid, int cmd, char *buf)
    begin
      u_int32 err ;
      char s1[15] ;
      short i ;

      shmname (shmid, (pchar) &s1) ;
      i = 0 ;
      do
        begin
          err = _os_unload((pchar) &s1, 0) ;
          i++ ;
        end
      while ((err == 0) land (i < 1000)) ;
      if (i >= 1000)
        then
          return -1 ;
        else
          return 0 ;
    end

/* if IPC_CREAT is set in semflag then create new event, else
   link to existing event. The event has a name of "SEMxxxxx"
   where xxxxx is the value of the key. nsems is ignored. */
  int semget (int key, int nsems, int semflag)
    begin
      u_int32 err ;
      int id ;
      int32 value ;
      char s1[12], s2[12] ;
      
      if ((key < 0) lor (key > 99999999))
        then
          return -1 ;
      sprintf ((pchar) &s2, "%d", key) ;
      strcpy ((pchar) &s1, "SEM") ;
      strcat ((pchar) &s1, (pchar) &s2) ; /* form SEMxxxxxxx name */
      if ((semflag and IPC_CREAT) != 0)
        then
          begin
            err = _os_ev_creat(-1, 1, 0, &((event_id) id), (pchar) &s1, 0, 0) ;
            if (err != 0)
              then
                begin
                  err = _os_ev_link((pchar)&s1, &((event_id) id)) ;
                  if (err != 0)
                    then
                      return -1 ;
                    else
                      begin
                        value = 0 ;
                        err = _os_ev_set(id, &value, 0) ;
                        if (err != 0)
                          then
                            return -1 ;
                          else
                            return id ;
                      end
                end
              else
                return id ;
          end
        else
          begin
            err = _os_ev_link((pchar) &s1, &((event_id) id)) ;
            if (err != 0)
              then
                return -1 ;
              else
                return id ;
          end
    end

/* opsptr is a point to array of 3 values, if the middle value
   is +1 then this is a ev_signl, if it is -1 then this is a
   ev_wait. nops is ignored. */
  int semop (int semid, struct sembuf *opsptr, unsigned int nops)
    begin
      u_int32 err ;
      int32 val ;
      
      if (opsptr->sem_op > 0)
        then
          err = _os_ev_signal((event_id) semid, &val, 1) ;
        else
          err = _os9_ev_wait((event_id) semid, &val, 1, 9999999) ;
      if (err != 0)
        then
          return -1 ;
        else
          return 0 ;
    end

/* compare two strings, ignoring case. Compare for a maximum of "n" characters
   returns 0 if strings are equal, -1 if string1 < string2, or
   1 if string1 > string2 */
  int strncasecmp(const char *string1, const char *string2, int n)
    begin
      short i ;
      char c1, c2 ;
      
      i = 0 ;
      while ((i < n) land (string1[i] != 0) land (string2[i] != 0))
        begin
          c1 = toupper(string1[i]) ;
          c2 = toupper(string2[i]) ;
          if (c1 < c2)
            then
              return -1 ;
          if (c1 > c2)
            then
              return 1 ;
          i++ ;
        end
      if (i >= n)
        then
          return 0 ; /* compared the maximum number of characters */
      if (string1[i] == 0)
        then
          if (string2[i] == 0)
            then
              return 0 ; /* both the same */
            else
              return -1 ; /* string1 is shorter */
        else
          return 1 ; /* string1 is longer */
    end

/* compare two strings, ignoring case. For some reason, Microware forgot this one
   returns 0 if strings are equal, -1 if string1 < string2, or
   1 if string1 > string2 */
  int strcasecmp(const char *string1, const char *string2)
    begin
      return strncasecmp(string1, string2, 32000) ;
    end
    
/* change size of interrupt input buffer, returns error code */
  int resetp (int path, int size)
    begin
      _asm(" 
   move.l d1,d2 need size in d2
   moveq.l #3,d1 function code
   os9 I$SetStt do call
   bcs.s %0
   clr.l d1 no error
%0 move.l d1,d0 needs to be in d0 to return",
   __label()
   ) ;
    end 

/* Enable hardware flow control, returns error code */
  int hardon (int path)
    begin
      _asm("
   moveq.l #$23,d1 function code for SS_OFC
   moveq.l #2,d2 whatever this does
   os9 I$SetStt do call
   bcs.s %0
   clr.l d1 no error
%0 move.l d1,d0 needs to be in d0 to return",
   __label()
   ) ;
    end
    
/* Try to a block read from the path. returns error code */
  int blockread (int path, int bytecnt, pchar buf)
    begin
      _asm("
   move.l d1,d2 need byte count in D2
   move.l #20,d1 function code
   move.l 8(a5),a0 get buf
   os9 I$GetStt do call
   bcs.s %0
   clr.l d1 no error
%0 move.l d1,d0 needs to be in d0 to return",
   __label()
   ) ;
    end

/* returns pointer to module header in location pointed to by hdr */
/* returns pointer to my entry in location pointed to by pve */
  int vopt_open (mh_com **hdr, voptentry **pve)
    begin
      u_int16 attr_rev, type_lang ;
      u_int32 err ;
      int pid ;
      char *modname ;
      mh_data *modhead ;
      char s1[15] ;
      vopttable *vt ;
      
      type_lang = 0 ;
      strcpy(s1, "VOPTTABLE") ;
      modname = (pchar) &s1 ; /* all this to specify a module name? */
      err = _os_link (&modname, hdr, (void *)&vt, &type_lang, &attr_rev) ;
      switch (err)
        begin
          case 221 :
            begin
              attr_rev = 0x8000 ;
              err = _os_datmod((pchar) &s1, sizeof(vopttable), &attr_rev,
                  &type_lang, 0x333, (void *)&vt, &modhead) ;
              if (err != 0)
                then
                  return err ;
              hdr = (void *) modhead ;
              break ;
            end
          case 0 : break ;
          default : return err ;
        end
      pid = getpid () ;
      if (pid <= HIGH_PID)
        then
          begin
          /* after a dozen failed attempts to get the compiler to index
             a derefernced pointer, I gave up and do it manually */
            *pve = (voptentry *) ((long) vt + sizeof(voptentry) * pid) ;
            return 0 ;
          end
        else
          return 224 ;
    end

/* passed address of module header, closes access to vopttable */
  void vopt_close (mh_com *hdr)
    begin
      u_int32 err ;
      
      err = _os_unlink (hdr) ;
    end
#endif
