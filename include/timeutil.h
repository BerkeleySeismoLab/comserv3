/*$Id: timeutil.h,v 1.1.1.1 2004/06/15 19:08:01 isti Exp $*/
/*   Time formats include module. Only client useful routines listed
     Copyright 1994, 1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 26 Mar 94 WHO Translated from timeutil.pas
    1  9 Aug 94 WHO Add definition for localtime_string (DSN).
    2  3 Nov 97 WHO Add c++ conditionals.
    3 29 Sep 2020 DSN Updated for comserv3.
*/

#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* utility routine to add leading characters to a string */
pchar lead (short col, char c, pchar s) ;
    
/* return pointer to static string that contains ascii version of time */
pchar time_string (double jul) ;
 
/* return pointer to static string that contains ascii version of time */
pchar localtime_string (double jul) ;
  
/* Get seconds since 1970 given the year and julian day */
int32_t jconv (short yr, short jday) ;

#ifdef __cplusplus
}
#endif

#endif
