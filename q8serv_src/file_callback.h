/* 
 * File: file_call.back.h
 *
 * Defines info needed for file open, close, read, write operations
 * from lib660 routines.
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 */

#include <linux/limits.h>

#include "platform.h"
#include "q8_private_station_info.h"
 
/* PROG_DL needs to be defined to include proper parts of q660util definitions. */
#ifndef	PROG_DL
#define PROG_DL
#endif

#include "xmlcfg.h"
#include "xmlseed.h"
#include "libclient.h"
#include "libsupport.h"

enum tfilekind {FK_UNKNOWN, FK_CFG, FK_CONT, FK_IDXDAT} ;
