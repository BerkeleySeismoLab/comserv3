/*  Libmsmcast time series handling definitions

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
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

#ifndef LIBSAMPGLOB_H
#define LIBSAMPGLOB_H
#define VER_LIBSAMPGLOB 1

#include "libtypes.h"
#include "libseed.h"
#include "libclient.h"

/* Send to Client destination bitmaps */
#define SCD_ARCH 1 /* send to archival output */
#define SCD_512 2 /* send to 512 byte miniseed output */
#define SCD_BOTH 3 /* send to both */

#define MAXSAMP 38
#define FIRMAXSIZE 400
#define MAXPOLES 8       /* Maximum number of poles in recursive filters */
#define FILTER_NAME_LENGTH 31 /* Maximum number of characters in an IIR filter name */
#define PEEKELEMS 16
#define PEEKMASK 15 /* TP7 doesn't optimize mod operation */
#define CFG_TIMEOUT 120 /* seconds since last config blockette added before flush */
#define LOG_TIMEOUT 120 /* seconds since last message line added before flush */
#define Q_OFF 80 /* Clock quality for GPS lock but no PLL lock */
#define Q_LOW 10 /* Minimum quality for data */
#define NO_LAST_DATA_QUAL 999 /* initial value */

typedef struct tlcq {
  struct tlcq *link ; /* forward link */
} tlcq ;
typedef tlcq *plcq ;

typedef struct tmsgqueue {
  struct tmsgqueue *link ;
  string250 msg ;
} tmsgqueue ;
typedef tmsgqueue *pmsgqueue ;

#endif
