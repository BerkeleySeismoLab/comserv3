/*   Lib660 Support routines
     Copyright 2017 Certified Software Corporation

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

pchar zpad (pchar s, integer lth)
begin
  integer len, diff ;

  len = (integer)strlen(s) ;
  diff = lth - len ;
  if (diff > 0)
    then
      begin
        memmove (addr(s[diff]), addr(s[0]), len + 1) ; /* shift existing string right */
        memset (addr(s[0]), '0', diff) ; /* add ascii zeroes at front */
      end
  return s ;
end

word day_julian (word yr, word wmth, word day)
begin
  word mth, jday ;

  jday = 0 ;
  mth = 1 ;
  while (mth < wmth)
    begin
      if (((yr mod 4) == 0) land (mth == 2))
        then
          jday = jday + 29 ;
        else
          jday = jday + days_mth[mth] ;
      inc(mth) ;
    end
  return jday + day ;
end

longint lib660_julian (tsystemtime *greg)
begin
  word year ;
  longint leap, days ;

  year = greg->wyear - 2016 ;
  leap = (year + 3) shr 2 ; /* leap years so far */
  days = (longint) year * 365 + leap ; /* number of years passed, plus leap days */
  days = days + day_julian (year, greg->wmonth, greg->wday) - 1 ;
  return days * 86400 + (longint) greg->whour * 3600 +
            (longint) greg->wminute * 60 + (longint) greg->wsecond ;
end


#ifdef X86_WIN32
double now (void)
begin
  SYSTEMTIME sgreg ;
  tsystemtime nowgreg ;
  double r ;

  GetSystemTime (addr(sgreg)) ;
  memcpy (addr(nowgreg), addr(sgreg), sizeof(tsystemtime)) ; /* copy to internal version */
  r = lib660_julian (addr(nowgreg)) ; /* get whole seconds since 2000 */
  r = r + nowgreg.wmilliseconds / 1000.0 ;
  return r ;
end

#else

double now (void)
begin
#define DIFF2016_1970 ((35 * 365) + (11 * 366)) /* Difference between 1970 and 2016 references */
  struct timeval tp;
  struct timezone tzp;
  double r ;

  gettimeofday (addr(tp), addr(tzp)) ;
  r = ((double) tp.tv_sec + ((double) tp.tv_usec / 1000000.0)) ;
  return r - DIFF2016_1970 * 86400.0 ;
end
#endif

longword lib_uround (double r)
begin
  longword result ;

  if (r >= 0.0)
    then
      result = r + 0.5 ;
    else
      result = r - 0.5 ;
  return result ;
end

void day_gregorian (word yr, word jday, word *mth, word *day)
begin
  word dim ;

  *mth = 1 ;
  repeat
    dim = days_mth[*mth] ;
    if (((yr mod 4) == 0) land (*mth == 2))
      then
        dim = 29 ;
    if (jday <= dim)
      then
        break ;
    jday = jday - dim ;
    inc(*mth) ;
  until (jday > 400)) ; /* just in case */
  *day = jday ;
end

void lib660_gregorian (longint jul, tsystemtime *greg)
begin
  longint quads, days, subday, yeartemp ;

  days = jul div 86400 ;
  subday = jul - (days * 86400) ;
  greg->whour = subday div 3600 ;
  subday = subday - ((longint) greg->whour * 3600) ;
  greg->wminute = subday div 60 ;
  greg->wsecond = subday - (greg->wminute * 60) ;
  yeartemp = 2016 ;
  quads = days div 1461 ; /* 0-3 groups */
  incn(yeartemp, quads shl 2) ;
  decn(days, quads * 1461) ;
  if (days >= 366)
    then
      begin
        inc(yeartemp) ;
        days = days - 366 ; /* remove leap year part of group */
        while (days >= 365)
          begin
            inc(yeartemp) ;
            days = days - 365 ;
          end
      end
  greg->wyear = yeartemp ;
  day_gregorian (greg->wyear, days + 1, addr(greg->wmonth), addr(greg->wday)) ;
end

pchar jul_string (longint jul, pchar result)
begin
  tsystemtime g ;
  string7 s1 ;

  if (jul < 0)
    then
      strcpy(result, "Invalid Time       ") ;
    else
      begin
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
      end
  return result ;
end

pchar packet_time (longint jul, pchar result)
begin
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
end ;

pchar t64tostring (t64 *val, pchar result)
begin
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
end

boolean stringtot64 (pchar s, t64 *value)
begin
  integer good, len ;
  string15 sh, sl ;
  string s1 ;
  unsigned int v0, v1 ;

  v0 = 0 ;
  v1 = 0 ;
  strcpy (s1, s) ; /* local copy for indexing */
  len = (integer)strlen(s1) ;
  if (len > 8)
    then
      begin
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
          then
            return FALSE ;
      end
  else if (s[0])
    then
      begin
#ifdef ENDIAN_LITTLE
        good = sscanf(s, "%x", addr(v0)) ;
#else
        good = sscanf(s, "%x", addr(v1)) ;
#endif
        if (good != 1)
          then
            return FALSE ;
      end
  (*value)[0] = v0 ;
  (*value)[1] = v1 ;
  return TRUE ;
end

/* If valid IPV6 address sets is6 TRUE and returns in ip6.
   Else if valid IPV4 address sets is6 FALSE and returns IP address.
   Else sets is6 FALSE and returns IPADDR_NONE */
longword get_ip_address (pchar pc, tip_v6 *ip6, boolean *is6, boolean prefer4)
begin
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
    then
      return ip ; /* Good IPV4 address */
  /* No IPV6 address found */
  phost = gethostbyname (pc) ;
  if (phost)
    then
      begin
        listptr = phost->h_addr_list ;
        ptr = (struct in_addr *) *listptr ;
        ip = ptr->s_addr ;
        return ip ;
      end
#else
  memclr (addr(hints), sizeof(struct addrinfo)) ;
  if (prefer4)
    then
      begin
        order[0] = AF_INET ;
        order[1] = AF_INET6 ;
      end
    else
      begin
        order[0] = AF_INET6 ;
        order[1] = AF_INET ;
      end
  for (i = 0 ; i <= 1 ; i++)
    switch (order[i]) begin
      case AF_INET6 :
        hints.ai_family = AF_INET6 ;
        hints.ai_socktype = SOCK_STREAM ;
        ret = getaddrinfo (pc, NIL, addr(hints), addr(result));
        if (ret == 0)
          then
            begin
              *is6 = TRUE ;
              memcpy (ip6, addr(result->ai_addr->sa_data[6]), sizeof(tip_v6)) ;
              freeaddrinfo(result);
              return 0 ; /* Good IPV6 address */
            end
        break ;
      case AF_INET :
        hints.ai_family = AF_INET ;
        hints.ai_socktype = SOCK_STREAM ;
        ret = getaddrinfo (pc, NIL, addr(hints), addr(result));
        if (ret == 0)
          then
            begin
              memcpy (addr(ip), addr(result->ai_addr->sa_data[2]), sizeof(longword)) ;
              freeaddrinfo(result);
              return ip ; /* Good IPV6 address */
            end
        break ;
    end
#endif
  return INADDR_NONE ;
end

word newrand (word *sum)
begin

  *sum = (*sum * 765) + 13849 ;
  return *sum ;
end

boolean lib_file_open (pfile_owner powner, pchar path, integer mode, tfile_handle *desc)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_OPEN ;
  fc.fname = path ;
  fc.options = mode ;
  powner->call_fileacc (addr(fc)) ;
  *desc = fc.handle ;
  return fc.fault ;
end

boolean lib_file_close (pfile_owner powner, tfile_handle desc)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_CLOSE ;
  fc.handle = desc ;
  powner->call_fileacc (addr(fc)) ;
  return fc.fault ;
end

boolean lib_file_seek (pfile_owner powner, tfile_handle desc, integer offset)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_SEEK ;
  fc.options = offset ;
  fc.handle = desc ;
  powner->call_fileacc (addr(fc)) ;
  return fc.fault ;
end

boolean lib_file_read (pfile_owner powner, tfile_handle desc, pointer buf, integer size)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_READ ;
  fc.buffer = buf ;
  fc.options = size ;
  fc.handle = desc ;
  powner->call_fileacc (addr(fc)) ;
  return fc.fault ;
end

boolean lib_file_write (pfile_owner powner, tfile_handle desc, pointer buf, integer size)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_WRITE ;
  fc.buffer = buf ;
  fc.options = size ;
  fc.handle = desc ;
  powner->call_fileacc (addr(fc)) ;
  return fc.fault ;
end

boolean lib_file_delete (pfile_owner powner, pchar path)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_DEL ;
  fc.fname = path ;
  powner->call_fileacc (addr(fc)) ;
  return fc.fault ;
end

boolean lib_file_size (pfile_owner powner, tfile_handle desc, integer *size)
begin
  tfileacc_call fc ;

  memclr (addr(fc), sizeof(tfileacc_call)) ;
  fc.owner = powner ;
  fc.fileacc_type = FAT_SIZE ;
  fc.handle = desc ;
  powner->call_fileacc (addr(fc)) ;
  if (fc.fault)
    then
      *size = 0 ;
    else
      *size = fc.options ;
  return fc.fault ;
end


