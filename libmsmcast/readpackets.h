/*  Libmsmcast Data & Status Record Constant and Type definitions

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2016, 2017 by Certified Software Corporation
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

#ifndef READPACKETS_H
#define READPACKETS_H
#define VER_READPACKETS 12

#ifndef UTILTYPES_H
#include "libtypes.h"
#endif

//::typedef string31 tlogfields[LOGF_SIZE] ;

/* Data Packet Codes */
#define DT_DATA 0           /* Normal Data */
#define DT_LLDATA 1         /* Low Latency Data */
#define DT_STATUS 2         /* Status */

extern byte loadbyte (pbyte *p) ;
extern shortint loadshortint (pbyte *p) ;
extern word loadword (pbyte *p) ;
extern int16 loadint16 (pbyte *p) ;
extern longword loadlongword (pbyte *p) ;
extern longint loadlongint (pbyte *p) ;
extern single loadsingle (pbyte *p) ;
extern double loaddouble (pbyte *p) ;
extern void loadstring (pbyte *p, integer fieldwidth, pchar s) ;
extern void loadt64 (pbyte *p, t64 *six4) ;
extern void loadblock (pbyte *p, integer size, pointer pdest) ;

#endif

