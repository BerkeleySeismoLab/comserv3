/*
 *
 * File     :
 *  qservdefs.h
 *
 * Purpose  :
 *  These types are used to make sure that all types are correctly
 *  defined on all platforms.
 *
 * The storage size is primarily an issue on data that is going on or
 * off the wire. If the wrong type is used, data can be mis-interpreted.
 *
 * Author   :
 *  Phil Maechling
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
 *
 * Modification History:
 *  17 March 2002
 *  29 Sep 2020 DSN Updated for comserv3.
 */

#ifndef QSERVDEFS_H
#define QSERVDEFS_H

//
// Simple return result values. We can refer to success and failure
// not 1 and -1
//

const int QSERV_SUCCESS  = 1;
const int QSERV_FAILURE = -1;

// The following MUST be identical to the values in quanstrc.h
#define EMPTY 0
#define RECORD_HEADER_1 1
#define RECORD_HEADER_2 2
#define CLOCK_CORRECTION 3
#define COMMENTS 4
#define DETECTION_RESULT 5
#define RECORD_HEADER_3 6
#define BLOCKETTE 7
#define FLOOD_PKT 8
#define END_OF_DETECTION 9
#define CALIBRATION 10
#define FLUSH 11
#define LINK_PKT 12
#define SEED_DATA 13
#define ULTRA_PKT 16
#define DET_AVAIL 17
#define CMD_ECHO 18
#define UPMAP 19
#define DOWNLOAD 20

#endif
