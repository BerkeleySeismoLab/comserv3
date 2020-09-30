/*
    Quanterra Socket Setup Definitions copyright 2018 by
    Certified Software Corporation
    John Day, Oregon, USA

REV  DATE        BY   CHANGE
-----------------------------------------------------------------------
  0  2018-07-15  rdr  Adapted from tcp_forward.c.
*/
#ifndef QSOCKETS_H
#define QSOCKETS_H
#define VER_QSOCKETS 0

#ifndef UTILTYPES_H
#include "utiltypes.h"
#endif

#ifdef X86_WIN32
extern integer set_server_opts (SOCKET path) ;
#else
extern integer set_server_opts (integer path) ;
#endif

#endif


