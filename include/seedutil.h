/* Functions in seedutil.c */

#ifndef SEEDUTIL_H
#define SEEDUTIL_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include "dpstruc.h"
#include "quanstrc.h"
#include "seedstrc.h"

#ifdef __cplusplus
extern "C" {
#endif

double seedblocks (seed_record_header *sl, commo_record *log);
/* given the seed channel name and location, returns a string in the format LL-CCC */
pchar seednamestring (seed_name_type *sd, location_type *loc);
/* given the seed station_ID return a string in the format SSSSS */
pchar seedstationidstring (char  *in);
/* given the seednet return a string in the format NN */
pchar seednetstring (char  *in);

#ifdef __cplusplus
}
#endif

#endif
