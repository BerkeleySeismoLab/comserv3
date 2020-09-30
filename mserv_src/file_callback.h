/* file_callback.h
 *
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#ifndef FILE_CALLBACK_H
#define FILE_CALLBACK_H

#include <linux/limits.h>

/* PROG_DL needs to be defined to include proper parts of q660util definitions. */
#ifndef	PROG_DL
#define PROG_DL
#endif

#include "libclient.h"
#include "libsupport.h"

enum tfilekind {FK_UNKNOWN, FK_CFG, FK_CONT, FK_IDXDAT} ;

#endif
