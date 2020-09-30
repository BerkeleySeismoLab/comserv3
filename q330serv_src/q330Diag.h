/*
 * File     :
 *  q330Diag.h
 *
 * Purpose  :
 *  These are diagnostic values used by the program.
 *  The standard values specified by the program are included here.
 *  Also specific target are defined here, to give the ability to
 *  print specific diagnostic statements if needed.
 *
 * Author   :
 *  Phil Maechling.
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Mod Date :
 *  27 July 2002
 *  2020-09-29 DSN Updated for comserv3.
 */

#ifndef Q330DIAG_H
#define Q330DIAG_H

const int D_SILENT        = 0;
const int D_MAJOR         = 1;
const int D_MINOR         = 2;
const int D_EVERYTHING    = 3;

//
// These are specific targets in the source code.
// The software will print diagnostics about the
// routines in the log file, if you select one
// of interest.
//
const int D_NO_TARGET	       = 0;
const int D_SHOW_CONFIG        = 1;
const int D_SHOW_STATE         = 2;

#endif
