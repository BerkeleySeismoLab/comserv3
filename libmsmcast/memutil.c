/*  Libmsmcaast Memory Utility Routines

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2016 Certified Software Corporation
    Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#include "libtypes.h"
#include "utiltypes.h"
#include "memutil.h"

#ifndef DEFAULT_MEMORY
#define DEFAULT_MEMORY 65536
#endif

integer max_size_requested ;

/* replaces all those memset with the obvious constant of zero */
void memclr (pvoid p, integer sz)
{

    memset (p, 0, sz) ;
}

void getbuf (pmeminst pmem, pointer *p, integer size)
{
    pbyte newblock ;
    pmem_manager pm ;

    pm = pmem->cur_memory ;
    size = (size + 3) & 0xFFFFFFFC ; /* make multiple of longword */
    if (size > max_size_requested)
	max_size_requested = size ;
    if ((pm->sofar + size) > pm->alloc_size)
    { /* need a new block of memory */
        if (pmem->cur_memory->next)
	{ /* already available from before */
	    pmem->cur_memory = pmem->cur_memory->next ;
	    pmem->cur_memory->sofar = 0 ;
	}
	else
	{ /* need new allocation */
	    pm->next = malloc (sizeof(tmem_manager)) ;
	    pm = pm->next ;
	    pm->next = NIL ;
	    if (size > DEFAULT_MEM_INC)
		pm->alloc_size = size ;
	    else
		pm->alloc_size = DEFAULT_MEM_INC ;
	    pm->sofar = 0 ;
	    pm->base = malloc (pm->alloc_size) ;
	    pmem->cur_memory = pm ;
	}
    }
    newblock = pmem->cur_memory->base ;
    incn(newblock, pmem->cur_memory->sofar) ;
    pmem->cur_memory->sofar = pmem->cur_memory->sofar + size ;
    memset (newblock, 0, size) ; /* make sure is zeroed out */
    *p = newblock ;
}

pointer extend_link (pointer base, pointer add)
{
    pptr p ;

    if (base == NIL)
	base = add ; /* first in list */
    else
    {
        p = (pptr) base ;
        while (*p != NIL)
	    p = (pptr) *p ; /* find end of list */
        *p = add ; /* add to end */
    }
    return base ;
}

pointer delete_link (pointer base, pointer del)
{
    pptr p, p2 ;

    p2 = del ;
    if (base == p2)
	base = *p2 ;/* first in list */
    else
    {
        p = base ;
        while ((p) && (*p != p2))
	    p = *p ; /* find entry right before */
        *p = *p2 ; /* skip over del */
    }
    return base ;
}

void initialize_memory_utils (pmeminst pmem)
{
    pmem_manager pm ;

    pm = pmem->memory_head ;
    if (pm)
    { /* memory already allocated, deallocate */
        pmem->cur_memory = pm ;
        while (pm)
	{ /* release  blocks */
            pm->sofar = 0 ;
            pm = pm->next ;
	}
    }
    else
    { /* first allocation */
        pm = malloc (sizeof(tmem_manager)) ;
        pm->next = NIL ;
        pm->alloc_size = DEFAULT_MEMORY ;
        pm->sofar = 0 ;
        pm->base = malloc (pm->alloc_size) ;
        pmem->cur_memory = pm ;
        pmem->memory_head = pm ;
    }
}

