/*$Id: buffers.c,v 1.2 2005/03/30 22:30:58 isti Exp $*/
/*   Server Buffer management
     Copyright 1994, 2000 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 23 Mar 94 WHO Pulled out of server.c
    1 27 Mar 94 WHO Double linked list of merged blockettes replaced with
                    circular list.
    2  9 Jun 94 WHO Cleanup to avoid warnings.
    3 28 Feb 95 WHO Start of conversion to run on OS9.
    4  3 Dec 96 WHO Add support for Blockette Queue.
    5 17 Oct 97 WHO Add VER_BUFFERS.
    6 29 Jan 00 WHO Fix checkmaks so that if "qnum" is -1 it updates
                    the "noackmask" of all queues, rather than just
                    returning the current noackmask <> 0.
    7 29 Sep 2020 DSN	Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#ifndef _OSK
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#endif
#include <signal.h>
#include "quanstrc.h"
#include "stuff.h"
#include "service.h"
#include "server.h"

short VER_BUFFERS = 7 ;

extern tring rings[NUMQ] ;         /* Description of each ring buffer */
extern pserver_struc base ;        /* Base address of server memory segment */
extern int32_t blockmask, noackmask ;

void setupbuffers (void)
{
    pserver_struc basetemp ;
    tring_elem *datatemp, *last ;
    short i, j ;
        
/* Build linked list of blockette & data buffers. Data buffers start
   immediately after tserver_struc.
*/
    basetemp = base ;
    basetemp++ ;      /* skip over tserver_struc */
    /* double word align */
    datatemp = (pvoid) ((((uintptr_t) basetemp + 7) >> 3) << 3) ;
    for (j = DATAQ ; j < NUMQ ; j++)
    {
	last = NULL ;
	for (i = 0 ; i < rings[j].count ; i++)
	{
	    if (i == 0)
		rings[j].head = datatemp ;  /* newest record */
	    datatemp->blockmap = 0 ;    /* not blocked */
	    if (last)
		last->next = (pvoid) datatemp ; /* forward link */
	    last = datatemp ;
	    datatemp = (pvoid) ((uintptr_t) datatemp + rings[j].size) ; /* move to next record */
	}
	last->next = (pvoid) rings[j].head ;    /* complete the linked list */
	rings[j].tail = rings[j].head ; /* empty ring */
    }
}
                
/* Return a new pointer to a free block for the indicated type
   of data. Returns NULL if there is none (blocked). The block
   is cleared to zeroes and the blocking mask is set.
*/
tring_elem *getbuffer (short qnum)
{
    tring_elem *bscan, *nbscan ;
      
    bscan = rings[qnum].head ;          /* next in */
    /* why can't the following two lines be combined into
       "if (bscan->next->blockmap)" ??? */
    nbscan = (pvoid) bscan->next ;              /* tail to remove */
    if (nbscan->blockmap)
	return NULL ;                   /* trying to get rid of blocked record */
    rings[qnum].head = nbscan ;         /* move next in pointer */
    memset((pchar) &bscan->user_data, 0, rings[qnum].size -
	   (sizeof(tring_elem) - sizeof(tdata_user))) ; /* clear to zero */
    bscan->blockmap = blockmask ;       /* put in current mask */
    bscan->packet_num = base->next_data++ ; /* packet number */
    if (nbscan == rings[qnum].tail)
	rings[qnum].tail = (pring_elem) rings[qnum].tail->next ; /* throw away oldest */
    return bscan ;
}

/* Return true if a buffer is available in the specified queue */
boolean bufavail (short qnum)
{
    tring_elem *bscan, *nbscan ;
      
    bscan = rings[qnum].head ;          /* next in */
    /* why can't the following two lines be combined into
       "if (bscan->next->blockmap)" ??? */
    nbscan = (pvoid) bscan->next ;              /* tail to remove */
    if (nbscan->blockmap)
	return FALSE ;                  /* trying to get rid of blocked record */
    else
	return TRUE ;
}

/* Returns true if the system is still blocked. If the parameter is not -1
   then a check is made to see if a buffer is available for that data first
   and the mask adjusted appropriately.
*/
boolean checkmask (short qnum)
{
    short j ;

    if (qnum != -1)
	if (bufavail(qnum))
	    clr_bit (&noackmask, qnum) ;
	else
	    set_bit (&noackmask, qnum) ;
    else
	for (j = DATAQ ; j < NUMQ ; j++)
	    if (bufavail(j))
		clr_bit (&noackmask, j) ;
	    else
		set_bit (&noackmask, j) ;
    return (noackmask != 0) ;
}

/* Remove blocking bits for the specified client */
void unblock (short clientnum)
{
    tring_elem *datatemp ;
    short i, j ;
        
    for (j = DATAQ ; j < NUMQ ; j++)
    {
	datatemp = rings[j].head ;
	for (i = 0 ; i < rings[j].count ; i++)
	{
	    clr_bit (&datatemp->blockmap, clientnum) ;    /* not blocked */
	    datatemp = (pvoid) datatemp->next ; /* move to next record */
	}
    }
}
 
/* Add blocking bits for the specified client */
void addblock (short clientnum)
{
    tring_elem *datatemp ;
    short i, j ;
        
    for (j = DATAQ ; j < NUMQ ; j++)
    {
	datatemp = rings[j].head ;
	for (i = 0 ; i < rings[j].count ; i++)
	{
	    set_bit (&datatemp->blockmap, clientnum) ;    /* blocked */
	    datatemp = (pvoid) datatemp->next ; /* move to next record */
	}
    }
}
 
