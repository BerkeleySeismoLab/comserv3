



/*
    Quanterra Socket Setup Definitions copyright 2018 by
    Kinemetrics, Inc.
    Pasadena, CA 91107 USA.

REV  DATE        BY   CHANGE
-----------------------------------------------------------------------
  0  2018-07-15  rdr  Adapted from tcp_forward.c.
  1  2021-12-24  rdr  Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef QSOCKETS_H
#define QSOCKETS_H
#define VER_QSOCKETS 4

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

#ifdef X86_WIN32
extern int set_server_opts (SOCKET path) ;
extern int set_nagle (SOCKET path, BOOLEAN enable) ;
#else
extern int set_server_opts (int path) ;
extern int set_nagle (int path, BOOLEAN enable) ;
#endif

#endif


