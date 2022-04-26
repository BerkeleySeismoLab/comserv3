



/*   XML Read Support definitions
     Copyright 2015-2017 by
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
    0 2015-12-18 rdr Adapted from WS330.
    1 2016-02-26 rdr Split up SEED section into STN, IIR, MAIN, SOH, and
                     ENG to match web page forms.
    2 2016-09-07 rdr Add xml_mutex for multi-threaded applications.
    3 2017-06-07 rdr Add txmlline.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef XMLSUP_H
#define XMLSUP_H
#define VER_XMLSUP 8

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

#ifndef MEMUTIL_H
#include "memutil.h"
#endif

#define MAX_ANNC 6 /* Maximum number of DP's to send announcements to */
#define MAXLINES 20000 /* number of lines for an xml file */
#define MAX_FIELD 50 /* maximum number of field entries */
#define IN6ADDRSZ 16 /* Size of IPV6 Address */
#define ESC_COUNT 5 /* For webpage */
#define ARRAY_DEPTH 2 /* maximum array nesting */

typedef I32 crc_table_type[256] ;

typedef struct
{
    char unesc ; /* unescaped character */
    string7 escpd ; /* escaped character */
} tescentry ;
typedef tescentry tescapes[ESC_COUNT] ;

enum txmltype
{X_REC, X_EREC, X_SIMP, X_ARRAY, X_EARRAY} ;
enum ttype
{T_BYTE, T_SHORT, T_WORD, T_INT, T_LWORD, T_LINT, T_SINGLE,
 T_DOUBLE, T_STRING, T_CRC
} ;
/* The main sections in the XML file */
enum txmlsect
{XS_SINF, /* System Info */
 XS_WRIT, /* Writer */
 XS_SENS, /* Sensors */
 XS_MAIN, /* Main Digitizer */
 XS_ACCL, /* Accelerometer */
 XS_DCFG, /* Dust Configuration */
 XS_TIME, /* Timing */
 XS_OPTS, /* Options */
 XS_NETW, /* Network */
 XS_ANNC, /* Announcements */
 XS_AUTO, /* Automatic Mass Recentering */
 XS_STN,  /* Station */
 XS_IIR,  /* IIR Filters */
 XS_MNAC, /* Main Digitizer & Accelerometer */
 XS_SOH,  /* State of Health */
 XS_DUST, /* Dust Channels */
 XS_ENG,  /* Engineering Data */
 XS_SIZE
} ;

typedef struct
{
    string23 arrayname ;
    int arrayidx, arraycount, arrayfirst, arraystart ;
    U16 arraysize ;
    U16 nestoffset ;
    BOOLEAN iscompact ;
} tarraystack ;

typedef struct
{
    string23 name ;
    enum txmltype x_type ;
    enum ttype stype ;
    U8 bopts ; /* size for string and character array, precision for scale, start for array */
    BOOLEAN inv_com ; /* invert for scale, compact for array. */
    U16 wopts ; /* mask for simple, count for array */
    U16 basesize ; /* for array */
    pointer svar ; /* pointer to variable */
} txmldef ;
typedef char txmlline[250] ;
typedef txmlline *pxmlline ;
typedef txmldef *pxmldef ;
typedef txmldef txmldefarray[1000] ;
typedef txmldefarray *pxmldefarray ;
typedef string23 tfieldarray[MAX_FIELD] ;
typedef tfieldarray *pfieldarray ;
typedef pchar tsrcptrs[MAXLINES] ;
typedef int terr_counts[XS_SIZE] ;
typedef string47 tsectnames[XS_SIZE] ;

extern enum txmlsect cur_sect ;
extern terr_counts error_counts ;
extern U32 crc_err_map ;
extern U32 load_map ;
extern int xmlnest ;
extern string value ;
extern string tag ;
extern BOOLEAN startflag, endflag ;
extern string cur_xml_line ;
extern int srccount ;
extern int srcidx ;
extern tsrcptrs srcptrs ;
extern pmeminst pxmlmem ;

extern const tescapes ESCAPES ;
extern const tsectnames SECTNAMES ;

/* following returned from read_xml */
extern BOOLEAN crcvalid ;
extern I32 sectcrc ;
/* end of caller variables */

/* XML Mutex routines */
extern void create_xml_mutex (void) ;
extern void xml_lock (void) ;
extern void xml_unlock (void) ;

/* Utility routines */
extern I32 gcrccalc (PU8 p, I32 len) ;
extern void gcrcupdate (PU8 p, I32 len, I32 *crc) ;
extern pchar q660_upper (pchar s) ;
extern I32 lib_round (double r) ;
extern BOOLEAN isclean (pchar f) ;

/* Start of XML read routines */
extern BOOLEAN read_xml_start (pchar section) ;
extern BOOLEAN read_next_tag (void) ;
extern int match_tag (pxmldefarray pxe, int cnt) ;
extern void proc_tag (pxmldef xe) ;
extern void read_xml (pxmldefarray pxe, int cnt, enum txmlsect sect) ;
extern pchar xml_unescape (pchar s) ;
extern pchar trim (string *s) ;
extern int match_field (pfieldarray pfld, int cnt, pchar pc) ;

#endif
