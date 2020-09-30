/* Comserv type definitions */

#ifndef CSTYPES_H
#define CSTYPES_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include <stdint.h>

/* Originally from dpstruc.h */
/* Boolean constants */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1 /* Must be 1 for compatibility with Pascal */
#endif

#ifndef boolean
#define boolean unsigned char
#endif
#ifndef byte
#define byte unsigned char
#endif

typedef char *pchar ;
typedef void *pvoid ;

/* instead of pascal 0..31 sets, bitmaps are used */
typedef uint32_t psuedo_set ;

/* likewise, channel maps are represented by bitmaps */
typedef uint32_t chan_map ;

/* types defined as "stringxx" are pascal format strings. The first byte is
   the dynamic length, followed by up to xx characters */
typedef char string15[16] ;
typedef char string23[24] ;
typedef char string59[60] ;
typedef char string79[80] ;

/* Common fields within SEED headers and other structures */
typedef char seed_name_type[3] ;
typedef char seed_net_type[2] ;
typedef char location_type[2] ;

/* 2 byte and 4 byte unions */
typedef union
{
    unsigned char b[2] ;
    signed char sb[2] ;
    int16_t s ;
} compword ;
typedef union
{
    unsigned char b[4] ;
    signed char sb[4] ;
    int16_t s[2] ;
    int32_t l ;
    float f ;
} complong ;

#endif
