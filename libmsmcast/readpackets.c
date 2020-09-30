/*  Libmsmcast Data & Status Record Conversion Routines

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2016 - 2019 Certified Software Corporation
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
#include "readpackets.h"

byte loadbyte (pbyte *p)
{
  byte temp ;

  temp = **p ;
  incn(*p, 1) ;
  return temp ;
}

shortint loadshortint (pbyte *p)
{
  byte temp ;

  temp = **p ;
  incn(*p, 1) ;
  return (shortint) temp ;
}

word loadword (pbyte *p)
{
  word w ;

  memcpy(addr(w), *p, 2) ;
#ifdef ENDIAN_LITTLE
  w = ntohs(w) ;
#endif
  incn(*p, 2) ;
  return w ;
}

int16 loadint16 (pbyte *p)
{
  int16 i ;

  memcpy(addr(i), *p, 2) ;
#ifdef ENDIAN_LITTLE
  i = ntohs(i) ;
#endif
  incn(*p, 2) ;
  return i ;
}

longword loadlongword (pbyte *p)
{
  longword lw ;

  memcpy(addr(lw), *p, 4) ;
#ifdef ENDIAN_LITTLE
  lw = ntohl(lw) ;
#endif
  incn(*p, 4) ;
  return lw ;
}

longint loadlongint (pbyte *p)
{
  longint li ;

  memcpy(addr(li), *p, 4) ;
#ifdef ENDIAN_LITTLE
  li = ntohl(li) ;
#endif
  incn(*p, 4) ;
  return li ;
}

single loadsingle (pbyte *p)
{
  longint li, exp ;
  single s ;

  memcpy(addr(li), *p, 4) ;
#ifdef ENDIAN_LITTLE
  li = ntohl(li) ;
#endif
  exp = (li shr 23) & 0xFF ;
  if ((exp < 1) || (exp > 254))
      li = 0 ; /* replace with zero */
  else if (li == (longint)0x80000000)
      li = 0 ; /* replace with positive zero */
  memcpy(addr(s), addr(li), 4) ;
  incn(*p, 4) ;
  return s ;
}

double loaddouble (pbyte *p)
{
  double d ;
  plword plw ;

  plw = (plword)addr(d) ;
#ifdef DOUBLE_HYBRID_ENDIAN
  *plw = loadlongword (p) ;
  inc(plw) ;
  *plw = loadlongword (p) ;
#else
#ifdef ENDIAN_LITTLE
  inc(plw) ;
  *plw = loadlongword (p) ;
  plw = (plword)addr(d) ;
  *plw = loadlongword (p) ;
#else
  *plw = loadlongword (p) ;
  inc(plw) ;
  *plw = loadlongword (p) ;
#endif
#endif
  return d ;
}

void loadstring (pbyte *p, integer fieldwidth, pchar s)
{

  memcpy(s, *p, fieldwidth) ;
  incn(*p, fieldwidth) ;
}

void loadt64 (pbyte *p, t64 *six4)
{

#ifdef ENDIAN_LITTLE
  (*six4)[1] = loadlongword (p) ;
  (*six4)[0] = loadlongword (p) ;
#else
  (*six4)[0] = loadlongword (p) ;
  (*six4)[1] = loadlongword (p) ;
#endif
}

void loadblock (pbyte *p, integer size, pointer pdest)
{

  memcpy(pdest, *p, size) ;
  incn(*p, size) ;
}

