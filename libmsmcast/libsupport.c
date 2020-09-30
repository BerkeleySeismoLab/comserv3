/*  Lib660 Support routines

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
    Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#include "libsupport.h"
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const dms_type days_mth = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} ;

pchar zpad (pchar s, integer lth)
{
    integer len, diff ;

    len = (integer)strlen(s) ;
    diff = lth - len ;
    if (diff > 0)
    {
	memmove (addr(s[diff]), addr(s[0]), len + 1) ; /* shift existing string right */
	memset (addr(s[0]), '0', diff) ; /* add ascii zeroes at front */
    }
    return s ;
}

word day_julian (word yr, word wmth, word day)
{
    word mth, jday ;

    jday = 0 ;
    mth = 1 ;
    while (mth < wmth)
    {
	if (((yr % 4) == 0) && (mth == 2))
	    jday = jday + 29 ;
	else
	    jday = jday + days_mth[mth] ;
	inc(mth) ;
    }
    return jday + day ;
}

longint lib660_julian (tsystemtime *greg)
{
    word year ;
    longint leap, days ;

    year = greg->wyear - 2016 ;
    leap = (year + 3) shr 2 ; /* leap years so far */
    days = (longint) year * 365 + leap ; /* number of years passed, plus leap days */
    days = days + day_julian (year, greg->wmonth, greg->wday) - 1 ;
    return days * 86400 + (longint) greg->whour * 3600 +
	(longint) greg->wminute * 60 + (longint) greg->wsecond ;
}


double now (void)
{
#define DIFF2016_1970 ((35 * 365) + (11 * 366)) /* Difference between 1970 and 2016 references */
    struct timeval tp;
    struct timezone tzp;
    double r ;

    gettimeofday (addr(tp), addr(tzp)) ;
    r = ((double) tp.tv_sec + ((double) tp.tv_usec / 1000000.0)) ;
    r = r - DIFF2016_1970 * 86400.0 ;
    return r ;
}

longword lib_uround (double r)
{
    longword result ;

    if (r >= 0.0)
	result = r + 0.5 ;
    else
	result = r - 0.5 ;
    return result ;
}

void day_gregorian (word yr, word jday, word *mth, word *day)
{
    word dim ;

    *mth = 1 ;
    do {
	dim = days_mth[*mth] ;
	if (((yr % 4) == 0) && (*mth == 2))
	    dim = 29 ;
	if (jday <= dim)
	    break ;
	jday = jday - dim ;
	inc(*mth) ;
    } while (!(jday > 400)) ; /* just in case */
    *day = jday ;
}

void lib660_gregorian (longint jul, tsystemtime *greg)
{
    longint quads, days, subday, yeartemp ;

    days = jul / 86400 ;
    subday = jul - (days * 86400) ;
    greg->whour = subday / 3600 ;
    subday = subday - ((longint) greg->whour * 3600) ;
    greg->wminute = subday / 60 ;
    greg->wsecond = subday - (greg->wminute * 60) ;
    yeartemp = 2016 ;
    quads = days / 1461 ; /* 0-3 groups */
    incn(yeartemp, quads shl 2) ;
    decn(days, quads * 1461) ;
    if (days >= 366)
    {
	inc(yeartemp) ;
	days = days - 366 ; /* remove leap year part of group */
	while (days >= 365)
	{
	    inc(yeartemp) ;
	    days = days - 365 ;
	}
    }
    greg->wyear = yeartemp ;
    day_gregorian (greg->wyear, days + 1, addr(greg->wmonth), addr(greg->wday)) ;
}

pchar jul_string (longint jul, pchar result)
{
    tsystemtime g ;
    string7 s1 ;

    if (jul < 0)
	strcpy(result, "Invalid Time       ") ;
    else
    {
	lib660_gregorian (jul, addr(g)) ;
	result[0] = 0 ;
	sprintf(result, "%d", g.wyear) ;
	strcat(result, "-") ;
	sprintf(s1, "%d", g.wmonth) ;
	zpad(s1, 2) ;
	strcat(result, s1) ;
	strcat(result, "-") ;
	sprintf(s1, "%d", g.wday) ;
	zpad(s1, 2) ;
	strcat(result, s1) ;
	strcat(result, " ") ;
	sprintf(s1, "%d", g.whour) ;
	zpad(s1, 2) ;
	strcat(result, s1) ;
	strcat(result, ":") ;
	sprintf(s1, "%d", g.wminute) ;
	zpad(s1, 2) ;
	strcat(result, s1) ;
	strcat(result, ":") ;
	sprintf(s1, "%d", g.wsecond) ;
	zpad(s1, 2) ;
	strcat(result, s1) ;
    }
    return result ;
}

pchar packet_time (longint jul, pchar result)
{
    tsystemtime g ;
    string15 s1 ;

    lib660_gregorian (jul, addr(g)) ;
    strcpy(result, "[") ;
    sprintf(s1, "%d", g.whour) ;
    zpad(s1, 2) ;
    strcat(result, s1) ;
    strcat(result, ":") ;
    sprintf(s1, "%d", g.wminute) ;
    zpad(s1, 2) ;
    strcat(result, s1) ;
    strcat(result, ":") ;
    sprintf(s1, "%d", g.wsecond) ;
    zpad(s1, 2) ;
    strcat(result, s1) ;
    strcat(result, "] ") ;
    return result ;
} ;

pchar t64tostring (t64 *val, pchar result)
{
    string15 s1, s2 ;

    sprintf(s1, "%X", (unsigned int)(*val)[0]) ;
    zpad(s1, 8) ;
    sprintf(s2, "%X", (unsigned int)(*val)[1]) ;
    zpad(s2, 8) ;
#ifdef ENDIAN_LITTLE
    strcpy(result, s2) ;
    strcat(result, s1) ;
#else
    strcpy(result, s1) ;
    strcat(result, s2) ;
#endif
    return result ;
}

boolean stringtot64 (pchar s, t64 *value)
{
    integer good, len ;
    string15 sh, sl ;
    string s1 ;
    unsigned int v0, v1 ;

    v0 = 0 ;
    v1 = 0 ;
    strcpy (s1, s) ; /* local copy for indexing */
    len = (integer)strlen(s1) ;
    if (len > 8)
    {
	strncpy(sh, s1, len - 8) ; /* first n - 8 characters */
	sh[len - 8] = 0 ; /* null terminator */
	memcpy(addr(sl), addr(s1[len - 8]), 9) ; /* last 8 characters and terminator */
#ifdef ENDIAN_LITTLE
	good = sscanf(sh, "%x", addr(v1)) ;
	good = good + sscanf(sl, "%x", addr(v0)) ;
#else
	good = sscanf(sh, "%x", addr(v0)) ;
	good = good + sscanf(sl, "%x", addr(v1)) ;
#endif
	if (good != 2)
	    return FALSE ;
    }
    else if (s[0])
    {
#ifdef ENDIAN_LITTLE
	good = sscanf(s, "%x", addr(v0)) ;
#else
	good = sscanf(s, "%x", addr(v1)) ;
#endif
	if (good != 1)
	    return FALSE ;
    }
    (*value)[0] = v0 ;
    (*value)[1] = v1 ;
    return TRUE ;
}

/* If valid IPV6 address sets is6 TRUE and returns in ip6.
   Else if valid IPV4 address sets is6 FALSE and returns IP address.
   Else sets is6 FALSE and returns IPADDR_NONE */
longword get_ip_address (pchar pc, tip_v6 *ip6, boolean *is6, boolean prefer4)
{
    longword ip ;
#ifdef CBUILDERX
    struct hostent *phost ;
    char **listptr ;
    struct in_addr *ptr ;
#else
    struct addrinfo hints ;
    struct addrinfo *result ;
    integer i ;
#endif
    integer ret ;
    integer order[2] ;

    *is6 = FALSE ; /* Assume IPV4 */
#ifdef CBUILDERX
    ip = inet_addr(pc) ;
    if (ip != INADDR_NONE)
	return ip ; /* Good IPV4 address */
    /* No IPV6 address found */
    phost = gethostbyname (pc) ;
    if (phost)
    {
	listptr = phost->h_addr_list ;
	ptr = (struct in_addr *) *listptr ;
	ip = ptr->s_addr ;
	return ip ;
    }
#else
    memclr (addr(hints), sizeof(struct addrinfo)) ;
    if (prefer4)
    {
	order[0] = AF_INET ;
	order[1] = AF_INET6 ;
    }
    else
    {
	order[0] = AF_INET6 ;
	order[1] = AF_INET ;
    }
    for (i = 0 ; i <= 1 ; i++)
	switch (order[i]) {
	case AF_INET6 :
	    hints.ai_family = AF_INET6 ;
	    hints.ai_socktype = SOCK_STREAM ;
	    ret = getaddrinfo (pc, NIL, addr(hints), addr(result));
	    if (ret == 0)
	    {
		*is6 = TRUE ;
		memcpy (ip6, addr(result->ai_addr->sa_data[6]), sizeof(tip_v6)) ;
		freeaddrinfo(result);
		return 0 ; /* Good IPV6 address */
	    }
	    break ;
	case AF_INET :
	    hints.ai_family = AF_INET ;
	    hints.ai_socktype = SOCK_STREAM ;
	    ret = getaddrinfo (pc, NIL, addr(hints), addr(result));
	    if (ret == 0)
	    {
		memcpy (addr(ip), addr(result->ai_addr->sa_data[2]), sizeof(longword)) ;
		freeaddrinfo(result);
		return ip ; /* Good IPV4 address */
	    }
	    break ;
	}
#endif
    return INADDR_NONE ;
}

/* Functions lifted from xmlsup.c */

longint lib_round (double r)
{
    longint result ;

    if (r >= 0.0)
	result = r + 0.5 ;
    else
	result = r - 0.5 ;
    return result ;
}

/* Functions lifted from qlib2 */

/************************************************************************/
/*  trim:                                                               */
/*      Trim trailing blanks from a string.  Return pointer to string.  */
/*  return:                                                             */
/*      pointer to string.                                              */
/************************************************************************/
char *trim
(char *str)                  /* string to trim trailing blanks.      */
{
    char *p = str + strlen(str);
    while (--p >= str)
	if (*p == ' ') *p = '\0'; else break;
    return (str);
}

/************************************************************************/
/*  charncpy:                                                           */
/*      strncpy through N characters, but ALWAYS add NULL terminator.   */
/*      Output string is dimensioned one longer than max string length. */
/*  return:                                                             */
/*      pointer to output string.                                       */
/************************************************************************/
char *charncpy
(char        *out,           /* ptr to output string.                */
 char        *in,            /* ptr to input string.                 */
 int         n)              /* number of characters to copy.        */
{
    char    *p = out;

    while ( (n-- > 0) && (*p++ = *in++) ) ;
    *p = '\0';
    return (out);
}

/************************************************************************/
/*  sps_rate:                                                           */
/*      Compute the samples_per_second given the sample_rate and the    */
/*      sample_rate_mult.                                               */
/************************************************************************/
double sps_rate
(int rate,                   /* sample rate in qlib convention.      */
 int rate_mult)              /* sampe_rate_mult in qlib convention.  */
{
    if (rate != 0 && rate_mult == 0) rate_mult = 1; /* backwards compat.*/
    if (rate == 0 || rate_mult == 0) return (0.);
    if (rate > 0 && rate_mult > 0) return ((double)rate * rate_mult);
    if (rate > 0 && rate_mult < 0) return ((double)-1*rate / rate_mult);
    if (rate < 0 && rate_mult > 0) return ((double)-1*rate_mult / rate);
    if (rate < 0 && rate_mult < 0) return ((double)rate_mult / rate);
    return (0);
}
