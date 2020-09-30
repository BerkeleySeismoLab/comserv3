#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdint.h>

#include "logging.h"
#include "dpstruc.h"
#include "cfgutil.h"
#include "csconfig.h"

/* Initial code borrowed from Earthworm logit() and logit_init() by 
   Paul Friberg 2005-11-29
  2017-04-24	DSN
    Updated datestamp yyymmdd to use months 1-12, not 0-11.
  2020-04-02    DSN
    Changed close(fp) to fclose(fp) in LogMessage() to properly close log file.
  2020-07-11	DSN
    Changed timestamp to ISO format:  yyyy-mm-ddThh:mm:ss
    Added additional functions to allow redefing printf calls to logging calls.
  29 Sep 2020 DSN Updated for comserv3.
*/

#define         DATE_LEN                16	/** Length of the date field **/
#define		DATETIME_LEN		32	/** Length of the datetime field **/
static FILE   *fp = NULL;
static char   date[DATE_LEN];
static char   datetime[DATETIME_LEN];
static char   date_prev[DATE_LEN];
static char  log_path[1024];
static char  log_name[1024];
static char  log_fullpath[2048+1+16];
static int   log_bufsize;
static int   log_mode;
static char * buf;
static int   init=0;


#define DATE_FORMAT	 "%04d-%02d-%02d"
#define TIME_FORMAT	 "%02d:%02d:%02d"
#define DATETIME_FORMAT DATE_FORMAT "T" TIME_FORMAT

#define LOG_MAX_TYPE 3
char log_type_string[LOG_MAX_TYPE][10]= {"INFO", "DEBUG", "ERROR"};

#define LOGPATH_FORMAT	"%s/%s.%s.log"
/***********************************************************************
 * LogInit initializes the logging file for output.
 *	mode -  a flag to indicate the output mode,
 *		can be  CS_LOG_MODE_TO_STDOUT or CS_LOG_MODE_TO_LOGFILE
 *
 *	path - the directory to put logs into;
 *
 *	logname - a unique name to identify this logfile from any other, left
 *		as an exercise to the user (e.g, station_name+prog_name etc...).
 *		The logfiles will have pathname of
 *			path/logname.ISODATETIME.log
 *		where ISODATETIME is the ISO format datetime
 *			yy-mm-ddThh:mm:ss
 *	buf_size - the size in bytes of the largest message to log
 *
 *	RETURNS 0 upon success, -1 upon failure;
 **********************************************************************/
int LogInit(int mode, char *path, char *logname, int buf_size)
{
    struct tm *gmt_now;
    time_t    now_epoch;

    /* check all input args */
    if (mode==CS_LOG_MODE_TO_LOGFILE && path == NULL) 
    {
	fprintf(stderr, "LogInit(): Fatal Error, path to log dir not provided\n");
	return -1;
    }
    if (logname == NULL) 
    {
	fprintf(stderr, "LogInit(): Fatal Error, logname not provided\n");
	return -1;
    }
    if (buf_size <= 36) 
    {
	fprintf(stderr, "LogInit(): Fatal Error, buf_size of %d needs to be positive (and large)\n", buf_size);
	return -1;
    }
    if (mode == CS_LOG_MODE_TO_STDOUT || mode == CS_LOG_MODE_TO_LOGFILE) 
    {
        log_mode=mode;
    } 
    else 
    {
	fprintf(stderr, "LogInit(): Fatal Error, mode of %d needs to be one of CS_LOG_MODE_TO_STDOUT or CS_LOG_MODE_TO_LOGFILE/n", mode);
	return -1;
    }

    /* store all args  for later */
    if (mode == CS_LOG_MODE_TO_LOGFILE) 
    {
    	strcpy(log_path, path);
    	strcpy(log_name, logname);
    }
    log_bufsize = buf_size;

    /* allocate the message buffer */
    if ( (buf = (char *) malloc( (size_t) log_bufsize)) == NULL)
    {
	fprintf(stderr, "LogInit(): Fatal Error, malloc() of %d bytes failed.\n", buf_size);
	return -1;
    }
    switch (mode) 
    {
  	case CS_LOG_MODE_TO_STDOUT:
	    fp=stdout;
	    break;
  	case CS_LOG_MODE_TO_LOGFILE:
	    time(&now_epoch);
	    gmt_now = gmtime(&now_epoch);
	    sprintf(date, DATE_FORMAT, gmt_now->tm_year + 1900, gmt_now->tm_mon+1, gmt_now->tm_mday);
	    sprintf(datetime, DATETIME_FORMAT, 
		    gmt_now->tm_year + 1900, gmt_now->tm_mon+1, gmt_now->tm_mday,
		    gmt_now->tm_hour, gmt_now->tm_min, gmt_now->tm_sec);
	    strcpy(date_prev, date);
	    sprintf(log_fullpath,LOGPATH_FORMAT, log_path, log_name, date);
	    if ( (fp = fopen(log_fullpath, "a")) == NULL )
	    {
		fprintf(stderr, "LogInit(): Fatal Error, could not open %s for writing\n", log_fullpath);
		return -1;
	    }
	    fprintf(fp, "%s - START - Logfile %s opened\n", datetime, log_fullpath);
	    fflush(fp);
	    break;
    }
    init = 1;
    return 0;
}

/***********************************************************************
 * vLogMessage()
 * Logs the message with date and type tag.
 *	 	yyyy-mm-ddThh:mm:ss - type - message
 *	Arg 1:	CS_LOG_TYPE_INFO, CS_LOG_TYPE_DEBUG, or CS_LOG_TYPE_ERROR
 *	Arg 2:	sprintf format string.
 *	Arg 3:  va_list with sprintf arguments
 *
 *	RETURNS 0 upon success, -1 upon failure
 **********************************************************************/

static int vLogMessage(int type, const char *format, va_list args)
{
    struct tm *gmt_now;
    time_t    now_epoch;
    int  ret;

    if (!init)
    {
	fprintf(stderr, "LogMessage(): Fatal Error, LogInit() not called first\n");
	return -1;
    }
    if (type < 0 || type >= LOG_MAX_TYPE)
    {
	fprintf(stderr, "LogMessage(): Fatal Error, type of %d provided not in range\n", type);
	return -1;
    }

    /* get date for comparison */
    time(&now_epoch);
    gmt_now = gmtime(&now_epoch);
    sprintf(date, DATE_FORMAT, gmt_now->tm_year + 1900, gmt_now->tm_mon+1, gmt_now->tm_mday);
    sprintf(datetime, DATETIME_FORMAT, 
	    gmt_now->tm_year + 1900, gmt_now->tm_mon+1, gmt_now->tm_mday,
	    gmt_now->tm_hour, gmt_now->tm_min, gmt_now->tm_sec);
    
    if (fp != stdout && strcmp(date, date_prev) != 0)
    {
    	fprintf(fp,"%s - END - UTC date changed; Logfile continues in file with date %s\n", datetime, date);
    	fclose(fp);
	sprintf(log_fullpath,LOGPATH_FORMAT, log_path, log_name, date);
	if ( (fp = fopen(log_fullpath, "a")) == NULL )
	{
		fprintf(stderr, "LogMessage(): Fatal Error, could not open new file %s for writing\n", log_fullpath);
		return -1;
	}
    	fprintf(fp,"%s - START - UTC date changed; Logfile continues from with date %s\n", datetime, date_prev);
	strcpy(date_prev, date);
    }

    /* now write to the buffer */
    ret = vsnprintf(buf, log_bufsize, format, args);

    if (ret > log_bufsize || ret == -1)
    {
    	fprintf(fp,"%s - ERROR - LogMessage() called with too big a mesage for buffer specified.\n", datetime);
	return -1;
    }

    if (ret > 1 && buf[ret-1] == '\n')
	fprintf(fp,"%s - %s - %s",   datetime, log_type_string[type], buf);
    else
	fprintf(fp,"%s - %s - %s\n", datetime, log_type_string[type], buf);
    fflush(fp);
    return 0;
}

/***********************************************************************
 * LogMessage()
 * Logs the message as per instructed with a format of the following:
 * 	yyyy-mm-ddThh:mm:ss - type - message
 *	Arg 1:	CS_LOG_TYPE_INFO, CS_LOG_TYPE_DEBUG, or CS_LOG_TYPE_ERROR
 *	Arg 2:	sprintf format string.
 *	Arg 3-N: arguments required by sprintf format.
 *
 *	Calls vLogMessage to perform actual logging.
 *
 *	RETURNS 0 upon success, -1 upon failure
 **********************************************************************/

int LogMessage(int type, const char *format, ...)
{
    auto va_list args;
    int retcode;

    va_start(args, format);
    retcode = vLogMessage(type, format, args);
    va_end(args);
    return retcode;
}

/***********************************************************************
 * LogMessage_INFO()
 * Logs the message tagged as CS_LOG_TYPE_INFO.
 *	Arg 1:	sprintf format string.
 *	Arg 2-N: arguments required by sprintf format.
 *
 *	Calls vLogMessage to perform actual logging.
 *
 *	RETURNS 0 upon success, -1 upon failure
 **********************************************************************/

int LogMessage_INFO(const char *format, ...)
{
    int type = CS_LOG_TYPE_INFO;
    int retcode;
    auto va_list args;

    va_start (args, format);
    retcode = vLogMessage(type, format, args);
    va_end (args);
    return retcode;
}

/***********************************************************************
 * LogMessage_DEBUG()
 * Logs the message tagged as CS_LOG_TYPE_DEBUG.
 *	Arg 1:	sprintf format string.
 *	Arg 2-N: arguments required by sprintf format.
 *
 *	Calls vLogMessage to perform actual logging.
 *
 *	RETURNS 0 upon success, -1 upon failure
 **********************************************************************/

int LogMessage_DEBUG(const char *format, ...)
{
    int type = CS_LOG_TYPE_DEBUG;
    int retcode;
    auto va_list args;

    va_start (args, format);
    retcode = vLogMessage(type, format, args);
    va_end (args);
    return retcode;
}

/***********************************************************************
 * LogMessage_ERROR()
 * Logs the message tagged as CS_LOG_TYPE_ERROR.
 *	Arg 1:	sprintf format string.
 *	Arg 2-N: arguments required by sprintf format.
 *
 *	Calls vLogMessage to perform actual logging.
 *
 *	RETURNS 0 upon success, -1 upon failure
 **********************************************************************/

int LogMessage_ERROR(const char *format, ...)
{
    int type = CS_LOG_TYPE_ERROR;
    int retcode;
    auto va_list args;

    va_start (args, format);
    retcode = vLogMessage(type, format, args);
    va_end (args);
    return retcode;
}
