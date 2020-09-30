/*$Id: stuff.h,v 1.1.1.1 2004/06/15 19:08:01 isti Exp $*/
/*   General purpose utility routines header file
     Copyright 1994-1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 30 Mar 94 WHO Hacked from various other files.
    1 30 May 94 WHO Missing "void" on str_right added (DSN).
    2 27 Feb 95 WHO Start of conversion to run on OS9.
    3  3 Nov 97 WHO Add c++ conditionals.
    4 29 Sep 2020 DSN Updated for comserv3.
*/

#ifndef STUFF_H
#define STUFF_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <sys/types.h>

#include "dpstruc.h"

#define TMPFILE_PREFIX P_tmpdir "/"
#define TMPFILE_SUFFIX "XXXXXX"

#ifdef __cplusplus
extern "C" {
#endif

/* Return seconds (and parts of a second) since 1970 */
double dtime (void) ;

/* safe version of gets */
char *gets_buf(char *buffer, int size) ;

/* safe version of strerror */
const char *strerror_buf(int errnum, char *buf, int buflen);

/* create a unique temporary file */
FILE *tmpfile_open(char *namebuf, const char *mode);

/* Convert C string to longinteger */
int32_t str_long (pchar name);

/* Convert longinteger to C string */
pchar long_str (int32_t name) ;

/* Convert pascal string to C string */
void strpcopy (pchar outstring, pchar instring) ;

/* Convert C string to Pascal string */
void strpas (pchar outstring, pchar instring) ;

/* Set the bit in the mask pointed to by the first parameter */
void set_bit (int32_t *mask, short bit) ;

/* Clear the bit in the mask pointed to by the first parameter */
void clr_bit (int32_t *mask, short bit) ;

/* Returns TRUE if the bit in set in the mask */
boolean test_bit (int32_t mask, short bit) ;

/* remove trailing spaces and control characters from a C string */
void untrail (pchar s) ;
   
/* upshift a C string */
void upshift (pchar s) ;

/* add a directory separator slash to the end of a C string if there isn't one */
void addslash (pchar s) ;

/* Start at ptr+1 and copy characters into dest up to and including the
   terminator */
void str_right (pchar dest, pchar ptr) ;

/* Return longinteger representation of a byte, making sure it is not sign extended */
int32_t longhex (byte b) ;

/* Return integer representation of a byte, making sure it is not sign extended */
short ord (byte b) ;

/*** ADDED FUNCTIONS FOR LITTLE ENDIAN SUPPORT */
/* flip short and integer */
short flip2 (short shToFlip);
int flip4 (int iToFlip);
float flip_float (float fToFlip);
int flip4array (int32_t *in, short bytes);

#ifdef __cplusplus
}
#endif
#endif
