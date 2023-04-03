/* 
    2007-09-13 - added DSN mods for signal trapping, cserv little endian changes from DSN
        and new lib660 from Bob R.
    2007-10-24 - added in slate computer endian inclusions and other changes from Frank Shelly
    2008-02-26 - added in new cLib660 from Certsoft (2008-02-25 version)
    2008-04-02 - v2.0.2 added in a newer cLib660 from Certsoft (SVN for cLib660 r56) (2008-03-20 date)
    2008-04-04 - v2.0.3 added in a newer cLib660 from Certsoft (SVN for cLib660 r57) (2008-04-03 date)
    2011-? - v2.0.4 - kalpesh had an intermediate version with multicast fixed for x86 platforms
    2012-01-11 - v2.0.5 - paulf version with newer compiler fixes 
    2012-02-06 - v2.0.6 - DSN version with limit to RLIMIT_NOFILE.
    2020-09-29 DSN Updated for comserv3.
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "private_station_info.h"

/* GLobal definitions. */
 
#define DEFAULT_STATUS_INTERVAL 100
#define MIN_STATUS_INTERVAL 5
#define MAX_STATUS_INTERVAL 200

#define DEFAULT_PACKETQUEUE_QUEUE_SIZE	500

#define QUOTE(x)        #x
#define STRING(x)       QUOTE(x)

#define APP_IDENT_STRING "mserv"
#define MAJOR_VERSION 2
#define MINOR_VERSION 0
#define RELEASE_VERSION 3
#define RELEASE_DATE "2023.093-beta"
#define APP_VERSION_STRING APP_IDENT_STRING " v" STRING(MAJOR_VERSION) "." STRING(MINOR_VERSION) "." STRING(RELEASE_VERSION) " (" RELEASE_DATE ")"

#define STATION_INI	"station.ini"

extern int log_inited;

// Info only c++ files.
#ifdef __cplusplus
#include "Logger.h"
#include "libmsmcastInterface.h"
extern Logger g_log;
extern LibmsmcastInterface *g_libInterface;
extern PacketQueue *g_packetQueue;
extern bool g_reset;
#endif

#endif
