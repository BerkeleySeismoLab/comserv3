/* 
  Modification History:
    2020-09-29 DSN Updated for comserv3.
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "q8_private_station_info.h"

/* GLobal definitions. */
 
#define DEFAULT_STATUS_INTERVAL 100
#define MIN_STATUS_INTERVAL 5
#define MAX_STATUS_INTERVAL 200

#define DEFAULT_PACKETQUEUE_QUEUE_SIZE	500

#define QUOTE(x)        #x
#define STRING(x)       QUOTE(x)

#define APP_IDENT_STRING "q8serv"
#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define RELEASE_VERSION 1
#define RELEASE_DATE "2023.093-beta"
#define APP_VERSION_STRING APP_IDENT_STRING " v" STRING(MAJOR_VERSION) "." STRING(MINOR_VERSION) "." STRING(RELEASE_VERSION) " (" RELEASE_DATE ")"

#define STATION_INI	"station.ini"

extern int log_inited;

// Info only c++ files.
#ifdef __cplusplus
#include "Logger.h"
#include "lib660Interface.h"
extern Logger g_log;
extern Lib660Interface *g_libInterface;
extern PacketQueue *g_packetQueue;
extern bool g_reset;
#endif

#endif
