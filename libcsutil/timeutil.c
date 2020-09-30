/*$Id: timeutil.c,v 1.2 2005/03/30 22:27:56 isti Exp $*/
/*   Time formats utility module.
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 26 Mar 94 WHO Translated from timeutil.pas
    1  9 Jun 94 WHO Cleanup to avoid warnings.
    2 11 Jun 94 WHO Encode rate added.
    3  9 Aug 94 WHO Add localtime_string function (DSN).
    3 28 Sep 94 WHO Rate translation table updated (320,400.800).
    4 28 Feb 95 WHO Start of conversion to run on OS9.
    5 17 Oct 97 WHO Add VER_TIMEUTIL
    6 29 Sep 2020 DSN Updated for comserv3.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#ifndef _OSK
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#else
#include <time.h>
#endif
#include "quanstrc.h"

#ifdef LINUX
# include <time.h>
#endif

short VER_TIMEUTIL = 5 ;

#define FIRST_YEAR_CONVERTED 1983
#define LAST_YEAR_CONVERTED 2047

static int32_t Julian_Calendar[LAST_YEAR_CONVERTED-FIRST_YEAR_CONVERTED+1] =
{ 0, 31622400,  63158400,  94694400, 126230400,
  157852800, 189388800, 220924800, 252460800,
  284083200, 315619200, 347155200, 378691200,
  410313600, 441849600, 473385600, 504921600,
  536544000, 568080000, 599616000, 631152000,
  662774400, 694310400, 725846400, 757382400,
  789004800, 820540800, 852076800, 883612800,
  915235200, 946771200, 978307200,1009843200,
  1041465600,1073001600,1104537600,1136073600,
  1167696000,1199232000,1230768000,1262304000,
  1293926400,1325462400,1356998400,1388534400,
  1420156800,1451692800,1483228800,1514764800,
  1546387200,1577923200,1609459200,1640995200,
  1672617600,1704153600,1735689600,1767225600,
  1798848000,1830384000,1861920000,1893456000,
  1925078400,1956614400,1988150400,2019686400 } ;

static short Calendar[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 } ;

static short Leap_Calendar[13] =
{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 } ;
                
static short Rate_Translation_Table[256] =
{    0,    1,    2,    3,    4,    5,    6,    7,
     8,    9,   10,   11,   12,   13,   14,   15,
     16,   17,   18,   19,   20,   21,   22,   23,
     24,   25,   26,   27,   28,   29,   30,   31,
     32,   33,   34,   35,   36,   37,   38,   39,
     40,   41,   42,   43,   44,   45,   46,   47,
     48,   49,   50,   51,   52,   53,   54,   55,
     56,   57,   58,   59,   60,   61,   62,   63,
     64,   65,   66,   67,   68,   69,   70,   71,
     72,   73,   74,   75,   76,   77,   78,   79,
     80,   81,   82,   83,   84,   85,   86,   87,
     88,   89,   90,   91,   92,   93,   94,   95,
     96,   97,   98,   99,  100,  120,  125,  150,  /* 103 */
     /* 104 */     160,  180,  200,  250,  320,  400,  500,  800,
     1000,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,-3600,  /* -113 */
     /* -112 */  -1800,-1200,-1000, -800, -600, -500, -400, -320,  /* -105 */
     /* -104 */   -250, -200, -180, -120, -100,  -99,  -98,  -97,
     -96,  -95,  -94,  -93,  -92,  -91,  -90,  -89,
     -88,  -87,  -86,  -85,  -84,  -83,  -82,  -81,
     -80,  -79,  -78,  -77,  -76,  -75,  -74,  -73,
     -72,  -71,  -70,  -69,  -68,  -67,  -66,  -65,
     -64,  -63,  -62,  -61,  -60,  -59,  -58,  -57,
     -56,  -55,  -54,  -53,  -52,  -51,  -50,  -49,
     -48,  -47,  -46,  -45,  -44,  -43,  -42,  -41,
     -40,  -39,  -38,  -37,  -36,  -35,  -34,  -33,
     -32,  -31,  -30,  -29,  -28,  -27,  -26,  -25,
     -24,  -23,  -22,  -21,  -20,  -19,  -18,  -17,
     -16,  -15,  -14,  -13,  -12,  -11,  -10,   -9,
     -8,   -7,   -6,   -5,   -4,   -3,   -2,   -1 } ;

extern int32_t longhex (byte b) ;
extern short ord (byte b) ;

/* return true if leap year */
boolean is_leap (short year)
{
    return ((year % 4) == 0) ;         /* this will work until 2099 */
}

/* Return days in month for specified year and month */
short days_in_month (short year, short month)
{
    if (is_leap (year))
	return Leap_Calendar [month] - Leap_Calendar [month - 1] ;
    else
	return Calendar [month] - Calendar [month - 1] ;
}

/* Return seconds since 1970 representation of gregorian time */
int32_t julian (time_array *gt)
{
    int32_t temp ;

    temp = Julian_Calendar[gt->year + 1899 - FIRST_YEAR_CONVERTED] ;
    if (is_leap (gt->year))
	temp = temp + (Leap_Calendar[gt->month - 1] * 86400) ;
    else
	temp = temp + (Calendar[gt->month -1] * 86400) ;
    temp = temp + (longhex(gt->day - 1) * 86400) + (longhex(gt->hour) * 3600) +
	(longhex(gt->min) * 60) + longhex(gt->sec) ;
    return temp + SECCOR ;
}

/* Convert quanterra byte representation of sampling rate to integer version */
short decode_rate (byte rate)
{
    return Rate_Translation_Table[ord(rate)] ;
}
    
signed char encode_rate (short rate)
{
    short rate_table_scan, er ;

    er = 0 ;
    if (rate >= 0)
    {
	if (rate <= 100)
	    er = rate ;
	else
	{
	    rate_table_scan = 101 ;
	    while ((rate_table_scan <= 127) &&
		   (Rate_Translation_Table [rate_table_scan] > 0))
	    {
		if (rate == Rate_Translation_Table [rate_table_scan])
		{
		    er = rate_table_scan ;
		    break ;
		}
		rate_table_scan++ ;
	    }
	}
    }
    else if (rate >= -100)
	er = rate ;
    else
    {
	rate_table_scan = ord(-101) ;
	while ((ord(rate_table_scan) >= 128) &&
	       (Rate_Translation_Table [ord(rate_table_scan)] < 0))
	{
	    if (rate == Rate_Translation_Table [ord(rate_table_scan)])
	    {
		er = rate_table_scan ;
		break ;
	    }
	    rate_table_scan-- ;
	}
    }
    return (signed char) er ;
}
    
/* Find julian day given time structure */
short julian_day (time_array *gt)
{
    short year, month, day ;

    year = ord (gt->year) + 1900 ;
    month = ord (gt->month) ;
    if (month < 1)
	month = 1 ;
    if (month > 12)
	month = 12 ;
    day = ord (gt->day) ;
    if (day < 1)
	day = 1 ;
    if (day > days_in_month (year, month))
	day = days_in_month (year, month) ;
    if (is_leap (year))
	return Leap_Calendar [month - 1] + day ;
    else
	return Calendar [month - 1] + day ;
}

/* utility routine to add leading characters to a string */
pchar lead (short col, char c, pchar s)
{
    static char w[80] ;
    short j, len ;

    w[0] = '\0' ;
    len = col - strlen(s) ;
    if (len > 0)
	for (j = 0 ; j < len ; j++)
	{
	    w[j] = c ;
	    w[j + 1] = '\0' ;
	}
    strcat (w, s) ;
    return (pchar) &w ;
}
    
/* return pointer to static string that contains ascii version of time */
pchar time_string (double jul)
{
    time_t seconds ;
    int32_t usecs ;
    static char tmp2[32] ;
    char tmp[26], tmp3[10] ;

    seconds = jul ;
    usecs = (jul - seconds) * 1000000.0 + 0.5 ;
    if (usecs < 0)
	usecs = 0 ;
    if (usecs > 999999)
	usecs = 999999 ;
#ifdef _OSK
    strcpy(tmp, (pchar) asctime(gmtime(& (time_t) seconds))) ; /* convert julian to gmt string */
#else
    strcpy(tmp, (pchar) asctime(gmtime((time_t *)&seconds))) ; /* convert julian to gmt string */
#endif
    strncpy(tmp2, &tmp[11], 8) ; /* pick out time */
    tmp2[8] = '\0' ;
    sprintf(tmp3, "%-8d", usecs + 1000000) ; /* convert usecs to string */
    strcat(tmp2, ".") ; /* separator */
    strcat(tmp2, &tmp3[1]) ; /* merge time and usecs */
    strncat(tmp2, tmp, 11) ; /* get day of week, month, and day */
    tmp2[27] = '\0' ;
    strncat(tmp2, &tmp[20], 4) ; /* add on year */
    tmp2[31] = '\0' ;
    return (pchar) &tmp2 ;
}
 
#ifdef _OSK
pchar localtime_string (double jul)
{
    return time_string (jul) ;
}
#else
/* return pointer to static string that contains ascii version of time */
pchar localtime_string (double jul)
{
    time_t seconds ;
    int32_t usecs ;
    static char tmp2[32] ;
    char tmp[26], tmp3[10] ;

    seconds = jul ;
    usecs = (jul - seconds) * 1000000.0 + 0.5 ;
    if (usecs < 0)
	usecs = 0 ;
    if (usecs > 999999)
	usecs = 999999 ;
    strcpy(tmp, (pchar) asctime(localtime((time_t *)&seconds))) ; /* convert julian to local time string */
    strncpy(tmp2, &tmp[11], 8) ; /* pick out time */
    tmp2[8] = '\0' ;
    sprintf(tmp3, "%-8d", usecs + 1000000) ; /* convert usecs to string */
    strcat(tmp2, ".") ; /* separator */
    strcat(tmp2, &tmp3[1]) ; /* merge time and usecs */
    strncat(tmp2, tmp, 11) ; /* get day of week, month, and day */
    tmp2[27] = '\0' ;
    strncat(tmp2, &tmp[20], 4) ; /* add on year */
    tmp2[31] = '\0' ;
    return (pchar) &tmp2 ;
}
#endif
    
/* Get seconds since 1970 given the year and julian day */
int32_t jconv (short yr, short jday)
{
    time_array tempta ;
    int32_t tempj ;

    tempta.year = yr ;
    tempta.month = 1 ;
    tempta.day = 1 ;
    tempta.hour = 0 ;
    tempta.min = 0 ;
    tempta.sec = 0 ;
    tempj = julian(&tempta) ; /* get seconds for beginning of this year */
    return tempj + (jday - 1) * 86400 ; /* add in days this year */
}

/* Passed the seconds since 1984, returns gregorian time in the supplied structure */
void gregorian (int32_t jdate, time_array *ret)
{
    typedef short caltype[13] ;
    short year ;
    short month ;
    short day ;
    short hour ;
    short min ;
    short sec ;
    int32_t jsec ;
    short jday ;
    caltype *cal ;

    if (jdate < 0)
	jdate = 0 ;
    year = 1984 ;
    while ((year <= LAST_YEAR_CONVERTED) &&
	   (jdate >= Julian_Calendar[year - FIRST_YEAR_CONVERTED]))
        year++ ;
    jsec = jdate - (Julian_Calendar [year - 1 - FIRST_YEAR_CONVERTED]) ;
    jday = (jsec / 86400) + 1 ;
    if (is_leap (year))
	cal = &Leap_Calendar ;
    else
	cal = &Calendar ;
    month = 1 ;
    while ((month <= 12) && (jday > (*cal)[month]))
        month++ ;
    day = jday - (*cal)[month - 1] ;
    jsec = jsec % 86400 ;
    hour = jsec / 3600 ;
    jsec = jsec - (longhex(hour) * 3600) ;
    min = jsec / 60 ;
    sec = jsec - (min * 60) ;
    ret->year = year - 1900 ;
    ret->month = month ;
    ret->day = day ;
    ret->hour = hour ;
    ret->min = min ;
    ret->sec = sec ;
}
