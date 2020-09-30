/* 
  Modification History:
    2007-09-13 - added DSN mods for signal trapping, cserv little endian changes from DSN
        and new lib330 from Bob R.
    2007-10-24 - added in slate computer endian inclusions and other changes from Frank Shelly
    2008-02-26 - added in new cLib330 from Certsoft (2008-02-25 version)
    2008-04-02 - v2.0.2 added in a newer cLib330 from Certsoft (SVN for cLib330 r56) (2008-03-20 date)
    2008-04-04 - v2.0.3 added in a newer cLib330 from Certsoft (SVN for cLib330 r57) (2008-04-03 date)
    2011-? - v2.0.4 - kalpesh had an intermediate version with multicast fixed for x86 platforms
    2012-01-11 - v2.0.5 - paulf version with newer compiler fixes 
    2012-02-06 - v2.0.6 - DSN version with limit to RLIMIT_NOFILE.
    2020-09-29 DSN Updated for comserv3.
*/
 
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/* GLobal definitions. */
 
#define MAX_CHARS_IN_SERVER_NAME 16

#define DEFAULT_STATUS_INTERVAL 100
#define MIN_STATUS_INTERVAL 5
#define MAX_STATUS_INTERVAL 200

#define DEFAULT_DATA_RATE_INTERVAL 3
#define MIN_DATA_RATE_INTERVAL 1
#define MAX_DATA_RATE_INTERVAL 100

#define QUOTE(x)        #x
#define STRING(x)       QUOTE(x)

#define APP_IDENT_STRING "q330serv"
#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define RELEASE_VERSION 0
#define APP_VERSION_STRING APP_IDENT_STRING " v" STRING(MAJOR_VERSION) "." STRING(MINOR_VERSION) "." STRING(RELEASE_VERSION) " beta 6"

#define NETWORK_INI	"/etc/network.ini"
#define STATIONS_INI	"/etc/stations.ini"
#define STATION_INI	"station.ini"

extern int log_inited;

// Info only c++ files.
#ifdef __cplusplus
#include "Logger.h"
#include "lib330Interface.h"
extern Logger g_log;
extern Lib330Interface *g_libInterface;
extern bool g_reset;
#endif

#endif
