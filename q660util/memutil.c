/*
     Memory Utility Routines
     Copyright 2016 Certified Software Corporation

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
    0 2016-01-08 rdr Adapted from WS330.
    1 2016-02-06 rdr Add getxmlbuf.
    2 2016-09-07 rdr Made thread safe.
    3 2016-09-23 rdr Make use of already existing memory chunks in the list.
    4 2017-01-24 rdr Fix getbuf to properly do the initial allocations.
    5 2017-06-06 rdr Add maximum size requested for sanity checking.
*/
#include "utiltypes.h"
#include "xmlsup.h"

#ifndef DEFAULT_MEMORY
#define DEFAULT_MEMORY 65536
#endif

integer max_size_requested ;

/* replaces all those memset with the obvious constant of zero */
void memclr (pvoid p, integer sz)
begin

  memset (p, 0, sz) ;
end

void getbuf (pmeminst pmem, pointer *p, integer size)
begin
  pbyte newblock ;
  pmem_manager pm ;

  pm = pmem->cur_memory ;
  size = (size + 3) and 0xFFFFFFFC ; /* make multiple of longword */
  if (size > max_size_requested)
    then
      max_size_requested = size ;
  if ((pm->sofar + size) > pm->alloc_size)
    then
      begin /* need a new block of memory */
        if (pmem->cur_memory->next)
          then
            begin /* already available from before */
              pmem->cur_memory = pmem->cur_memory->next ;
              pmem->cur_memory->sofar = 0 ;
            end
          else
            begin /* need new allocation */
              pm->next = malloc (sizeof(tmem_manager)) ;
              pm = pm->next ;
              pm->next = NIL ;
              if (size > DEFAULT_MEM_INC)
                then
                  pm->alloc_size = size ;
                else
                  pm->alloc_size = DEFAULT_MEM_INC ;
              pm->sofar = 0 ;
              pm->base = malloc (pm->alloc_size) ;
              pmem->cur_memory = pm ;
            end
      end
  newblock = pmem->cur_memory->base ;
  incn(newblock, pmem->cur_memory->sofar) ;
  pmem->cur_memory->sofar = pmem->cur_memory->sofar + size ;
  memset (newblock, 0, size) ; /* make sure is zeroed out */
  *p = newblock ;
end

/* Special routine to get buffer to hold XML file */
void getxmlbuf (pmeminst pmem, pointer *p, integer size)
begin

  size = (size + 3) and 0xFFFFFFFC ;
  if (pmem->xmlbuf)
    then
      begin /* Previously allocated */
        if (size > pmem->xmlsize)
          then
            begin /* need to make it bigger */
              pmem->xmlbuf = realloc (pmem->xmlbuf, size) ;
              pmem->xmlsize = size ;
            end
      end
    else
      begin
        pmem->xmlbuf = malloc (size) ;
        pmem->xmlsize = size ;
      end
  *p = pmem->xmlbuf ;
end

pointer extend_link (pointer base, pointer add)
begin
  pptr p ;

  if (base == NIL)
    then
      base = add ; /* first in list */
    else
      begin
        p = (pptr) base ;
        while (*p != NIL)
          p = (pptr) *p ; /* find end of list */
        *p = add ; /* add to end */
      end
  return base ;
end

pointer delete_link (pointer base, pointer del)
begin
  pptr p, p2 ;

  p2 = del ;
  if (base == p2)
    then
      base = *p2 ;/* first in list */
    else
      begin
        p = base ;
        while ((p) land (*p != p2))
          p = *p ; /* find entry right before */
        *p = *p2 ; /* skip over del */
      end
  return base ;
end

void initialize_memory_utils (pmeminst pmem)
begin
  pmem_manager pm ;

  pm = pmem->memory_head ;
  if (pm)
    then
      begin /* memory already allocated, deallocate */
        pmem->cur_memory = pm ;
        while (pm)
          begin /* release  blocks */
            pm->sofar = 0 ;
            pm = pm->next ;
          end
      end
    else
      begin /* first allocation */
        pm = malloc (sizeof(tmem_manager)) ;
        pm->next = NIL ;
        pm->alloc_size = DEFAULT_MEMORY ;
        pm->sofar = 0 ;
        pm->base = malloc (pm->alloc_size) ;
        pmem->cur_memory = pm ;
        pmem->memory_head = pm ;
      end
end

