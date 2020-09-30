/*  private_station_info.h
 *
 *  Private station info structure for use with lib660.
 *
 *  29 Sep 2020 DSN Updated for comserv3.
 */

#ifndef __Q8_PRIVATE_STATION_INFO_H__
#define __Q8_PRIVATE_STATION_INFO_H__
#include <linux/limits.h>

/* Structure for storing private station-specific info. */
typedef struct _private_station_info {
  char stationName[16];
  char conf_path_prefix[PATH_MAX];
} private_station_info;

#endif
