



/*   Platform specific system includes and definitions
     Copyright 2006-2020 by
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
    0 2015-12-17 rdr Copied from WS330
    1 2017-06-07 rdr integer should be defined as int.
    2 2017-06-10 rdr Don't conditionalize out ENDIAN_LITTLE for Win.
    3 2018-07-05 rdr tblkcheck type added.
    4 2020-12-24 rdr Add check for aarch64 for Mac ARM.
    5 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
    6 2022-02-23 jms tblkcheck,DOUBLE_HYBRID_ENDIAN removed
*/
#ifndef platform_h
#define platform_h

#include <stdio.h>
#include <math.h>

#define NIL 0

#if defined(__SVR4) && defined(__sun)
    #define solaris
    #include <sys/filio.h>
#endif


#if defined(DATALOG) || (defined(__arm__) && defined(linux))
    #define ARM_UNIX32
#endif

#if defined(ARM_UNIX32)
    #ifndef __ARMEB__
        #ifndef ENDIAN_LITTLE
            #define ENDIAN_LITTLE
        #endif
    #endif
    #include <sys/time.h>
#elif defined(linux) || defined(solaris)
    #if defined(__i386__) || defined(__x86_64__)
        #define X86_UNIX32
        #ifndef ENDIAN_LITTLE
            #define ENDIAN_LITTLE
        #endif
    #elif defined(__x86_64__)
        #define X86_UNIX64
        #ifndef ENDIAN_LITTLE
            #define ENDIAN_LITTLE
        #endif
    #else
        #define SPARC_UNIX32
    #endif
    #include <sys/time.h>
    #define OMIT_SERIAL
#endif


#if defined (X86_WIN32)

    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501
    #elif (_WIN32_WINNT < 0x0501)
        #undef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501
    #endif
    #include <winsock2.h>
    #include <Ws2tcpip.h>
    #include <windows.h>
    #include <winbase.h>

    #define BOOLEAN unsigned __int8 /* 8 bit unsigned, 0 or non-zero */
    #define I8 signed __int8 /* 8 bit signed */
    #define U8 unsigned __int8 /* 8 bit unsigned */
    #define I16 __int16 /* 16 bit signed */
    #define U16 unsigned __int16 /* 16 bit unsigned */
    #define I32 __int32 /* 32 bit signed */
    #define U32 unsigned __int32 /* 32 bit unsigned */
    #define PNTRINT size_t /* unsigned integer, same size as pointer */
    typedef HANDLE tfile_handle ;
    typedef struct _stat tfile_state ;
    #define ENDIAN_LITTLE
    #define INVALID_FILE_HANDLE INVALID_HANDLE_VALUE
    #define INVALID_IO_HANDLE INVALID_HANDLE_VALUE

    /* Eclipse with MinGW64 has incorrect values for these, replace */
    #if (EWOULDBLOCK != 10035)
        #define EWOULDBLOCK WSAEWOULDBLOCK
        #define ECONNRESET WSAECONNRESET
        #define ENOBUFS WSAENOBUFS
        #define EINPROGRESS WSAEINPROGRESS
        #define ECONNABORTED WSAECONNABORTED
    #endif
    #ifndef SHUT_RDWR
        #define SHUT_RDWR 2
    #endif

#elif defined(X86_UNIX32) || defined(SPARC_UNIX32) || defined(X86_UNIX64) || defined(ARM_UNIX32)

    #include <sys/types.h>
    #include <pthread.h>
    #include <errno.h>
    #include <string.h>
    #include <stdlib.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <sys/ioctl.h>
    #include <sys/stat.h>
    #include <netdb.h>
    #include <signal.h>

    #include <fcntl.h>
    #include <unistd.h>     /* required for ARM Linux. maybe for others as well. */
    #ifndef OMIT_SERIAL
        #include <sys/termios.h>
    #endif

    #define BOOLEAN uint8_t /* 8 bit unsigned, 0 or non-zero */
    #define I8 int8_t /* 8 bit signed */
    #define U8 uint8_t /* 8 bit unsigned */
    #define I16 int16_t /* 16 bit signed */
    #define U16 uint16_t /* 16 bit unsigned */
    #define I32 int32_t /* 32 bit signed */
    #define U32 uint32_t /* 32 bit unsigned */
    #if defined(X86_UNIX64) || defined(__x86_64__)
        #define PNTRINT uint64_t /* 64 bit unsigned, same size as pointer */
    #else
        #define PNTRINT uint32_t /* 32 bit unsigned, same size as pointer */
    #endif
    typedef int tfile_handle ;
    typedef struct stat tfile_state ;
    #define INVALID_FILE_HANDLE -1
    #define INVALID_IO_HANDLE -1
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1

    #define FALSE 0
    #define TRUE 1

#elif defined(__APPLE__)

    #include <sys/types.h>
    #include <pthread.h>
    #include <errno.h>
    #include <string.h>
    #include <stdlib.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <netdb.h>
    #include <signal.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/termios.h>

    #define BOOLEAN uint8_t /* 8 bit unsigned, 0 or non-zero */
    #define I8 int8_t /* 8 bit signed */
    #define U8 uint8_t /* 8 bit unsigned */
    #define I16 int16_t /* 16 bit signed */
    #define U16 uint16_t /* 16 bit unsigned */
    #define I32 int32_t /* 32 bit signed */
    #define U32 uint32_t /* 32 bit unsigned */
    #if defined(__x86_64__) || defined(__aarch64__)
        #define PNTRINT uint64_t /* 64 bit unsigned, same size as pointer */
        #define ALIGN_64 1 /* Use to adjust structures for 64 bit apple */
    #else
        #define PNTRINT uint32_t /* 32 bit unsigned, same size as pointer */
    #endif
    typedef int tfile_handle ;
    typedef struct stat tfile_state ;
    #define INVALID_FILE_HANDLE -1
    #define INVALID_IO_HANDLE -1
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define FALSE 0
    #define TRUE 1
    #ifdef __LITTLE_ENDIAN__
        #ifndef ENDIAN_LITTLE
            #define ENDIAN_LITTLE
        #endif
    #endif

#endif

/* at least on solaris, this is undefined */
#ifndef INADDR_NONE
    #define INADDR_NONE (U32) -1
#endif

#endif
