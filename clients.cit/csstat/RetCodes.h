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
const int TN_TRUE    = 1;
const int TN_FALSE   = 0;

// Generic function return codes 
const int TN_SUCCESS    = 0;
const int TN_FAILURE    = -1;
const int TN_EOF        = -2;
const int TN_SIGNAL     = -3;
const int TN_NODATA     = -4;
const int TN_NOTVALID   = -5;
const int TN_TIMEOUT    = -6;
// These two codes have been replaced by TN_BEGIN, TN_END
const int TN_STARTGROUP = -7;
const int TN_ENDGROUP   = -8;
const int TN_BEGIN      = -7;
const int TN_END        = -8;
//
const int TN_PARENT     = -9;
const int TN_CHILD      = -10;
// Operational failure return codes
const int TN_FAIL_WRITE = -20;
const int TN_FAIL_READ  = -21;


// Seismic-specific return codes
const int TN_BAD_SAMPRATE = -100;

#endif
