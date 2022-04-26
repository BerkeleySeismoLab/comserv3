



/*   Lib660 Message Handlers
     Copyright 2017, 2020 by
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
    0 2017-06-09 rdr Created
    1 2020-02-05 rdr Missing string for LIBMSG_ENDTIME added.
    2 2020-02-12 rdr Missing string for LIBMSG_PRILIMIT added. LIBMSG_DISCREQ added.
   1b 2020-03-07 jms add LIBMSG_NOTREADY.
    3 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
    4 2022-03-01 jms added throttle and BSL enhancement messages
    5 2022-03-31 jms add BWFILL message    

*/
#include "libmsgs.h"
#include "libsampglob.h"
#include "libsupport.h"
#include "liblogs.h"

pchar lib_get_msg (U16 code, pchar result)
{
    string95 s ;

    sprintf(s, "Unknown Message Number %d ", code) ; /* default */

    switch (code / 100) {
    case 0 : /* debugging */
        switch (code) {
        case LIBMSG_GENDBG :
            strcpy(s, "") ; /* generic, all info in suffix */ 
            break ;

        case LIBMSG_PKTIN :
            strcpy(s, "Recv") ;
            break ;

        case LIBMSG_PKTOUT :
            strcpy(s, "Sent") ;
            break ;
        }

        break ;

    case 1 : /* informational */
        switch (code) {
        case LIBMSG_USER :
            strcpy(s, "Msg From ") ;
            break ;

        case LIBMSG_DECNOTFOUND :
            strcpy(s, "Decimation filter not found for ") ;
            break ;

        case LIBMSG_FILTDLY :
            strcpy(s, "Filters & delay ") ;
            break ;

        case LIBMSG_SEQBEG :
            strcpy (s, "Data begins at ") ;
            break ;

        case LIBMSG_SEQOVER :
            strcpy(s, "Data resumes overlapping: ") ;
            break ;

        case LIBMSG_SEQRESUME :
            strcpy(s, "Data resumes: ") ;
            break ;

        case LIBMSG_CONTBOOT :
            strcpy(s, "Data continuity lost due to ") ;
            break ;

        case LIBMSG_CONTFND :
            strcpy(s, "Continuity found: ") ;
            break ;

        case LIBMSG_BUFSHUT :
            strcpy(s, "De-Registration due to reaching disconnect latency time") ;
            break ;

        case LIBMSG_CONNSHUT :
            strcpy(s, "De-Registration due to reaching maximum connection time") ;
            break ;

        case LIBMSG_NOIP :
            strcpy(s, "No initial IP Address specified, waiting for POC") ;
            break ;

        case LIBMSG_ENDTIME :
            strcpy(s, "De-Registration due to reaching end time") ;
            break ;

        case LIBMSG_RESTCONT :
            strcpy(s, "Restoring continuity") ;
            break ;

        case LIBMSG_DISCARD :
            strcpy(s, "Discarded 1: ") ;
            break ;

        case LIBMSG_CSAVE :
            strcpy(s, "Continuity saved: ") ;
            break ;

        case LIBMSG_DETECT :
            strcpy(s, "") ; /* all info in suffix */ break ;

        case LIBMSG_NETSTN :
            strcpy(s, "Station: ") ;
            break ;

        case LIBMSG_TOTAL :
            strcpy(s, "") ; /* all info in suffix */ break ;

        case LIBMSG_PRILIMIT :
            strcpy(s, "Priority Limitation: ") ;
            break ;

        case LIBMSG_DISCREQ :
            strcpy(s, "De-Registration due to disconnect request from digitizer") ;
            break ;
        
        case LIBMSG_STARTAT :
            strcpy(s, "Connection starting at: ") ;
            break ;

        case LIBMSG_THROTTLE :
            strcpy(s, "Connection throttle (kbit/sec) request set to: ") ;
            break ;
            
        case LIBMSG_BWFILL : 
            strcpy(s, "Bandwidth Fill Parameter Block request set to: ") ;
            break ;
        }


        break ;

    case 2 : /* success code */
        switch (code) {
        case LIBMSG_CREATED :
            strcpy(s, "Station Thread Created") ;
            break ;

        case LIBMSG_REGISTERED :
            strcpy(s, "Registered with Q660 ") ;
            break ;

        case LIBMSG_DEALLOC :
            strcpy(s, "De-Allocating Data Structures") ;
            break ;

        case LIBMSG_DEREG :
            strcpy(s, "De-Registered with Q660") ;
            break ;

        case LIBMSG_XMLREAD :
            strcpy(s, "XML loaded, size=") ;
            break ;

        case LIBMSG_POCRECV :
            strcpy(s, "POC Received: ") ;
            break ;

        case LIBMSG_WRCONT :
            strcpy(s, "Writing Continuity File: ") ;
            break ;

        case LIBMSG_SOCKETOPEN :
            strcpy(s, "Socket Opened ") ;
            break ;

        case LIBMSG_CONN :
            strcpy(s, "Connected to Q660") ;
            break ;
        }

        break ;

    case 3 : /* converted Q660 blockettes */
        switch (code) {
        case LIBMSG_GPSSTATUS :
            strcpy(s, "New GPS Status=") ;
            break ;

        case LIBMSG_DIGPHASE :
            strcpy(s, "New Digitizer Phase=") ;
            break ;

        case LIBMSG_LEAPDET :
            strcpy(s, "Leap Second Detected") ;
            break ;

        case LIBMSG_PHASERANGE :
            strcpy(s, "Phase Out of Range of ") ;
            break ;

        case LIBMSG_TIMEJMP :
            strcpy(s, "Time Jump of ") ;
            break ;
        }

        break ;

    case 4 : /* client faults */
        switch (code) {
        case LIBMSG_XMLERR :
            strcpy(s, "Error reading XML, Will retry registration in ") ;
            break ;

        case LIBMSG_SNR :
            strcpy(s, "Not Registered, Will retry registration in ") ;
            break ;

        case LIBMSG_INVREG :
            strcpy(s, "Invalid Registration Request, Will retry registration in ") ;
            break ;

        case LIBMSG_CONTIN :
            strcpy(s, "Continuity Error: ") ;
            break ;

        case LIBMSG_DATATO :
            strcpy(s, "Data Timeout, Will retry registration in ") ;
            break ;

        case LIBMSG_CONCRC :
            strcpy(s, "Continuity CRC Error, Ignoring rest of file: ") ;
            break ;

        case LIBMSG_CONTNR :
            strcpy(s, "Continuity not restored") ;
            break ;

        case LIBMSG_STATTO :
            strcpy(s, "Status Timeout, Will retry registration in ") ;
            break ;

        case LIBMSG_CONPURGE :
            strcpy(s, "Continuity was expired, Ignoring rest of file: ") ;
            break ;

        case LIBMSG_BADTIME :
            strcpy(s, "Requested Data Not Found") ;
            break ;
        }

        break ;

    case 5 : /* server faults */
        switch (code) {
        case LIBMSG_SOCKETERR :
            strcpy(s, "Open Socket error: ") ;
            break ;

        case LIBMSG_ADDRERR :
            strcpy (s, "Invalid Nework Address: ") ;
            break ;

        case LIBMSG_SEQGAP :
            strcpy(s, "Sequence Gap ") ;
            break ;

        case LIBMSG_LBFAIL :
            strcpy(s, "Last block failed on ") ;
            break ;

        case LIBMSG_NILRING :
            strcpy(s, "NIL Ring on ") ;
            break ;

        case LIBMSG_UNCOMP :
            strcpy(s, "Uncompressable Data on ") ;
            break ;

        case LIBMSG_CONTERR :
            strcpy(s, "Continuity Error on ") ;
            break ;

        case LIBMSG_TIMEDISC :
            strcpy(s, "time label discontinuity: ") ;
            break ;

        case LIBMSG_RECOMP :
            strcpy(s, "Re-compression error on ") ;
            break ;

        case LIBMSG_PKTERR :
            strcpy(s, "Network error: ") ;
            break ;

        case LIBMSG_NOTREADY :
            strcpy(s, "BE660 Not Ready: ") ;
            break ;
        }

        break ;

    case 6 :
        strcpy(s, "") ; /* contained in suffix */
        break ;

    case 7 : /* aux server messages */
        switch (code) {
        case AUXMSG_SOCKETOPEN :
            strcpy(s, "Socket Opened ") ;
            break ;

        case AUXMSG_SOCKETERR :
            strcpy(s, "Open Socket error: ") ;
            break ;

        case AUXMSG_BINDERR :
            strcpy(s, "Bind error: ") ;
            break ;

        case AUXMSG_LISTENERR :
            strcpy(s, "Listen error: ") ;
            break ;

        case AUXMSG_DISCON :
            strcpy(s, "Client Disconnected from ") ;
            break ;

        case AUXMSG_ACCERR :
            strcpy(s, "Accept error: ") ;
            break ;

        case AUXMSG_CONN :
            strcpy(s, "Client Connected ") ;
            break ;

        case AUXMSG_NOBLOCKS :
            strcpy(s, "No Blocks Available ") ;
            break ;

        case AUXMSG_SENT :
            strcpy(s, "Sent: ") ;
            break ;

        case AUXMSG_RECVTO :
            strcpy(s, "Receive Timeout ") ;
            break ;

        case AUXMSG_WEBLINK :
            strcpy(s, "") ; /* contained in suffix */ break ;

        case AUXMSG_RECV :
            strcpy(s, "Recv: ") ;
            break ;
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
    case LIBERR_NOERR :
        strcpy(s, "No error") ;
        break ;

    case LIBERR_NOTR :
        strcpy(s, "You are not registered") ;
        break ;

    case LIBERR_INVREG :
        strcpy(s, "Invalid Registration Request") ;
        break ;

    case LIBERR_ENDTIME :
        strcpy(s, "Reached End Time") ;
        break ;

    case LIBERR_XMLERR :
        strcpy(s, "Error reading XML") ;
        break ;

    case LIBERR_THREADERR :
        strcpy(s, "Could not create thread") ;
        break ;

    case LIBERR_REGTO :
        strcpy(s, "Registration Timeout, ") ;
        break ;

    case LIBERR_STATTO :
        strcpy(s, "Status Timeout, ") ;
        break ;

    case LIBERR_DATATO :
        strcpy(s, "Data Timeout, ") ;
        break ;

    case LIBERR_NOSTAT :
        strcpy(s, "Your requested status is not yet available") ;
        break ;

    case LIBERR_INVSTAT :
        strcpy(s, "Your requested status in not a valid selection") ;
        break ;

    case LIBERR_CFGWAIT :
        strcpy(s, "Your requested configuration is not yet available") ;
        break ;

    case LIBERR_BUFSHUT :
        strcpy(s, "Shutdown due to reaching disconnect latency time") ;
        break ;

    case LIBERR_CONNSHUT :
        strcpy(s, "Shutdown due to reaching maximum connection time") ;
        break ;

    case LIBERR_CLOSED :
        strcpy(s, "Closed by host") ;
        break ;

    case LIBERR_NETFAIL :
        strcpy(s, "Networking Failure") ;
        break ;

    case LIBERR_INVCTX :
        strcpy(s, "Invalid Context") ;
        break ;
    }

    strcpy (result, s) ;
    return result ;
}

pchar lib_get_statestr (enum tlibstate state, pchar result)
{
    string63 s ;

    strcpy(s, "Unknown State") ;

    switch (state) {
    case LIBSTATE_IDLE :
        strcpy(s, "Not connected to Q660") ;
        break ;

    case LIBSTATE_TERM :
        strcpy(s, "Terminated") ;
        break ;

    case LIBSTATE_CONN :
        strcpy(s, "TCP Connect Wait") ;
        break ;

    case LIBSTATE_REQ :
        strcpy(s, "Requesting Registration") ;
        break ;

    case LIBSTATE_RESP :
        strcpy(s, "Registration Response") ;
        break ;

    case LIBSTATE_XML :
        strcpy(s, "Reading Configuration") ;
        break ;

    case LIBSTATE_CFG :
        strcpy(s, "Configuring") ;
        break ;

    case LIBSTATE_RUNWAIT :
        strcpy(s, "Ready to Run") ;
        break ;

    case LIBSTATE_RUN :
        strcpy(s, "Running") ;
        break ;

    case LIBSTATE_DEALLOC :
        strcpy(s, "De-allocating structures") ;
        break ;

    case LIBSTATE_WAIT :
        strcpy(s, "Waiting for a new registration") ;
        break ;
    }

    strcpy (result, s) ;
    return result ;
}

void dump_msgqueue (pq660 q660)
{

    if ((q660->libstate != LIBSTATE_TERM) && ((q660->network)[0]) &&
            (q660->msgq_out != q660->msgq_in) && (q660->msg_lcq) && (q660->msg_lcq->com->ring))

    {
        msglock (q660) ;

        while (q660->msgq_out != q660->msgq_in) {
            log_message (q660, q660->msgq_out->msg) ;
            q660->msgq_out = q660->msgq_out->link ;
        }

        msgunlock (q660) ;
    }
}

#ifdef CONSTMSG
void msgadd (pq660 q660, U16 msgcode, U32 dt, const pchar msgsuf, BOOLEAN client)
#else
void msgadd (pq660 q660, U16 msgcode, U32 dt, pchar msgsuf, BOOLEAN client)
#endif
{
    string s, s1, s2 ;

    msglock (q660) ;
    q660->msg_call.context = q660 ;
    q660->msg_call.timestamp = lib_round(now()) ;
    q660->msg_call.datatime = dt ;
    q660->msg_call.code = msgcode ;

    if (msgcode == 0)
        s[0] = 0 ;

    strcpy(q660->msg_call.suffix, msgsuf) ;

    if (dt)
        sprintf(s1, "[%s] ", jul_string(dt, s2)) ;
    else
        strcpy(s1, " ") ;

    sprintf(s, "{%d}%s%s%s", msgcode, s1, lib_get_msg (msgcode, s2), msgsuf) ;

    if (((msgcode / 100) != 7) || (q660->cur_verbosity & VERB_AUXMSG)) {
        (q660->msg_count)++ ;
        q660->msg_call.msgcount = q660->msg_count ;

        if (! q660->nested_log) {
            if ((client) || (q660->libstate == LIBSTATE_TERM) || ((q660->network)[0] == 0) ||
                    (q660->msg_lcq == NIL) || (q660->msg_lcq->com->ring == NIL))

            {
                if (q660->msgq_in->link != q660->msgq_out) {
                    strcpy(q660->msgq_in->msg, s) ;
                    q660->msgq_in = q660->msgq_in->link ;
                }
            } else {
                while (q660->msgq_out != q660->msgq_in) {
                    log_message (q660, q660->msgq_out->msg) ;
                    q660->msgq_out = q660->msgq_out->link ;
                }

                log_message (q660, s) ;
            }
        }

        if (q660->par_create.call_messages)
            q660->par_create.call_messages (&(q660->msg_call)) ;
    }

    msgunlock (q660) ;
}

#ifdef CONSTMSG
void libmsgadd (pq660 q660, U16 msgcode, const pchar msgsuf)
#else
void libmsgadd (pq660 q660, U16 msgcode, pchar msgsuf)
#endif
{

    msgadd (q660, msgcode, 0, msgsuf, FALSE) ;
}

#ifdef CONSTMSG
void libdatamsg (pq660 q660, U16 msgcode, const pchar msgsuf)
#else
void libdatamsg (pq660 q660, U16 msgcode, pchar msgsuf)
#endif
{
    U32 dt ;

    dt = lib_round(q660->data_timetag) ;
    msgadd (q660, msgcode, dt, msgsuf, FALSE) ;
}

/* Cleaned up version derived from BIND 4.9.4 release. Returns
   address of string */
pchar inet_ntop6 (tip_v6 *src)
{
#define IN6WCNT 8 /* Number of words in an IPV6 address (256/16) */
    typedef struct
    {
        int base ;
        int len ;
    } tmatch ;
    tmatch best ;
    tmatch cur ;
    static string47 tmp ; /* returned result */
    pchar tp ;
    U16 words[IN6WCNT] ;
    int i ;
    PU16 pw ;

    /* Preprocess:
     * Copy the input (bytewise) array into a wordwise array.
     * Find the longest run of 0x00's in src[] for :: shorthanding. */
    memclr (&(words), sizeof(tip_v6)) ;
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

    for (i = 0 ; i < IN6WCNT ; i++) {
        if (words[i] == 0) {
            if (cur.base == -1) {
                cur.base = i ;
                cur.len = 1 ;
            } else
                cur.len++ ;
        } else {
            if (cur.base != -1) {
                if ((best.base == -1) || (cur.len > best.len)) {
                    best = cur ;
                    cur.base = -1 ;
                }
            }
        }
    }

    if (cur.base != -1) {
        if ((best.base == -1) || (cur.len > best.len))
            best = cur ;
    }

    if ((best.base != -1) && (best.len < 2))
        best.base = -1 ;

    /* Format the result. */
    tp = (pchar)&(tmp) ;

    for (i = 0 ; i < IN6WCNT ; i++) {
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
        tp = tp + (strlen(tp)) ;
    }

    /* Was it a trailing run of 0x00's? */
    if ((best.base != -1) && ((best.base + best.len) == IN6WCNT))
        *tp++ = ':' ;

    *tp++ = '\0' ;
    return tmp ;
}

pchar showdot (U32 num, pchar result)
{

    sprintf(result, "%d.%d.%d.%d", (int)((num >> 24) & 255), (int)((num >> 16) & 255),
            (int)((num >> 8) & 255), (int)(num & 255)) ;
    return result ;
}

pchar command_name (U8 cmd, pchar result)
{
    string95 s ;
    int h ;

    strcpy (s, "Unknown") ;

    switch (cmd) {
    case DT_DATA :
        strcpy(s, "Normal Data Record") ;
        break ;

    case DT_STATUS :
        strcpy(s, "Status") ;
        break ;

    case DT_DISCON :
        strcpy(s, "Disconnect") ;
        break ;

    default : {
        h = cmd ;
        sprintf(s, "$%x", h) ;
    }
    }

    strcpy (result, s) ;
    return result ;
}

pchar lib_gps_pwr (enum tgpspwr gp, pchar result)
{
    typedef string31 tpwrs[GPWR_SIZE] ;
    const tpwrs PWRS = {"Off", "Powered off due to PLL Lock", "Powered off due to time limit",
                        "Powered off by command", "Powered off due to a fault",
                        "Powered on automatic", "Powered on by command", "Coldstart"
                       } ;

    if (gp <= GPWR_COLD)
        strcpy (result, (pchar)&(PWRS[gp])) ;
    else
        strcpy (result, "Unknown Power State") ;

    return result ;
}

pchar lib_gps_fix (enum tgpsfix gf, pchar result)
{
    typedef string31 tfixs[GFIX_SIZE] ;
    const tfixs FIXS = {"Off, unknown fix", "Off, no fix", "Off, last fix was 1D",
                        "Off, last fix was 2D", "Off, last fix was 3D", "On, no fix",
                        "On, unknown fix", "On, 1D fix", "On, 2D fix", "On, 3D fix"
                       } ;

    if (gf <= GFIX_ON3)
        strcpy (result, (pchar)&(FIXS[gf])) ;
    else
        strcpy (result, "Unknown Fix") ;

    return result ;
}

pchar lib_pll_state (enum tpllstat ps, pchar result)
{
    typedef string15 tplls[PLL_SIZE] ;
    const tplls PLLS = {"Off", "Hold", "Tracking", "Locked"} ;

    if (ps <= PLL_LOCK)
        strcpy (result, (pchar)&(PLLS[ps])) ;
    else
        strcpy (result, "Unknown") ;

    return result ;
}

pchar lib_log_types (enum tlogfld logfld, pchar result)
{
    const tlogfields ELOGFIELDS = {"Log Messages", "XML Configuration", "Timing Blockettes",
                                   "Data Gaps", "Re-Boots", "Received Bps", "Sent Bps", "Communications Attempts",
                                   "Communications Successes", "Packets Received", "Communications Efficiency",
                                   "POC's Received", "IP Address Changes", "Comm. Duty Cycle", "Throughput",
                                   "Missing Data", "Sequence Errors", "Checksum Errors", "Data Latency",
                                   "Low Latency Data", "Status Latency"
                                  } ;

    if (logfld < LOGF_SIZE)
        strcpy (result, (pchar)&(ELOGFIELDS[logfld])) ;
    else
        strcpy (result, "Unknown") ;

    return result ;
}


