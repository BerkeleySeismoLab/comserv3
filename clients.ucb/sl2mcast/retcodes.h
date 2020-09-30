/***********************************************************

File Name :
        RetCodes.h

Programmer:
        Phil Maechling

Description:
        These are basic declarations defined here for use by 
        the TriNet software.

Creation Date:
        16 March 1998

Modification History:


Usage Notes:


**********************************************************/
#ifndef RetCodes_H
#define RetCodes_H

// Boolean values
#define TN_TRUE     1
#define TN_FALSE    0

// Generic function return codes 
#define TN_SUCCESS     0
#define TN_FAILURE     -1
#define TN_EOF         -2
#define TN_SIGNAL      -3
#define TN_NODATA      -4
#define TN_NOTVALID    -5
#define TN_TIMEOUT     -6
// These two codes have been replaced by TN_BEGIN, TN_END
#define TN_STARTGROUP  -7
#define TN_ENDGROUP    -8
#define TN_BEGIN       -7
#define TN_END         -8
//
#define TN_PARENT      -9
#define TN_CHILD       -10
// Operational failure return codes
#define TN_FAIL_WRITE  -20
#define TN_FAIL_READ   -21


// Seismic-specific return codes
#define TN_BAD_SAMPRATE  -100

#endif
