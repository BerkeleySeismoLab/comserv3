/*  private_station_info.h
 *
 *  Private station info structure for use wth libmsmcast.
 *
 *  29 Sep 2020 DSN Updated for comserv3.
 */

#ifndef __PRIVATE_STATION_INFO_H__
#define __PRIVATE_STATION_INFO_H__

#include <linux/limits.h>

#include "cstypes.h"

/* Structure for storing private station-specific info. */
typedef struct _private_station_info {
    tservername stationName;
    char conf_path_prefix[PATH_MAX];
} private_station_info;

#endif
