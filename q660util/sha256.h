



/*
    Q660 SHA256 definitions Copyright 2016 by
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

REV  DATE        BY   CHANGE
-----------------------------------------------------------------------
  0  2016-09-05  rdr  Created.
  1  2021-12-24  rdr  Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/

#ifndef SHA256_H
#define SHA256_H
#define VER_SHA256 4

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

extern void sha256 (pchar inpmsg, pchar outhash) ;

#endif

