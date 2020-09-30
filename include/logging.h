/* logging.h for comserv servers */

#ifndef LOGGING_H
#define LOGGING_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include "csconfig.h"

#define CS_LOG_MODE_TO_STDOUT 0
#define CS_LOG_MODE_TO_LOGFILE 1

#define CS_LOG_TYPE_INFO 0
#define CS_LOG_TYPE_DEBUG 1
#define CS_LOG_TYPE_ERROR 2

#ifdef __cplusplus
extern "C" {
#endif
int LogInit(int mode, char *path, char *logname, int buf_size);
int LogMessage (int type, const char *format, ...);
int LogMessage_INFO  (const char *format, ...);
int LogMessage_DEBUG (const char *format, ...);
int LogMessage_ERROR (const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif
