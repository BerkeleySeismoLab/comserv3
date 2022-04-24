/*  Libmsmcast Message Handlers

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

#include "libsampglob.h"
#include "libmsgs.h"
#include "libsupport.h"
#include "readpackets.h"

pchar lib_get_msg (word code, pchar result)
{
  string95 s ;

  sprintf(s, "Unknown Message Number %d ", code) ; /* default */
  switch (code / 100) {
    case 0 : /* debugging */
      switch (code) {
        case LIBMSG_GENDBG : strcpy(s, "") ; /* generic, all info in suffix */ break ;
        case LIBMSG_PKTIN : strcpy(s, "Recv") ; break ;
        case LIBMSG_PKTOUT : strcpy(s, "Sent") ; break ;
      }
      break ;
    case 1 : /* informational */
      switch (code) {
        case LIBMSG_USER : strcpy(s, "Msg From ") ; break ;
        case LIBMSG_DECNOTFOUND : strcpy(s, "Decimation filter not found for ") ; break ;
        case LIBMSG_FILTDLY : strcpy(s, "Filters & delay ") ; break ;
        case LIBMSG_SEQBEG : strcpy (s, "Data begins at ") ; break ;
        case LIBMSG_SEQOVER : strcpy(s, "Data resumes overlapping: ") ; break ;
        case LIBMSG_SEQRESUME : strcpy(s, "Data resumes: ") ; break ;
        case LIBMSG_CONTBOOT : strcpy(s, "Data continuity lost due to ") ; break ;
        case LIBMSG_CONTFND : strcpy(s, "Continuity found: ") ; break ;
        case LIBMSG_BUFSHUT : strcpy(s, "De-Registration due to reaching disconnect latency time") ; break ;
        case LIBMSG_CONNSHUT : strcpy(s, "De-Registration due to reaching maximum connection time") ; break ;
        case LIBMSG_NOIP : strcpy(s, "No initial IP Address specified, waiting for POC") ; break ;
        case LIBMSG_RESTCONT : strcpy(s, "Restoring continuity") ; break ;
        case LIBMSG_DISCARD : strcpy(s, "Discarded 1: ") ; break ;
        case LIBMSG_CSAVE : strcpy(s, "Continuity saved: ") ; break ;
        case LIBMSG_DETECT : strcpy(s, "") ; /* all info in suffix */ break ;
        case LIBMSG_NETSTN : strcpy(s, "Station: ") ; break ;
        case LIBMSG_TOTAL : strcpy(s, "") ; /* all info in suffix */ break ;
        case LIBMSG_PRILIMIT : strcpy(s, "Max Sample Rate for priority: ") ; break ;
      }
      break ;
    case 2 : /* success code */
      switch (code) {
        case LIBMSG_CREATED : strcpy(s, "Station Thread Created") ; break ;
        case LIBMSG_REGISTERED : strcpy(s, "Registered with multicast socket") ; break ;
        case LIBMSG_DEALLOC : strcpy(s, "De-Allocating Data Structures") ; break ;
        case LIBMSG_DEREG : strcpy(s, "De-Registered with multicast socket") ; break ;
        case LIBMSG_XMLREAD : strcpy(s, "XML loaded, size=") ; break ;
        case LIBMSG_POCRECV : strcpy(s, "POC Received: ") ; break ;
        case LIBMSG_WRCONT : strcpy(s, "Writing Continuity File: ") ; break ;
        case LIBMSG_SOCKETOPEN : strcpy(s, "Socket Opened ") ; break ;
        case LIBMSG_CONN : strcpy(s, "Connected to multicast socket") ; break ;
      }
      break ;
    case 3 : /* converted Q660 blockettes */
      switch (code) {
        case LIBMSG_GPSSTATUS : strcpy(s, "New GPS Status=") ; break ;
        case LIBMSG_DIGPHASE : strcpy(s, "New Digitizer Phase=") ; break ;
        case LIBMSG_LEAPDET : strcpy(s, "Leap Second Detected") ; break ;
        case LIBMSG_PHASERANGE : strcpy(s, "Phase Out of Range of ") ; break ;
        case LIBMSG_TIMEJMP : strcpy(s, "Time Jump of ") ; break ;
      }
      break ;
    case 4 : /* client faults */
      switch (code) {
        case LIBMSG_XMLERR : strcpy(s, "Error reading XML, Will retry registration in ") ; break ;
        case LIBMSG_SNR : strcpy(s, "Not Registered, Will retry registration in ") ; break ;
        case LIBMSG_INVREG : strcpy(s, "Invalid Registration Request, Will retry registration in ") ; break ;
        case LIBMSG_CONTIN : strcpy(s, "Continuity Error: ") ; break ;
        case LIBMSG_DATATO : strcpy(s, "Data Timeout, Will retry registratin in ") ; break ;
        case LIBMSG_CONCRC : strcpy(s, "Continuity CRC Error, Ignoring rest of file: ") ; break ;
        case LIBMSG_CONTNR : strcpy(s, "Continuity not restored") ; break ;
        case LIBMSG_STATTO : strcpy(s, "Status Timeout, Will retry registratin in ") ; break ;
        case LIBMSG_CONPURGE : strcpy(s, "Continuity was expired, Ignoring rest of file: ") ; break ;
        case LIBMSG_BADTIME : strcpy(s, "Requested Data Not Found") ; break ;
      }
      break ;
    case 5 : /* server faults */
      switch (code) {
        case LIBMSG_SOCKETERR : strcpy(s, "Open Socket error: ") ; break ;
        case LIBMSG_ADDRERR : strcpy (s, "Invalid Nework Address: ") ; break ;
        case LIBMSG_SEQGAP : strcpy(s, "Sequence Gap ") ; break ;
        case LIBMSG_LBFAIL : strcpy(s, "Last block failed on ") ; break ;
        case LIBMSG_NILRING : strcpy(s, "NIL Ring on ") ; break ;
        case LIBMSG_UNCOMP : strcpy(s, "Uncompressable Data on ") ; break ;
        case LIBMSG_CONTERR : strcpy(s, "Continuity Error on ") ; break ;
        case LIBMSG_TIMEDISC : strcpy(s, "time label discontinuity: ") ; break ;
        case LIBMSG_RECOMP : strcpy(s, "Re-compression error on ") ; break ;
        case LIBMSG_PKTERR : strcpy(s, "Network error: ") ; break ;
        case LIBMSG_NOTREADY : strcpy(s, "BE660 Not Ready: ") ; break ;
      }
      break ;
    case 6 :
      strcpy(s, "") ; /* contained in suffix */
      break ;
    case 7 : /* aux server messages */
      switch (code) {
        case AUXMSG_SOCKETOPEN : strcpy(s, "Socket Opened ") ; break ;
        case AUXMSG_SOCKETERR : strcpy(s, "Open Socket error: ") ; break ;
        case AUXMSG_BINDERR : strcpy(s, "Bind error: ") ; break ;
        case AUXMSG_LISTENERR : strcpy(s, "Listen error: ") ; break ;
        case AUXMSG_DISCON : strcpy(s, "Client Disconnected from ") ; break ;
        case AUXMSG_ACCERR : strcpy(s, "Accept error: ") ; break ;
        case AUXMSG_CONN : strcpy(s, "Client Connected ") ; break ;
        case AUXMSG_NOBLOCKS : strcpy(s, "No Blocks Available ") ; break ;
        case AUXMSG_SENT : strcpy(s, "Sent: ") ; break ;
        case AUXMSG_RECVTO : strcpy(s, "Receive Timeout ") ; break ;
        case AUXMSG_WEBLINK : strcpy(s, "") ; /* contained in suffix */ break ;
        case AUXMSG_RECV : strcpy(s, "Recv: ") ; break ;
      }
      break ;
    case 8 :
      strcpy(s, "") ; /* contained in suffix */
      break ;
  }
  strcpy (result, s) ;
  return result ;
}

pchar lib_get_errstr (enum tliberr err, pchar result)
{
  string63 s ;

  strcpy(s, "Unknown Error Code") ;
  switch (err) {
    case LIBERR_NOERR : strcpy(s, "No error") ; break ;
    case LIBERR_NOTR : strcpy(s, "You are not registered") ; break ;
    case LIBERR_INVREG : strcpy(s, "Invalid Registration Request") ; break ;
    case LIBERR_ENDTIME : strcpy(s, "Reached End Time") ; break ;
    case LIBERR_XMLERR : strcpy(s, "Error reading XML") ; break ;
    case LIBERR_THREADERR : strcpy(s, "Could not create thread") ; break ;
    case LIBERR_REGTO : strcpy(s, "Registration Timeout, ") ; break ;
    case LIBERR_STATTO : strcpy(s, "Status Timeout, ") ; break ;
    case LIBERR_DATATO : strcpy(s, "Data Timeout, ") ; break ;
    case LIBERR_NOSTAT : strcpy(s, "Your requested status is not yet available") ; break ;
    case LIBERR_INVSTAT : strcpy(s, "Your requested status in not a valid selection") ; break ;
    case LIBERR_CFGWAIT : strcpy(s, "Your requested configuration is not yet available") ; break ;
    case LIBERR_BUFSHUT : strcpy(s, "Shutdown due to reaching disconnect latency time") ; break ;
    case LIBERR_CONNSHUT : strcpy(s, "Shutdown due to reaching maximum connection time") ; break ;
    case LIBERR_CLOSED : strcpy(s, "Closed by host") ; break ;
    case LIBERR_NETFAIL : strcpy(s, "Networking Failure") ; break ;
    case LIBERR_INVCTX : strcpy(s, "Invalid Context") ; break ;
  }
  strcpy (result, s) ;
  return result ;
}

pchar lib_get_statestr (enum tlibstate state, pchar result)
{
  string63 s ;

  strcpy(s, "Unknown State") ;
  switch (state) {
    case LIBSTATE_IDLE : strcpy(s, "Not connected to multicast socket") ; break ;
    case LIBSTATE_TERM : strcpy(s, "Terminated") ; break ;
    case LIBSTATE_CONN : strcpy(s, "TCP Connect Wait") ; break ;
    case LIBSTATE_REQ : strcpy(s, "Requesting Registration") ; break ;
    case LIBSTATE_RESP : strcpy(s, "Registration Response") ; break ;
    case LIBSTATE_XML : strcpy(s, "Reading Configuration") ; break ;
    case LIBSTATE_CFG : strcpy(s, "Configuring") ; break ;
    case LIBSTATE_RUNWAIT : strcpy(s, "Ready to Run") ; break ;
    case LIBSTATE_RUN : strcpy(s, "Running") ; break ;
    case LIBSTATE_DEALLOC : strcpy(s, "De-allocating structures") ; break ;
    case LIBSTATE_WAIT : strcpy(s, "Waiting for a new registration") ; break ;
  }
  strcpy (result, s) ;
  return result ;
}

#ifdef CONSTMSG
void msgadd (pmsmcast msmcast, word msgcode, longword dt, const pchar msgsuf, boolean client)
#else
void msgadd (pmsmcast msmcast, word msgcode, longword dt, pchar msgsuf, boolean client)
#endif
{
  string s, s1, s2 ;

  msglock (msmcast) ;
  msmcast->msg_call.context = msmcast ;
  msmcast->msg_call.timestamp = lib_round(now()) ;
  msmcast->msg_call.datatime = dt ;
  msmcast->msg_call.code = msgcode ;
  if (msgcode == 0)
      s[0] = 0 ;
  strcpy(msmcast->msg_call.suffix, msgsuf) ;
  if (dt)
      sprintf(s1, "[%s] ", jul_string(dt, s2)) ;
    else
      strcpy(s1, " ") ;
  sprintf(s, "{%d}%s%s%s", msgcode, s1, lib_get_msg (msgcode, s2), msgsuf) ;
  if (((msgcode / 100) != 7) || (msmcast->cur_verbosity & VERB_AUXMSG))
      {
        inc(msmcast->msg_count) ;
        msmcast->msg_call.msgcount = msmcast->msg_count ;
        if (! msmcast->nested_log)
            {
		if ((client) || (msmcast->libstate == LIBSTATE_TERM) || /*:: ((msmcast->network)[0] == 0) || ::*/
                  (msmcast->msg_lcq == NIL) /*:: || (msmcast->msg_lcq->com->ring == NIL)::*/ )
                  {
                    if (msmcast->msgq_in->link != msmcast->msgq_out)
                        {
                          strcpy(msmcast->msgq_in->msg, s) ;
                          msmcast->msgq_in = msmcast->msgq_in->link ;
                        }
                  }
                else
                  {
                    while (msmcast->msgq_out != msmcast->msgq_in)
                      {
                        msmcast->msgq_out = msmcast->msgq_out->link ;
                      }
                  }
            }
        if (msmcast->par_create.call_messages)
            msmcast->par_create.call_messages (addr(msmcast->msg_call)) ;
      }
  msgunlock (msmcast) ;
}

#ifdef CONSTMSG
void libmsgadd (pmsmcast msmcast, word msgcode, const pchar msgsuf)
#else
void libmsgadd (pmsmcast msmcast, word msgcode, pchar msgsuf)
#endif
{

  msgadd (msmcast, msgcode, 0, msgsuf, FALSE) ;
}

#ifdef CONSTMSG
void libdatamsg (pmsmcast msmcast, word msgcode, const pchar msgsuf)
#else
void libdatamsg (pmsmcast msmcast, word msgcode, pchar msgsuf)
#endif
{
  longword dt ;

  dt = lib_round(msmcast->data_timetag) ;
  msgadd (msmcast, msgcode, dt, msgsuf, FALSE) ;
}

/* Cleaned up version derived from BIND 4.9.4 release. Returns
   address of string */
pchar inet_ntop6 (tip_v6 *src)
{
#define IN6WCNT 8 /* Number of words in an IPV6 address (256/16) */
  typedef struct {
    integer base ;
    integer len ;
  } tmatch ;
  tmatch best ;
  tmatch cur ;
  static string47 tmp ; /* returned result */
  pchar tp ;
  word words[IN6WCNT] ;
  integer i ;
  pword pw ;

  /* Preprocess:
   * Copy the input (bytewise) array into a wordwise array.
   * Find the longest run of 0x00's in src[] for :: shorthanding. */
  memclr (addr(words), sizeof(tip_v6)) ;
  pw = (pvoid)src ; /* Get a word pointer */
  for (i = 0 ; i < IN6WCNT ; i++)
#ifdef ENDIAN_LITTLE
    words[i] = ntohs(*pw++) ;
#else
    words[i] = *pw++ ;
#endif
  best.base = -1 ;
  cur.base = -1 ;
  best.len = 0 ;
  cur.len = 0 ;
  for (i = 0 ; i < IN6WCNT ; i++)
    {
      if (words[i] == 0)
          {
            if (cur.base == -1)
                {
                  cur.base = i ;
                  cur.len = 1 ;
                }
              else
                cur.len++ ;
          }
        else
          {
            if (cur.base != -1)
                {
                  if ((best.base == -1) || (cur.len > best.len))
                      {
                        best = cur ;
                        cur.base = -1 ;
                      }
                }
          }
    }
  if (cur.base != -1)
      {
        if ((best.base == -1) || (cur.len > best.len))
            best = cur ;
      }
  if ((best.base != -1) && (best.len < 2))
      best.base = -1 ;
  /* Format the result. */
  tp = (pchar)addr(tmp) ;
  for (i = 0 ; i < IN6WCNT ; i++)
    {
      /* Are we inside the best run of 0x00's? */
      if ((best.base != -1) && (i >= best.base) &&
          (i < (best.base + best.len)))
          {
            if (i == best.base)
                *tp++ = ':' ;
            continue ;
          }
      /* Are we following an initial run of 0x00s or any real hex? */
      if (i)
          *tp++ = ':' ;
      sprintf(tp, "%x", words[i]) ;
      incn (tp, strlen(tp)) ;
    }
  /* Was it a trailing run of 0x00's? */
  if ((best.base != -1) && ((best.base + best.len) == IN6WCNT))
      *tp++ = ':' ;
  *tp++ = '\0' ;
  return tmp ;
}

pchar showdot (longword num, pchar result)
{

  sprintf(result, "%d.%d.%d.%d", (integer)((num shr 24) & 255), (integer)((num shr 16) & 255),
          (integer)((num shr 8) & 255), (integer)(num & 255)) ;
  return result ;
}

pchar command_name (byte cmd, pchar result)
{
  string95 s ;
  integer h ;

  strcpy (s, "Unknown") ;
  switch (cmd) {
    case DT_DATA : strcpy(s, "Normal Data Record") ; break ;
    case DT_LLDATA : strcpy(s, "Low Latency Data") ; break ;
    case DT_STATUS : strcpy(s, "Status") ; break ;
    default :
      {
        h = cmd ;
        sprintf(s, "$%x", h) ;
      }
  }
  strcpy (result, s) ;
  return result ;
}

pchar lib_log_types (enum tlogfld logfld, pchar result)
{
const tlogfields ELOGFIELDS = {"Log Messages", "XML Configuration", "Timing Blockettes",
    "Data Gaps", "Re-Boots", "Received Bps", "Sent Bps", "Communications Attempts",
    "Communications Successes", "Packets Received", "Communications Efficiency",
    "POC's Received", "IP Address Changes", "Comm. Duty Cycle", "Throughput",
    "Missing Data", "Sequence Errors", "Checksum Errors", "Data Latency",
    "Low Latency Data", "Status Latency"} ;

  if (logfld < LOGF_SIZE)
      strcpy (result, (pchar)addr(ELOGFIELDS[logfld])) ;
    else
      strcpy (result, "Unknown") ;
  return result ;
}


