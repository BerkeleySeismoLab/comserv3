/*   Lib660 Message Definitions

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
    Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/
/*  Libmsmcast Message Definitions

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation                                                           Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#ifndef LIBMSGS_H
#define libmsgs_h
#define VER_LIBMSGS 0

#include "libtypes.h"
#include "libstrucs.h"

/* verbosity bitmap flags */
#define VERB_SDUMP 1 /* set in cur_verbosity to enable status dump messages */
#define VERB_REGMSG 4 /* Registration messages */
#define VERB_LOGEXTRA 8 /* stuff like filter delays */
#define VERB_AUXMSG 16 /* webserver & netserver messages to seed message log */
#define VERB_PACKET 32 /* packet level debug */

/* message codes */
#define LIBMSG_GENDBG 0
#define LIBMSG_PKTIN 1
#define LIBMSG_PKTOUT 2

#define LIBMSG_USER 100
#define LIBMSG_DECNOTFOUND 101
#define LIBMSG_FILTDLY 102
#define LIBMSG_SEQBEG 103
#define LIBMSG_SEQOVER 104
#define LIBMSG_SEQRESUME 105
#define LIBMSG_CONTBOOT 106
#define LIBMSG_CONTFND 107
#define LIBMSG_BUFSHUT 108
#define LIBMSG_CONNSHUT 109
#define LIBMSG_NOIP 110
#define LIBMSG_ENDTIME 111
#define LIBMSG_RESTCONT 112
#define LIBMSG_DISCARD 113
#define LIBMSG_CSAVE 114
#define LIBMSG_DETECT 115
#define LIBMSG_NETSTN 116
#define LIBMSG_TOTAL 117
#define LIBMSG_PRILIMIT 118

#define LIBMSG_CREATED 200
#define LIBMSG_REGISTERED 201
#define LIBMSG_DEALLOC 202
#define LIBMSG_DEREG 203
#define LIBMSG_XMLREAD 204
#define LIBMSG_POCRECV 205
#define LIBMSG_WRCONT 206
#define LIBMSG_SOCKETOPEN 207
#define LIBMSG_CONN 208

#define LIBMSG_GPSSTATUS 300
#define LIBMSG_DIGPHASE 301
#define LIBMSG_LEAPDET 302
#define LIBMSG_PHASERANGE 303
#define LIBMSG_TIMEJMP 304

#define LIBMSG_XMLERR 400
#define LIBMSG_SNR 401
#define LIBMSG_INVREG 402
#define LIBMSG_CONTIN 403
#define LIBMSG_DATATO 404
#define LIBMSG_CONCRC 405
#define LIBMSG_CONTNR 406
#define LIBMSG_STATTO 407
#define LIBMSG_CONPURGE 408
#define LIBMSG_BADTIME 409

#define LIBMSG_SOCKETERR 500
#define LIBMSG_ADDRERR 501
#define LIBMSG_SEQGAP 502
#define LIBMSG_LBFAIL 503
#define LIBMSG_NILRING 504
#define LIBMSG_UNCOMP 505
#define LIBMSG_CONTERR 506
#define LIBMSG_TIMEDISC 507
#define LIBMSG_RECOMP 508
#define LIBMSG_PKTERR 509
#define LIBMSG_NOTREADY 510

#define LIBMSG_SYSINFO 600

#define AUXMSG_SOCKETOPEN 700
#define AUXMSG_SOCKETERR 701
#define AUXMSG_BINDERR 702
#define AUXMSG_LISTENERR 703
#define AUXMSG_DISCON 704
#define AUXMSG_ACCERR 705
#define AUXMSG_CONN 706
#define AUXMSG_NOBLOCKS 707
#define AUXMSG_SENT 708
#define AUXMSG_RECVTO 709
#define AUXMSG_WEBLINK 710
#define AUXMSG_RECV 711

#define HOSTMSG_ALL 800

#ifdef CONSTMSG
extern void libmsgadd (pmsmcast msmcast, word msgcode, const pchar msgsuf) ;
extern void libdatamsg (pmsmcast msmcast, word msgcode, const pchar msgsuf) ;
extern void msgadd (pmsmcast msmcast, word msgcode, longword dt, const pchar msgsuf, boolean client) ;
#else
extern void libmsgadd (pmsmcast msmcast, word msgcode, pchar msgsuf) ;
extern void libdatamsg (pmsmcast msmcast, word msgcode, pchar msgsuf) ;
extern void msgadd (pmsmcast msmcast, word msgcode, longword dt, pchar msgsuf, boolean client) ;
#endif

extern void dump_msgqueue (pmsmcast msmcast) ;
extern pchar lib_get_msg (word code, pchar result) ;
extern pchar lib_get_errstr (enum tliberr err, pchar result) ;
extern pchar lib_get_statestr (enum tlibstate state, pchar result) ;
extern pchar showdot (longword num, pchar result) ;
extern pchar inet_ntop6 (tip_v6 *src) ;
extern pchar command_name (byte cmd, pchar result) ;

#endif
