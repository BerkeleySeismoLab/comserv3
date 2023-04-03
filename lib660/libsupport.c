



/*   Lib660 Support routines
     Copyright 2017-2020 by
     Kinemetrics, Inc.
     Pasadena, CA 91107 USA.

    This file is part of Lib660

    Lib660 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Lib660 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2017-06-11 rdr Created
    1 2017-06-15 rdr Fix now for non-windows to use 2016 instead of 2000 ref.
                     get_ip_address rewritten to use getaddrinfo for non-CBUILDERX
                     builds.
    2 2017-06-17 rdr File access routines re-written. All file accesses actually
                     occur in the now mandatory file calback.
    3 2017-06-24 rdr In get_ip_address when doing a lookup check IPV4 first if prefer4
                     is TRUE, else check IPV6 first.
    4 2020-02-14 jms getaddrinfo must be followed by freeaddrinfo() not to leak memory
    5 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef libsupport_h
#include "libsupport.h"
#endif
#include <ctype.h>
#include <stdio.h>
#ifndef X86_WIN32
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

const dms_type days_mth = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} ;

pchar zpad (pchar s, int lth)
{
    int len, diff ;

    len = (int)strlen(s) ;
    diff = lth - len ;

    if (diff > 0) {
        memmove (&(s[diff]), &(s[0]), len + 1) ; /* shift existing string right */
        memset (&(s[0]), '0', diff) ; /* add ascii zeroes at front */
    }

    return s ;
}

U16 day_julian (U16 yr, U16 wmth, U16 day)
{
    U16 mth, jday ;

    jday = 0 ;
    mth = 1 ;

    while (mth < wmth) {
        if (((yr % 4) == 0) && (mth == 2))
            jday = jday + 29 ;
        else
            jday = jday + days_mth[mth] ;

        (mth)++ ;
    }

    return jday + day ;
}

I32 lib660_julian (tsystemtime *greg)
{
    U16 year ;
    I32 leap, days ;

    year = greg->wyear - 2016 ;
    leap = (year + 3) >> 2 ; /* leap years so far */
    days = (I32) year * 365 + leap ; /* number of years passed, plus leap days */
    days = days + day_julian (year, greg->wmonth, greg->wday) - 1 ;
    return days * 86400 + (I32) greg->whour * 3600 +
           (I32) greg->wminute * 60 + (I32) greg->wsecond ;
}


#ifdef X86_WIN32
double now (void)
{
    SYSTEMTIME sgreg ;
    tsystemtime nowgreg ;
    double r ;

    GetSystemTime (&(sgreg)) ;
    memcpy (&(nowgreg), &(sgreg), sizeof(tsystemtime)) ; /* copy to internal version */
    r = lib660_julian (&(nowgreg)) ; /* get whole seconds since 2000 */
    r = r + nowgreg.wmilliseconds / 1000.0 ;
    return r ;
}

#else

double now (void)
{
#define DIFF2016_1970 ((35 * 365) + (11 * 366)) /* Difference between 1970 and 2016 references */
    struct timeval tp;
    struct timezone tzp;
    double r ;

    gettimeofday (&(tp), &(tzp)) ;
    r = ((double) tp.tv_sec + ((double) tp.tv_usec / 1000000.0)) ;
    return r - DIFF2016_1970 * 86400.0 ;
}
#endif

U32 lib_uround (double r)
{
    U32 result ;

    if (r >= 0.0)
        result = r + 0.5 ;
    else
        result = r - 0.5 ;

    return result ;
}

void day_gregorian (U16 yr, U16 jday, U16 *mth, U16 *day)
{
    U16 dim ;

    *mth = 1 ;

    do {
        dim = days_mth[*mth] ;

        if (((yr % 4) == 0) && (*mth == 2))
            dim = 29 ;

        if (jday <= dim)
            break ;

        jday = jday - dim ;
        (*mth)++ ;
    } while (! (jday > 400)) ; /* just in case */

    *day = jday ;
}

void lib660_gregorian (I32 jul, tsystemtime *greg)
{
    I32 quads, days, subday, yeartemp ;

    days = jul / 86400 ;
    subday = jul - (days * 86400) ;
    greg->whour = subday / 3600 ;
    subday = subday - ((I32) greg->whour * 3600) ;
    greg->wminute = subday / 60 ;
    greg->wsecond = subday - (greg->wminute * 60) ;
    yeartemp = 2016 ;
    quads = days / 1461 ; /* 0-3 groups */
    yeartemp = yeartemp + (quads << 2) ;
    days = days - (quads * 1461) ;

    if (days >= 366) {
        (yeartemp)++ ;
        days = days - 366 ; /* remove leap year part of group */

        while (days >= 365) {
            (yeartemp)++ ;
            days = days - 365 ;
        }
    }

    greg->wyear = yeartemp ;
    day_gregorian (greg->wyear, days + 1, &(greg->wmonth), &(greg->wday)) ;
}

pchar jul_string (I32 jul, pchar result)
{
    tsystemtime g ;
    string7 s1 ;

    if (jul < 0)
        strcpy(result, "Invalid Time       ") ;
    else {
        lib660_gregorian (jul, &(g)) ;
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

pchar packet_time (I32 jul, pchar result)
{
    tsystemtime g ;
    string15 s1 ;

    lib660_gregorian (jul, &(g)) ;
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

BOOLEAN stringtot64 (pchar s, t64 *value)
{
    int good, len ;
    string15 sh, sl ;
    string s1 ;
    unsigned int v0, v1 ;

    v0 = 0 ;
    v1 = 0 ;
    strcpy (s1, s) ; /* local copy for indexing */
    len = (int)strlen(s1) ;

    if (len > 8) {
        strncpy(sh, s1, len - 8) ; /* first n - 8 characters */
        sh[len - 8] = 0 ; /* null terminator */
        memcpy(&(sl), &(s1[len - 8]), 9) ; /* last 8 characters and terminator */
#ifdef ENDIAN_LITTLE
        good = sscanf(sh, "%x", &(v1)) ;
        good = good + sscanf(sl, "%x", &(v0)) ;
#else
        good = sscanf(sh, "%x", &(v0)) ;
        good = good + sscanf(sl, "%x", &(v1)) ;
#endif

        if (good != 2)
            return FALSE ;
    } else if (s[0]) {
#ifdef ENDIAN_LITTLE
        good = sscanf(s, "%x", &(v0)) ;
#else
        good = sscanf(s, "%x", &(v1)) ;
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
U32 get_ip_address (pchar pc, tip_v6 *ip6, BOOLEAN *is6, BOOLEAN prefer4)
{
    U32 ip ;
#ifdef CBUILDERX
    struct hostent *phost ;
    char **listptr ;
    struct in_addr *ptr ;
#else
    struct addrinfo hints ;
    struct addrinfo *result ;
    int i ;
#endif
    int ret ;
    int order[2] ;

    *is6 = FALSE ; /* Assume IPV4 */
#ifdef CBUILDERX
    ip = inet_addr(pc) ;

    if (ip != INADDR_NONE)
        return ip ; /* Good IPV4 address */

    /* No IPV6 address found */
    phost = gethostbyname (pc) ;

    if (phost) {
        listptr = phost->h_addr_list ;
        ptr = (struct in_addr *) *listptr ;
        ip = ptr->s_addr ;
        return ip ;
    }

#else
    memclr (&(hints), sizeof(struct addrinfo)) ;

    if (prefer4) {
        order[0] = AF_INET ;
        order[1] = AF_INET6 ;
    } else {
        order[0] = AF_INET6 ;
        order[1] = AF_INET ;
    }

    for (i = 0 ; i <= 1 ; i++)
        switch (order[i]) {
        case AF_INET6 :
            hints.ai_family = AF_INET6 ;
            hints.ai_socktype = SOCK_STREAM ;
            ret = getaddrinfo (pc, NIL, &(hints), &(result));

            if (ret == 0) {
                *is6 = TRUE ;
                memcpy (ip6, &(result->ai_addr->sa_data[6]), sizeof(tip_v6)) ;
                freeaddrinfo(result);
                return 0 ; /* Good IPV6 address */
            }

            break ;

        case AF_INET :
            hints.ai_family = AF_INET ;
            hints.ai_socktype = SOCK_STREAM ;
            ret = getaddrinfo (pc, NIL, &(hints), &(result));

            if (ret == 0) {
                memcpy (&(ip), &(result->ai_addr->sa_data[2]), sizeof(U32)) ;
                freeaddrinfo(result);
                return ip ; /* Good IPV6 address */
            }

            break ;
        }

#endif
    return INADDR_NONE ;
}

U16 newrand (U16 *sum)
{

    *sum = (*sum * 765) + 13849 ;
    return *sum ;
}

BOOLEAN lib_file_open (pfile_owner powner, pchar path, int mode, tfile_handle *desc)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_OPEN ;
    fc.fname = path ;
    fc.options = mode ;
    powner->call_fileacc (&(fc)) ;
    *desc = fc.handle ;
    return fc.fault ;
}

BOOLEAN lib_file_close (pfile_owner powner, tfile_handle desc)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_CLOSE ;
    fc.handle = desc ;
    powner->call_fileacc (&(fc)) ;
    return fc.fault ;
}

BOOLEAN lib_file_seek (pfile_owner powner, tfile_handle desc, int offset)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_SEEK ;
    fc.options = offset ;
    fc.handle = desc ;
    powner->call_fileacc (&(fc)) ;
    return fc.fault ;
}

BOOLEAN lib_file_read (pfile_owner powner, tfile_handle desc, pointer buf, int size)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_READ ;
    fc.buffer = buf ;
    fc.options = size ;
    fc.handle = desc ;
    powner->call_fileacc (&(fc)) ;
    return fc.fault ;
}

BOOLEAN lib_file_write (pfile_owner powner, tfile_handle desc, pointer buf, int size)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_WRITE ;
    fc.buffer = buf ;
    fc.options = size ;
    fc.handle = desc ;
    powner->call_fileacc (&(fc)) ;
    return fc.fault ;
}

BOOLEAN lib_file_delete (pfile_owner powner, pchar path)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_DEL ;
    fc.fname = path ;
    powner->call_fileacc (&(fc)) ;
    return fc.fault ;
}

BOOLEAN lib_file_size (pfile_owner powner, tfile_handle desc, int *size)
{
    tfileacc_call fc ;

    memclr (&(fc), sizeof(tfileacc_call)) ;
    fc.owner = powner ;
    fc.fileacc_type = FAT_SIZE ;
    fc.handle = desc ;
    powner->call_fileacc (&(fc)) ;

    if (fc.fault)
        *size = 0 ;
    else
        *size = fc.options ;

    return fc.fault ;
}


