



/*   Memory Utility definitions
     Copyright 2016 by
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
    0 2016-01-08 rdr Adapted from WS330.
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef MEMUTIL_H
#define MEMUTIL_H
#define VER_MEMUTIL 6

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

#ifndef DEFAULT_MEM_INC
#define DEFAULT_MEM_INC 16384
#endif

typedef struct tmem_manager   /* Linked list of memory segments for token expansion and buffers */
{
    struct tmem_manager *next ; /* next block */
    int alloc_size ; /* allocated size of this block */
    int sofar ; /* amount used in this block */
    pointer base ; /* start of the allocated memory in this block */
} tmem_manager ;
typedef tmem_manager *pmem_manager ;

typedef pointer *pptr ;

typedef struct         /* An instance of a memory manager */
{
    pmem_manager memory_head ; /* start of memory management blocks */
    pmem_manager cur_memory ; /* current block we are allocating from */
    pointer xmlbuf ; /* Address of XML buffer */
    int xmlsize ; /* Size of XML buffer */
    int cur_memory_required ; /* for continuity, if required */
} tmeminst ;
typedef tmeminst *pmeminst ;

extern void memclr (pointer p, int sz) ;
extern void getbuf (pmeminst pmem, pointer *p, int size) ;
extern void getxmlbuf (pmeminst pmem, pointer *p, int size) ;
extern pointer extend_link (pointer base, pointer add) ;
extern pointer delete_link (pointer base, pointer del) ;
extern void initialize_memory_utils (pmeminst pmem) ;

#endif
