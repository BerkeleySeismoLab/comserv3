



/*   Lib660 Command Processing
     Copyright 2017-2021 by
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
    0 2017-06-07 rdr Created
    1 2017-06-30 rdr When calculating max_spread use the lower of the priority
                     limit from BE660 or the client limit.
    2 2017-07-19 rdr Add handling of @IDL at end of IP address for internal data logger.
                     In start_deallocation copy data_timetag to saved_data_timetag if
                     needed for Resume.
    3 2017-07-21 rdr Set q660->opstat.ipv6 in lib_continue_registration and q660->ipv6 in
                     open_socket.
    4 2018-04-03 rdr Add sending host_ident.
    5 2018-04-19 jms added missing "err ="
    6 2018-07-19 rdr Add using table for tcp errors to get registration wait timer. Change
                     registration timeout to 30 seconds for those cases where there is no
                     connection refused error (system not running). Increment dynip_age
                     when in LIBSTATE_WAIT. Add sending <log_stat> XML to BE every minute.
    7 2018-09-24 jms protect, regardless the root cause, for the case of unreasonable
                     saved timetag. method used good for 68, not 136 years. if linux,
                     which is a proxy for an embedded target, impose additional restrictions
                     on saved timetag.
    8 2018-10-30 rdr Expand STATSTRS to include IDL status.
    9 2018-11-14 rdr If start_newer is TRUE then use <resume> instead of <start>.
   10 2019-09-26 rdr Add calling init_ll_lcq.
   11 2019-11-07 jms remove most special checks for validity of saved_continuity
                     when resuming. this is no longer needed as suitable responses
                     are returned from be660 in cases of resume requests outside
                     the time range held by be660.
                     change memcpy to memmove for overlapping data in tcp input buffer.
   12 2020-01-22 rdr Remove DT_LLDATA handling.
   13 2020-02-12 rdr Add handling of network retry override.
   14 2020-02-12 rdr When looking for internal data logger simulation look for loopback
                     address or the @IDL, the @IDL will not be presented when initiated by
                     a POC. Add handling of new DT_DISCON packet.
   15 2020-02-13 rdr If within a second of data delay disconnecting until we get IB_EOS.
  12b 2020-02-20 jms buffer all XML dialog to prevent reception confusion at BE660.
                     add various debugging, default off.
                     shorten longesst entries in RETRY_TAB
  13b 2020-03-10 jms assure that tcpidx and lastidx are zeroed at start of XML dialog.
                     to prevent reading stale data after incomplete prior registration.
                     added handling of "Not Ready" error from BE660 that may occur now
                     on an XDL connection prior to BE660 startup complete. reduces
                     connection time, since this case is distinguished from other errors.
                     additional conditional debugging of registration negotiations.
   16 2021-01-27 rdr Clean up the XML to BE buffering mechanism.
   17 2021-04-04 rdr Add Dust handling.
   18 2021-12-11 jms various temporary debugging prints.
   19 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
   20 2022-03-01 jms added throttle and BSL enhancements
   21 2022-03-07 jms use queued message add during registration
   22 2022-04-01 jms added BW fill
*/


#undef LINUXDEBUGPRINT

#include "libcmds.h"
#include "libclient.h"
#include "libdata.h"
#include "libmsgs.h"
#include "libsample.h"
#include "libverbose.h"
#include "libsampcfg.h"
#include "libsampglob.h"
#include "libsupport.h"
#include "libstats.h"
#include "libcont.h"
#include "liblogs.h"
#include "libopaque.h"
#include "libarchive.h"
#include "xmlseed.h"
#include "sha256.h"

#define CHUNK_SIZE 10000
#define RETRY_ENTRIES 7
#define CALSTAT_CLEAR_TIMEOUT 200 /* 20 second backup cal status clearing */

typedef U16 tretrytab[RETRY_ENTRIES] ;

typedef string3 tstatstrs[ST_SIZE] ;
const tstatstrs STATSTRS = {"SM", "GPS", "PLL", "LS", "IDL", "DST", ""} ;
const tretrytab RETRY_TAB = {30, 10, 10, 15, 30, 60, 60} ; /* Retry times in seconds */

void close_socket (pq660 q660)
{

    if (q660->cpath != INVALID_SOCKET) {
#ifdef X86_WIN32
        closesocket (q660->cpath) ;
#else
        close (q660->cpath) ;
#endif
        q660->cpath = INVALID_SOCKET ;
    }
}

void start_deallocation (pq660 q660)
{

    libmsgadd (q660, LIBMSG_DEALLOC, "") ;
    new_state (q660, LIBSTATE_DEALLOC) ;
    q660->saved_data_timetag = q660->data_timetag ; /* For resume */
    deallocate_sg (q660) ;
    new_state (q660, LIBSTATE_WAIT) ;
    q660->registered = FALSE ;
    close_socket (q660) ;
}

void disconnect_me (pq660 q660)
{

    lib_change_state (q660, LIBSTATE_WAIT, LIBERR_NOTR) ;
    close_socket (q660) ;

    if (q660->par_register.opt_nro)
        q660->reg_wait_timer = q660->par_register.opt_nro ;
    else {
        q660->reg_wait_timer = RETRY_TAB[q660->retry_index] ;

        if (q660->retry_index < (RETRY_ENTRIES - 1))
            (q660->retry_index)++ ;
    }

    q660->registered = FALSE ;
}

void tcp_error (pq660 q660, pchar msgsuf)
{
    string95 s ;

    disconnect_me (q660) ;
    sprintf (s, "%s, Waiting %d seconds", msgsuf, q660->reg_wait_timer) ;
    libmsgadd (q660, LIBMSG_PKTERR, s) ;
}

void write_xmlbuf (pq660 q660)
{
    int err ;
    string31 s1 ;

    if (q660->cpath != INVALID_SOCKET) {
        err = (int)send(q660->cpath, &(q660->xml_buf), q660->xml_buf_lth, 0) ;

        if (err == SOCKET_ERROR) {
            err =
#ifdef X86_WIN32
                WSAGetLastError() ;
#else
                errno ;
#endif
            sprintf (s1, "%d", err) ;
            tcp_error (q660, s1) ;
        } else
            add_status (q660, LOGF_SENTBPS, (int)strlen(q660->xml_buf)) ;
    }
}

void init_xmlbuf (pq660 q660)
{
    memclr (&(q660->xml_buf), BUFSOCKETSIZE) ;
    q660->xml_buf_lth = 0 ;
}

/* points to address of current buffer pointer. pointer is updated by strlen(s)+1 */
void write_socket (pq660 q660, pchar s)
{

    memcpy (&(q660->xml_buf[q660->xml_buf_lth]), s, strlen(s)) ; /* don't copy null terminator */
    q660->xml_buf_lth = q660->xml_buf_lth + strlen(s) ;
}

/* points to address of current buffer pointer. pointer is updated by strlen(s)+1 */
void writeln_socket (pq660 q660, pchar s)
{
    write_socket (q660, s) ;
    write_socket (q660, "\r\n") ;
}

void change_status_request (pq660 q660)
{
    string63 s ;
    enum tstype idx ;

    init_xmlbuf (q660) ;
    writeln_socket (q660, "<Q660_Data>") ;
    writeln_socket (q660, "<status>") ;

    if (q660->initial_status) {
        writeln_socket (q660, " <interval>1</interval>") ;
        strcpy (s, "SM,GPS,PLL,LS,DST") ;
    } else {
        /* Build custom list */
        sprintf (s, " <interval>%d</interval>", q660->share.status_interval) ;
        writeln_socket (q660, s) ;
        strcpy (s, "SM") ;

        for (idx = ST_GPS ; idx <= ST_DUST ; idx++) {
            strcat (s, ",") ;
            strcat (s, (pchar)&(STATSTRS[idx])) ;

            if ((q660->share.extra_status & (1 << (U16)idx)) == 0)
                strcat (s, ":3") ;
        }
    }

    write_socket (q660, " <include>") ;
    write_socket (q660, s) ;
    writeln_socket (q660, "</include>") ;
    writeln_socket (q660, "</status>") ;
    writeln_socket (q660, "</Q660_Data>") ;
    write_xmlbuf (q660) ;
}

void lib_continue_registration (pq660 q660)
{
    U16 ctrlport ;
    int lth ;
    struct sockaddr_in xyz ;
    struct sockaddr_in6 xyz6 ;
    string63 s ;

    init_xmlbuf (q660) ;

    if (q660->ipv6) {
        lth = sizeof(struct sockaddr_in6) ;
        getsockname (q660->cpath, (pvoid)&(xyz6), (pvoid)&(lth)) ;
        ctrlport = ntohs(xyz6.sin6_port) ;
        memcpy (&(q660->share.opstat.current_ipv6), &(xyz6.sin6_addr), sizeof(tip_v6)) ;
    } else {
        lth = sizeof(struct sockaddr) ;
        getsockname (q660->cpath, (pvoid)&(xyz), (pvoid)&(lth)) ;
        ctrlport = ntohs(xyz.sin_port) ;
        q660->share.opstat.current_ip4 = ntohl(xyz.sin_addr.s_addr) ;
    }

    q660->share.opstat.ipv6 = q660->ipv6 ;
    sprintf (s, "on port %d", (int)ctrlport) ;
    libmsgadd(q660, LIBMSG_SOCKETOPEN, s) ;
    q660->share.opstat.current_port = ctrlport ;
    new_state (q660, LIBSTATE_REQ) ;
    q660->registered = FALSE ;
    lock (q660) ;
    q660->share.have_config = 0 ;
    q660->share.have_status = 0 ;
    q660->share.want_config = 0 ;
    q660->status_timer = 0 ;
    unlock (q660) ;
    q660->tcpidx = 0 ;
    q660->lastidx = 0 ;
    writeln_socket (q660, "<Q660_Data>") ;
    writeln_socket (q660, "<regreq>") ;
    write_socket (q660, " <sn>") ;
    write_socket (q660, t64tostring (&(q660->par_create.q660id_serial), s)) ;
    writeln_socket (q660, "</sn>") ;

    writeln_socket (q660, " <features>1</features>") ; /* minimally, V1 lib660. ignored by V0 */

    if ((strstr(q660->par_register.q660id_address, "@IDL") == NULL) && 
         (q660->par_register.opt_throttle_kbitpersec))
    {
        sprintf(s," <throttlekb>%d</throttlekb>",q660->par_register.opt_throttle_kbitpersec) ;
        writeln_socket (q660, s) ; /* throttle ignored by V0 Q8 */
        
        sprintf(s,"%d",q660->par_register.opt_throttle_kbitpersec) ;
        msgadd(q660, LIBMSG_THROTTLE, 0, s, TRUE) ;       
    }

    if (q660->par_register.opt_bwfill_kbit_target > 0)
    {
        sprintf(s," <bwfill>%d,%d,%d,%d,%d</bwfill>",
                    q660->par_register.opt_bwfill_kbit_target,
                    q660->par_register.opt_bwfill_probe_interval,
                    q660->par_register.opt_bwfill_exceed_trigger,
                    q660->par_register.opt_bwfill_increase_interval,
                    q660->par_register.opt_bwfill_max_latency);
        writeln_socket (q660, s) ; /* bwfill ignored by V0 Q8 */

        sprintf(s,"%d,%d,%d,%d,%d",
                    q660->par_register.opt_bwfill_kbit_target,
                    q660->par_register.opt_bwfill_probe_interval,
                    q660->par_register.opt_bwfill_exceed_trigger,
                    q660->par_register.opt_bwfill_increase_interval,
                    q660->par_register.opt_bwfill_max_latency);
        msgadd(q660, LIBMSG_BWFILL, 0, s, TRUE) ;
    }

    writeln_socket (q660, "</regreq>") ;
    writeln_socket (q660, "</Q660_Data>") ;
    write_xmlbuf (q660) ;
    q660->reg_timer = 0 ;
}

void enable_packet_streaming (pq660 q660)
{

    init_xmlbuf (q660) ;
    writeln_socket (q660, "<Q660_Data>") ;
    writeln_socket (q660, " <packet_mode>1</packet_mode>") ;
    writeln_socket (q660, "</Q660_Data>") ;
    write_xmlbuf (q660) ;
    q660->tcpidx = 0 ;
    q660->lastidx = 0 ;
    q660->retry_index = 0 ; /* go back to starting retry time */
}

void send_user_message (pq660 q660, pchar msg)
{

    init_xmlbuf (q660) ;
    writeln_socket (q660, "<Q660_Data>") ;
    write_socket (q660, " <usermsg>") ;
    write_socket (q660, msg) ;
    writeln_socket (q660, "</usermsg>") ;
    writeln_socket (q660, "</Q660_Data>") ;
    write_xmlbuf (q660) ;
}

void send_logger_stat (pq660 q660, int val, double tage)
{
    string15 s ;

    init_xmlbuf (q660) ;
    writeln_socket (q660, "<Q660_Data>") ;
    write_socket (q660, " <log_stat>") ;
    sprintf (s, "%d,%4.3f", val, tage) ; /* inform V1 host of last packet time */
    write_socket (q660, s) ;
    writeln_socket (q660, "</log_stat>") ;
    writeln_socket (q660, "</Q660_Data>") ;
    write_xmlbuf (q660) ;
    if (q660->cur_verbosity & VERB_PACKET)
    {
        sprintf(s," log_stat=%d,%4.3f",val,tage);
        libmsgadd(q660, LIBMSG_PKTIN, s) ;
    }        
    
} ;

void send_chan_mask (pq660 q660)
{
    int i ;
    string95 s ;
    string15 s1 ;

    init_xmlbuf (q660) ;
    s[0] = 0 ;

    for (i = 0 ; i <= TOTAL_CHANNELS ; i++) {
        if (i)
            strcat (s, ",") ;

        sprintf (s1, "%X", (int)q660->mymask[i]) ;
        strcat (s, s1) ;
    }

    writeln_socket (q660, "<Q660_Data>") ;
    write_socket (q660, " <block>") ;
    write_socket (q660, s) ;
    writeln_socket (q660, "</block>") ;
    writeln_socket (q660, "</Q660_Data>") ;
    write_xmlbuf (q660) ;
} ;

static BOOLEAN open_socket (pq660 q660)
{
    int flag, j, err  ;
    U32 ip ;
    U16 port ;
    BOOLEAN is_ipv6 ;
    BOOLEAN internal_dl ;
    struct sockaddr_in *psock ;
    struct sockaddr_in6 *psock6 ;
    tip_v6 ip_v6 ;
    string s ;
    pchar pc ;
#ifdef X86_WIN32
    BOOL flag2 ;
#else
    int flag2 ;
#endif

    close_socket (q660) ;
    is_ipv6 = FALSE ;
    strcpy (s, q660->par_register.q660id_address) ;
    internal_dl = FALSE ;
    pc = strstr (s, "@IDL") ;

    if (pc) {
        internal_dl = TRUE ;
        *pc = 0 ; /* Remove from address */
    } else {
        pc = strstr (s, "127.0.0.1") ;

        if (pc)
            internal_dl = TRUE ;
    }

    ip = get_ip_address (s, &(ip_v6), &(is_ipv6), q660->par_register.prefer_ipv4) ;

    if ((! is_ipv6) && (ip == INADDR_NONE)) {
        libmsgadd(q660, LIBMSG_ADDRERR, q660->par_register.q660id_address) ;
        return TRUE ;
    }

    if (is_ipv6)
        q660->cpath = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP) ;
    else
        q660->cpath = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP) ;

    if (q660->cpath == INVALID_SOCKET) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif
        sprintf (s, "%d on Q660 port", err) ;
        libmsgadd(q660, LIBMSG_SOCKETERR, s) ;
        return TRUE ;
    }

    if (internal_dl)
        port = q660->par_register.q660id_baseport + 5 ;
    else
        port = q660->par_register.q660id_baseport + 2 ;

    if (is_ipv6) {
        psock6 = (pointer) &(q660->csock6) ;
        memset(psock6, 0, sizeof(struct sockaddr_in6)) ;
        psock6->sin6_family = AF_INET6 ;
        psock6->sin6_port = htons(port) ;
        memcpy (&(psock6->sin6_addr), &(ip_v6), sizeof(tip_v6)) ;
    } else {
        psock = (pointer) &(q660->csock) ;
        memset(psock, 0, sizeof(struct sockaddr)) ;
        psock->sin_family = AF_INET ;
        psock->sin_port = htons(port) ;
        psock->sin_addr.s_addr = ip ; /* always big-endian */
    }

    flag = 1 ;
#ifdef X86_WIN32
    ioctlsocket (q660->cpath, FIONBIO, (pvoid)&(flag)) ;
#else

    if (q660->cpath > q660->high_socket)
        q660->high_socket = q660->cpath ;

    flag = fcntl (q660->cpath, F_GETFL, 0) ;
    fcntl (q660->cpath, F_SETFL, flag | O_NONBLOCK) ;
#endif
    flag2 = 1 ;
#ifdef X86_WIN32
    j = sizeof(BOOL) ;
#else
    j = sizeof(int) ;
#endif
    setsockopt (q660->cpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)&(flag2), j) ;

    if (is_ipv6)
        err = connect (q660->cpath, (pvoid)&(q660->csock6), sizeof(struct sockaddr_in6)) ;
    else
        err = connect (q660->cpath, (pvoid)&(q660->csock), sizeof(struct sockaddr)) ;

    if (err) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif

        if ((err != EWOULDBLOCK) && (err != EINPROGRESS)) {
            close_socket (q660) ;
            sprintf (s, "%d on Q660 port", err) ;
            libmsgadd(q660, LIBMSG_SOCKETERR, s) ;
            return TRUE ;
        }
    }

    q660->ipv6 = is_ipv6 ;
    return FALSE ;
}

void lib_start_registration (pq660 q660)
{

    if (open_socket (q660)) {
        lib_change_state (q660, LIBSTATE_WAIT, LIBERR_NETFAIL) ;
        q660->reg_wait_timer = 60 * 10 ;
        q660->registered = FALSE ;
        libmsgadd (q660, LIBMSG_SNR, "10 Minutes") ;
        return ;
    } ;

    q660->reg_timer = 0 ;

    new_state (q660, LIBSTATE_CONN) ;
}

static void build_strings (pq660 q660)
{
    pchar pc, buf, limit ;

    limit = (pchar)((PNTRINT)q660->xmlbuf + q660->xmlsize - 1) ;
    (*(q660->xmlbuf))[q660->xmlsize - 1] = 0 ; /* Null terminate to use search routines */
    buf = (pchar)q660->xmlbuf ;

    do {
        pc = strchr (buf, 0xD) ;

        if (pc == NIL)
            pc = strchr(buf, 0xA) ; /* some sort of Unix BS */

        if (pc) {
            if (srccount < (MAXLINES - 1))
                srcptrs[srccount++] = buf ;

            *pc++ = 0 ; /* terminate and skip to next */

            if (*pc == 0xA)
                pc++ ; /* skip the LF following CR */

            buf = pc ;
        }
    } while (! ((pc == NIL) || (buf >= limit))) ;
}

/* Header is in qdphdr, payload is in qdp_data */
static void process_packet (pq660 q660, int lth)
{
    PU8 p, pb ;
    int i ;
    I32 mask ;
    enum tstype idx ;
    string95 s ;
    string63 s1 ;

    add_status (q660, LOGF_PACKRECV, 1) ;
    p = (PU8)&(q660->qdp_data) ;

    if (q660->cur_verbosity & VERB_PACKET) {
        /* log the packet received */
        switch ((U8)(q660->qdphdr.cmd_ver & CMD_MASK)) {
        case DT_DATA :
            packet_time(lib_round(q660->data_timetag), s) ;
            break ;

        default :
            strcpy (s, " ") ;
            break ;
        }

        strcpy (s, command_name (q660->qdphdr.cmd_ver & CMD_MASK, s1)) ;
        sprintf (s1, ", Seqs=%d, Dlength=%d", (int)q660->qdphdr.seqs,
                 (int)(((U16)q660->qdphdr.dlength + 1) << 2)) ;
        strcat (s, s1) ;
        pb = p ;

        switch ((U8)(q660->qdphdr.cmd_ver & CMD_MASK)) {
        case DT_DATA :
            sprintf (s1, ", Sec=%d, FB=$%2X", (int)loadlongword(&(pb)),
                     (int)loadbyte(&(pb))) ;
            strcat (s, s1) ;
            break ;

        case DT_STATUS :
            sprintf (s1, ", Mask=$%3X", (int)loadlongint(&(pb))) ;
            strcat (s, s1) ;
            break ;
        } ;

        libmsgadd(q660, LIBMSG_PKTIN, s) ;
    }

#ifdef LINUXDEBUGPRINT
    {
        static int K=0;
        fprintf(stderr,"K%d\n",K++);
        fflush(stderr);
    }
#endif

    switch ((U8)(q660->qdphdr.cmd_ver & CMD_MASK)) {
    case DT_DATA :
        pb = p ;
        q660->last_data_packet_received_local_timestamp = now() ;
        q660->last_data_packet_received_embedded_timestamp = loadlongword(&pb) ;
        
        process_data (q660, p, lth) ;
        break ;

    case DT_STATUS :
        mask = loadlongint (&(p)) ;

        for (idx = ST_SM ; idx <= ST_DUST ; idx++)
            if (mask & (1 << (U16)idx))
                switch (idx) {
                case ST_SM :
                    loadstatsm (&(p), &(q660->share.stat_sm)) ;
                    q660->share.opstat.clock_qual = q660->share.stat_sm.clock_qual ;
                    q660->share.opstat.clock_drift = q660->share.stat_sm.usec_offset ;
                    q660->share.opstat.pll_stat = q660->share.stat_sm.pll_status ;

                    for (i = 0 ; i <= BOOM_COUNT - 1 ; i++)
                        q660->share.opstat.mass_pos[i] = q660->share.stat_sm.booms[i] ;

                    q660->share.opstat.sys_temp = q660->share.stat_sm.sys_temp ;
#ifdef DEVSOH
                    q660->share.opstat.pwr_volt = q660->share.stat_sm.input_volts * 0.002 ;
                    q660->share.opstat.pwr_cur = q660->share.stat_sm.sys_cur * 0.0001 ;
#else
                    q660->share.opstat.pwr_volt = q660->share.stat_sm.input_volts * 0.01 ;
                    q660->share.opstat.pwr_cur = q660->share.stat_sm.sys_cur * 0.001 ;
#endif
                    q660->share.opstat.pkt_full = q660->share.stat_sm.pkt_buf ;
                    q660->share.opstat.gpio1 = q660->share.stat_sm.gpio1 ;
                    q660->share.opstat.gpio2 = q660->share.stat_sm.gpio2 ;

                    if ((q660->share.stat_sm.cal_stat & (CAL_SGON | CAL_ENON)) == 0) {
                        if (q660->calstat_clear_timer == 0)
                            q660->calstat_clear_timer = CALSTAT_CLEAR_TIMEOUT ;
                    } else
                        q660->calstat_clear_timer = 0 ; /* not running */

                    break ;

                case ST_GPS :
                    loadstatgps (&(p), &(q660->share.stat_gps)) ;
                    update_gps_stats (q660) ;

                    if (q660->need_sats)
                        finish_log_clock (q660) ;

                    break ;

                case ST_PLL :
                    loadstatpll (&(p), &(q660->share.stat_pll)) ;
                    break ;

                case ST_LS :
                    loadstatlogger (&(p), &(q660->share.stat_logger)) ;

                    if (q660->need_regmsg) {
                        q660->need_regmsg = FALSE ;
                        strcpy (s, q660->par_create.host_software) ;
                        strcat (s, " registered") ;
                        send_user_message (q660, s) ;
                    }

                    break ;

                case ST_DUST :
                    loadstatdust (&(p), &(q660->share.stat_dust)) ;
                    break ;

                default :
                    break ;
                }

        q660->status_timer = 0 ;
        q660->last_status_received = now () ;
        new_status (q660, mask) ; /* Let client know what we have */

        if (q660->initial_status) {
            q660->initial_status = FALSE ;
            log_all_info (q660) ;
        }

        q660->initial_status = FALSE ;

        if (q660->status_change_pending) {
            q660->status_change_pending = FALSE ;
            change_status_request (q660) ;
        }

        break ;

    case DT_DISCON :
        libmsgadd (q660, LIBMSG_DISCREQ, "") ;

        if (q660->within_second)
            q660->discon_pending = TRUE ; /* Need to wait */
        else
            disconnect_me (q660) ;

        break ;
    }
}

static void extract_value (pchar src, pchar dest)
{
    pchar pc ;

    *dest = 0 ; /* nothing there as default */
    src = strstr (src, ">") ;

    if (src == NIL)
        return ;

    src++ ; /* skip > */
    pc = strstr (src, "<") ;

    if (pc == NIL)
        return ;

    *pc = 0 ; /* null terminate */
    strcpy (dest, src) ;
}

static void check_for_disconnect (pq660 q660, int err)
{
    string31 s ;

    if (q660->par_register.opt_disc_latency) {
        /* Power Cycled, this is ok */
        close_socket (q660) ;
        q660->reg_wait_timer = (int)q660->par_register.opt_connwait * 60 ;
        lib_change_state (q660, LIBSTATE_WAIT, LIBERR_BUFSHUT) ;
        libmsgadd (q660, LIBMSG_BUFSHUT, "") ;
    } else if (err == ECONNRESET)
        tcp_error (q660, "Connection Reset") ;
    else {
        sprintf (s, "%d", err) ;
        tcp_error (q660, s) ;
    }
}

void read_cmd_socket (pq660 q660)
{
    int i, err, idx, lth, flth ;
    int start_inner, end_inner ;
    int value, good ;
    txmlline buf ;
    string95 s1, s2, s3, s4 ;
    U8 dlength, complth ;
    string s ;
    pchar pc ;
    PU8 p ;
    U32 t ;
    U32 t_limit ;

    if (q660->cpath == INVALID_SOCKET)
        return ;

    switch (q660->libstate) {
    case LIBSTATE_XML :
        i = q660->xmlsize - q660->tcpidx + 1 ;

        if (i > CHUNK_SIZE)
            i = CHUNK_SIZE ;

        pc = (pchar)((PNTRINT)q660->xmlbuf + q660->tcpidx) ;
        err = (int)recv (q660->cpath, pc, i, 0) ;
        break ;

    default :
        err = (int)recv (q660->cpath, (pvoid)&(q660->tcpbuf[q660->tcpidx]),
                             TCPBUFSZ - q660->tcpidx, 0) ;
        break ;
    }

    if (err == SOCKET_ERROR) {
        err =
#ifdef X86_WIN32
            WSAGetLastError() ;
#else
            errno ;
#endif
#ifdef X86_WIN32

        if ((err == ECONNRESET) || (err == ECONNABORTED))
#else
        if (err == EPIPE)
#endif

            check_for_disconnect (q660, err) ;
        else if (err != EWOULDBLOCK) {
            sprintf (s, "%d", err) ;
            tcp_error (q660, s) ;
        }

        return ;
    } else if (err == 0) {
        check_for_disconnect (q660, ECONNRESET) ;
        return ;
    } else if (err < 0)
        return ;

    add_status (q660, LOGF_RECVBPS, err) ;

    switch (q660->libstate) {
    case LIBSTATE_RUN : /* Streaming Binary Packets */
#ifdef LINUXDEBUGPRINT
    {
        static int R=0;
        fprintf(stderr,"R%d\n",R++);
        fflush(stderr);
    }

#endif
    q660->tcpidx = q660->tcpidx + err ;

    while (q660->tcpidx >= QDP_HDR_LTH) {
        p = (PU8)&(q660->tcpbuf[2]) ;
        dlength = loadbyte (&(p)) ;
        complth = loadbyte (&(p)) ;
        lth = ((U16)dlength + 1) << 2 ;

        if ((lth > MAXDATA) || (complth != ((~ dlength) & 0xFF))) {
            tcp_error (q660, "Invalid Data Packet") ;
            return ;
        }

        if (q660->tcpidx >= (lth + QDP_HDR_LTH + 4)) {
            /* have a full packet, or more */
            p = (PU8)&(q660->tcpbuf) ;
            lth = loadqdphdr (&(p), &(q660->qdphdr)) ;

            if (lth == 0) {
                tcp_error (q660, "Invalid Data Packet CRC") ;
                add_status (q660, LOGF_CHKERR, 1) ;
                return ;
            }

            memcpy (&(q660->qdp_data[0]), &(q660->tcpbuf[QDP_HDR_LTH]), lth) ; /* actual data */
            flth = lth + QDP_HDR_LTH + 4 ; /* amount actually used */
            q660->tcpidx = q660->tcpidx - flth ; /* amount left over */

            if (q660->tcpidx > 0)
                memmove (&(q660->tcpbuf), &(q660->tcpbuf[flth]), q660->tcpidx) ;

            process_packet (q660, lth) ;
        } else
            break ; /* not enough to process */
    }

    break ;

    case LIBSTATE_XML :
        q660->tcpidx = q660->tcpidx + err ;

        if (q660->tcpidx >= q660->xmlsize) {
            /* done with download */
            xml_lock () ;
            initialize_xml_reader (&(q660->connmem), FALSE) ;
            build_strings (q660) ;
            read_sysinfo () ;
            read_maindigis () ;
            read_accel () ;
            read_seed () ;
            q660->xmlcopy.psysinfo = &(q660->share.sysinfo) ;
            q660->xmlcopy.pdigis = &(q660->share.digis) ;
            q660->xmlcopy.pdispatch = NIL ;
            q660->xmlcopy.pmdispatch = NIL ;
            q660->xmlcopy.pseed = &(q660->share.seed) ;
            make_xml_copy (&(q660->xmlcopy)) ;
            xml_unlock () ;
            q660->chanchain = q660->xmlcopy.pchanchain ;
            q660->dlchain = q660->xmlcopy.pdlchain ;
            q660->xiirchain = q660->xmlcopy.piirchain ;
            q660->share.extra_status = 0 ;
            q660->share.status_interval = 10 ;
            q660->share.opstat.runtime = 0 ;
            q660->registered = TRUE ;

            if (q660->cur_verbosity & VERB_REGMSG)
                q660->need_regmsg = TRUE ;

            if (q660->cur_verbosity & VERB_SDUMP)
                q660->initial_status = TRUE ;

            change_status_request (q660) ;
            sprintf (s, "%d bytes", q660->xmlsize) ;
            libmsgadd (q660, LIBMSG_XMLREAD, s) ;
            new_state (q660, LIBSTATE_CFG) ;
            q660->linecount = 0 ;
            q660->tcpidx = 1 ;
            q660->lastidx = 0 ;
            q660->sensora_ok = FALSE ;
            q660->sensorb_ok = FALSE ;

            for (i = 0 ; i <= 2 ; i++)
                if (q660->share.digis[i].freqmap) {
                    q660->sensora_ok = TRUE ;
                    break ;
                }

            for (i = 3 ; i <= 5 ; i++)
                if (q660->share.digis[i].freqmap) {
                    q660->sensorb_ok = TRUE ;
                    break ;
                }

            value = q660->share.sysinfo.spslimit ;

            if (value > q660->par_register.opt_maxsps)
                value = q660->par_register.opt_maxsps ; /* reduce to my request */

            if (value <= 1)
                q660->max_spread = 10 ;
            else
                q660->max_spread = 1 ;

            if (q660->share.sysinfo.spslimit < q660->par_register.opt_maxsps) {
                sprintf (s, "SPS Limited to %d", (int)q660->share.sysinfo.spslimit) ;
                libmsgadd (q660, LIBMSG_PRILIMIT, s) ;
            }

            set_net_station (q660) ;
            new_cfg (q660, 1) ;
        }

        break ;

    default :
        q660->tcpidx = q660->tcpidx + err ;
        idx = q660->lastidx ; /* normally zero */

        while (idx < q660->tcpidx) {
            if ((q660->tcpbuf[idx] == 0xd) || (q660->tcpbuf[idx] == 0xa)) {
                /* found end of line */
                memcpy (&(q660->srclines[q660->linecount]), &(q660->tcpbuf[q660->lastidx]),
                        idx - q660->lastidx) ;
                q660->srclines[q660->linecount][idx - q660->lastidx] = 0 ;
                (q660->linecount)++ ;

                if ((q660->tcpbuf[idx] == 0xd) && (q660->tcpbuf[idx + 1] == 0xa))
                    (idx)++ ; /* skip over first line ending */

                q660->lastidx = idx + 1 ; /* start of next line */

                if (q660->linecount >= MAXRESPLINES) {
                    strcpy (buf, (pchar)&(q660->srclines[1])) ;
                    pc = strstr (buf, "<cfgsize>") ;

                    if (pc) {
                        extract_value (buf, s2) ;
                        good = sscanf (s2, "%d", &(value)) ;

                        if (good == 1) {
                            q660->xmlsize = value ;
                            getxmlbuf (&(q660->connmem),
                                       (pvoid)&(q660->xmlbuf), q660->xmlsize + 2) ;
                            memcpy (q660->xmlbuf, &(q660->tcpbuf[1]), q660->tcpidx - 1) ;
                            (q660->tcpidx)-- ; /* Skipping leading character */
                            new_state (q660, LIBSTATE_XML) ;
                        }
                    }

                    q660->linecount = 0 ;
                    break ;
                }
            }

            (idx)++ ;
        }

        start_inner = -1 ;
        end_inner = 0 ;

        if ((q660->linecount >= 1) &&
                (strstr((pchar)&(q660->srclines[0]), "<Q660_D") == NIL))

        {
            lib_change_state (q660, LIBSTATE_WAIT, LIBERR_XMLERR) ;
            q660->registered = FALSE ;
            sprintf (s, "%d seconds", q660->piu_retry) ;
            libmsgadd(q660, LIBMSG_XMLERR, s) ;
            q660->reg_wait_timer = q660->piu_retry ;
            return ;
        }

        for (idx = 0 ; idx <= q660->linecount - 1 ; idx++) {
            strcpy (buf, (pchar)&(q660->srclines[idx])) ;
            pc = strstr(buf, "<Q660_D") ;

            if (pc)
                start_inner = idx + 1 ;
            else if (start_inner >= 0) {
                pc = strstr (buf, "</Q660_Data>") ;

                if (pc) {
                    end_inner = idx - 1 ;
                    break ;
                }
            }
        }

        if ((start_inner >= 0) && (end_inner > 0) && (end_inner >= start_inner)) {
            /* Have complete request */
            add_status (q660, LOGF_PACKRECV, 1) ;

            switch (q660->libstate) {
            case LIBSTATE_REQ :
                strcpy (buf, (pchar)&(q660->srclines[start_inner])) ;
                pc = strstr (buf, "<challenge>") ;

                if (pc) {
                    extract_value (buf, s2) ;
                    init_xmlbuf (q660) ;
                    writeln_socket (q660, "<Q660_Data>") ;
                    writeln_socket (q660, "<regresp>") ;
                    write_socket (q660, " <sn>") ;
                    write_socket (q660, t64tostring (&(q660->par_create.q660id_serial), s4)) ;
                    writeln_socket (q660, "</sn>") ;
                    write_socket (q660, " <priority>") ;
                    sprintf (s, "%d", (int)q660->par_create.q660id_priority) ;
                    write_socket (q660, s) ;
                    writeln_socket (q660, "</priority>") ;
                    write_socket (q660, " <challenge>") ;
                    write_socket (q660, s2) ;
                    writeln_socket (q660, "</challenge>") ;
                    s3[0] = 0 ;

                    for (i = 0 ; i < 8 ; i++) {
                        value = (U32)newrand (&(q660->rsum)) ; /* make sure doesn't sign extend */
                        sprintf (s1, "%x", value) ;
                        zpad (s1, 4) ;
                        strcat (s3, s1) ;
                    }

                    write_socket (q660, " <random>") ;
                    write_socket (q660, s3) ;
                    writeln_socket (q660, "</random>") ;
                    strcpy (s, s4) ;
                    strcat (s, s2) ;
                    strcat (s, q660->par_register.q660id_pass) ;
                    strcat (s, s3) ;
                    sha256 (s, q660->hash) ;
                    write_socket (q660, " <hash>") ;
                    write_socket (q660, q660->hash) ;
                    writeln_socket (q660, "</hash>") ;

                    switch (q660->par_register.opt_start) {
                    case OST_NEW :
                        t = 0 ;
                        break ;

                    case OST_LAST :
                        t = lib_uround(q660->saved_data_timetag) ;

                        if ((((int)t) <= ((int)OST_LAST)) || (!isnormal(q660->saved_data_timetag))) 
                        {
                            t = 0 ; /* invalid saved timetag. */
                            q660->par_register.opt_start = OST_NEW ;
                        } else 
                        {
                            /* Optionally limit the amount of backfill requested. */
                            /* the limit is only approximate, depending on accuracy of the */
                            /* receiver system clock, as now() is used to estimate current UTC. */
                            /* any limit must be positive and > 60s to be credible */
                            if (q660->par_register.opt_limit_last_backfill > 60) 
                            {
                                t_limit = now() - q660->par_register.opt_limit_last_backfill;
                                if (t < t_limit) 
                                    t = t_limit; /* limit backlog */
                            }
                            t-- ; /* Move back one second for overlap. time is within the last 68 years. */
                            sprintf(s, "%s", jul_string(t, s1));
                            libmsgadd (q660, LIBMSG_STARTAT, s) ;
                        }
                        break ;

                    default :
                        t = q660->par_register.opt_start ;
                        break ;
                    }

                    sprintf (s, "%u", (unsigned int)t) ;

                    if ((q660->par_register.opt_start == OST_LAST) ||
                            (q660->par_register.start_newer))
                    {
                        write_socket (q660, " <resume>") ;
                        write_socket (q660, s) ;
                        writeln_socket (q660, "</resume>") ;
                    } else {
                        write_socket (q660, " <start>") ;
                        write_socket (q660, s) ;
                        writeln_socket (q660, "</start>") ;
                    }

                    /* backward-compatible library operation is to set start time now to OST_LAST. */
                    /* BSL mode, maybe preferable, is to set OST_LAST after first data received */
                    if (!(q660->par_register.opt_client_mode & LMODE_BSL))
                        q660->par_register.opt_start = OST_LAST ;
                    sprintf (s, "%d", (int)q660->par_register.opt_maxsps) ;
                    write_socket (q660, " <maxsps>") ;
                    write_socket (q660, s) ;
                    writeln_socket (q660, "</maxsps>") ;

                    if (q660->par_register.opt_token) {
                        sprintf (s, "%u", (unsigned int)q660->par_register.opt_token) ;
                        write_socket (q660, " <poc_token>") ;
                        write_socket (q660, s) ;
                        writeln_socket (q660, "</poc_token>") ;
                    }

                    if (q660->par_create.host_ident[0]) {
                        write_socket (q660, " <ident>") ;
                        write_socket (q660, q660->par_create.host_ident ) ;
                        writeln_socket (q660, "</ident>") ;
                    }

                    writeln_socket (q660, "</regresp>") ;
                    writeln_socket (q660, "</Q660_Data>") ;
                    write_xmlbuf (q660) ;
                    new_state (q660, LIBSTATE_RESP) ;
                    q660->linecount = 0 ;
                    q660->tcpidx = 1 ;
                    q660->lastidx = 0 ;
                }

                break ;

            case LIBSTATE_RESP :
                strcpy (buf, (pchar)&(q660->srclines[start_inner])) ;
                pc = strstr (buf, "<error>") ;

                if (pc) {
                    value = q660->piu_retry ; /* assume piu retry interval */
                    extract_value (buf, s2) ;
                    lib_change_state (q660, LIBSTATE_WAIT, LIBERR_INVREG) ;
                    q660->registered = FALSE ;
                    pc = strstr (s2, "Requested") ;

                    if (pc)
                        libmsgadd(q660, LIBMSG_BADTIME, "") ;
                    else {
                        pc = strstr (s2, "Ready") ;

                        if (pc) {
                            libmsgadd(q660, LIBMSG_NOTREADY, "") ;
                            value = 5 ; /* default retry for server not ready */
                        }
                    }

                    sprintf (s, "%d seconds", value) ;
                    libmsgadd(q660, LIBMSG_INVREG, s) ;
                    q660->reg_wait_timer = value ;
                }

                break ;

            default :
                break ;
            }
        }

        break ;
    }
}

void lib_timer (pq660 q660)
{
#ifdef TEST_BLOCKING
    int i ;
#endif
    string s ;

    if (q660->contmsg[0]) {
        libmsgadd (q660, LIBMSG_CONTIN, q660->contmsg) ;
        q660->contmsg[0] = 0 ;
    }

    if (q660->libstate != LIBSTATE_RUN) {
        q660->conn_timer = 0 ;
        q660->share.opstat.gps_age = -1 ;
    } else
        q660->dynip_age = 0 ;

    if (q660->libstate != q660->share.target_state) {
        q660->share.opstat.runtime = 0 ;

        switch (q660->share.target_state) {
        case LIBSTATE_WAIT :
        case LIBSTATE_IDLE :
            switch (q660->libstate) {
            case LIBSTATE_IDLE :
                restore_thread_continuity (q660, FALSE, NIL) ; /* load up DP statistics LCQ's */
                purge_thread_continuity (q660) ; /* and mark as purged */
                new_state (q660, q660->share.target_state) ;
                break ;

            case LIBSTATE_CONN :
            case LIBSTATE_REQ :
            case LIBSTATE_RESP :
                close_socket (q660) ;
                new_state (q660, q660->share.target_state) ;
                break ;

            case LIBSTATE_XML :
            case LIBSTATE_CFG :
            case LIBSTATE_RUNWAIT :
            case LIBSTATE_RUN :
                start_deallocation(q660) ;
                break ;

            case LIBSTATE_WAIT :
                new_state (q660, q660->share.target_state) ;
                break ;

            default :
                break ;
            }

            break ;

        case LIBSTATE_RUNWAIT :
            switch (q660->libstate) {
            case LIBSTATE_CFG :
                strcpy (q660->station_ident, q660->share.seed.network) ;
                strcat (q660->station_ident, "-") ;
                strcat (q660->station_ident, q660->share.seed.station) ;
                init_lcq (q660) ;
                /*              setup_control_detectors (q660) ; */
                init_ll_lcq (q660) ;
                process_dplcqs (q660) ;
                strcpy (s, q660->station_ident) ;
                strcat (s, ", ") ;
                strcat (s, q660->par_create.host_software) ;
                libmsgadd(q660, LIBMSG_NETSTN, s) ;
                check_continuity (q660) ;

                if (q660->data_timetag > 1) { /* if non-zero continuity was restored, write cfg blks at start of session */
                    libdatamsg (q660, LIBMSG_RESTCONT, "") ;
                    q660->contingood = restore_continuity (q660) ;

                    if (! q660->contingood)
                        libdatamsg (q660, LIBMSG_CONTNR, "") ;
                }

                q660->first_data = TRUE ;
                q660->last_data_qual = NO_LAST_DATA_QUAL ;
                q660->data_timer = 0 ;
                q660->status_timer = 0 ;

                if (q660->arc_size > 0)
                    preload_archive (q660, TRUE, NIL) ;

                new_state (q660, q660->share.target_state) ;
                break ;

            case LIBSTATE_RUN :
                start_deallocation(q660) ;
                break ;

            case LIBSTATE_IDLE :
                restore_thread_continuity (q660, FALSE, NIL) ; /* load up DP statistics LCQ's */
                purge_thread_continuity (q660) ; /* and mark as purged */
                lib_start_registration (q660) ;
                break ;

            case LIBSTATE_WAIT :
                lib_start_registration (q660) ;
                break ;

            default :
                break ;
            }

            break ;

        case LIBSTATE_RUN :
            switch (q660->libstate) {
            case LIBSTATE_RUNWAIT :
#ifdef TEST_BLOCKING
                for (i = 0 ; i < TOTAL_CHANNELS ; i++)
                    q660->mymask[i] = 0x3FE ; /* Everything but 1Hz */

                send_chan_mask (q660) ;
#endif
                enable_packet_streaming (q660) ;
                add_status (q660, LOGF_COMMSUCC, 1) ; /* complete cycle */
                new_state (q660, LIBSTATE_RUN) ;
                break ;

            default :
                break ;
            }

            break ;

        case LIBSTATE_TERM :
            if (! q660->terminate) {
                flush_dplcqs (q660) ;
                save_thread_continuity (q660) ;
                q660->terminate = TRUE ;
            }

            break ;

        default :
            break ;
        }
    }

    switch (q660->libstate) {
    case LIBSTATE_IDLE :
        (q660->timercnt)++ ;

        if (q660->timercnt >= 10) {
            /* about 1 second */
            q660->timercnt = 0 ;
            q660->share.opstat.runtime = q660->share.opstat.runtime - 1 ;
            continuity_timer (q660) ;
        }

        return ;

    case LIBSTATE_TERM :
        return ;

    case LIBSTATE_WAIT :
        (q660->timercnt)++ ;

        if (q660->timercnt >= 10) {
            /* about 1 second */
            q660->timercnt = 0 ;
            (q660->dynip_age)++ ;
            (q660->reg_wait_timer)-- ;

            if (q660->reg_wait_timer <= 0) {
                if ((q660->par_register.opt_dynamic_ip) &&
                        ((q660->par_register.q660id_address[0] == 0) ||
                         ((q660->par_register.opt_ipexpire) && ((q660->dynip_age / 60) >= q660->par_register.opt_ipexpire))))

                    q660->reg_wait_timer = 1 ;
                else {
                    q660->par_register.opt_token = 0 ;
                    lib_change_state (q660, LIBSTATE_RUNWAIT, LIBERR_NOERR) ;
                }
            }

            q660->share.opstat.runtime = q660->share.opstat.runtime - 1 ;
            continuity_timer (q660) ;
        }

        return ;

    case LIBSTATE_RUN :
        if (++(q660->logstat_timer) >= 300) {
            /* about 2 per minute */
            q660->logstat_timer = 0 ; /* return last timestamp seen for server latency calc */
            send_logger_stat (q660, q660->last_data_packet_received_embedded_timestamp,
                                (now() - q660->last_data_packet_received_local_timestamp)) ;
        }

        break ;

    default :
        break ;
    }

    lock (q660) ;

    if (q660->share.usermessage_requested) {
        q660->share.usermessage_requested = FALSE ;
        send_user_message (q660, q660->share.user_message.msg) ;
    }

    unlock (q660) ;
    dump_msgqueue (q660) ;

    if (q660->flush_all) {
        q660->flush_all = FALSE ;

        if (q660->libstate == LIBSTATE_RUN)
            flush_lcqs (q660) ;

        if (q660->libstate >= LIBSTATE_RESP)
            flush_dplcqs (q660) ;
    }

    (q660->timercnt)++ ;

    if (q660->timercnt >= 10) {
        /* about 1 second */
        q660->timercnt = 0 ;

        if (q660->share.freeze_timer > 0) {
            lock (q660) ;
            (q660->share.freeze_timer)-- ;
            unlock (q660) ;
        } else
            continuity_timer (q660) ;

        if (q660->libstate != LIBSTATE_RUN) {
            (q660->dynip_age)++ ;
            (q660->share.opstat.runtime)-- ;
        } else {
            (q660->share.opstat.runtime)++ ;

            if (q660->log_timer > 0) {
                (q660->log_timer)-- ;

                if (q660->log_timer <= 0)
                    flush_messages (q660) ;
            }
        }

        switch (q660->libstate) {
        case LIBSTATE_RUN :
            if (q660->share.freeze_timer <= 0)
                (q660->data_timer)++ ;

            if (q660->data_timer > q660->data_timeout) {
                lib_change_state (q660, LIBSTATE_WAIT, LIBERR_DATATO) ;
                q660->reg_wait_timer = q660->data_timeout_retry ;
                sprintf (s, "%d Minutes", q660->data_timeout_retry / 60) ;
                libmsgadd (q660, LIBMSG_DATATO, s) ;
                add_status (q660, LOGF_COMMATMP, 1) ;
            }

            if (q660->conn_timer < MAXLINT) {
                (q660->conn_timer)++ ;

                if ((q660->par_register.opt_conntime) && (q660->share.target_state != LIBSTATE_WAIT) &&
                        ((q660->conn_timer / 60) >= q660->par_register.opt_conntime) && (q660->share.freeze_timer <= 0))

                {
                    q660->reg_wait_timer = (int)q660->par_register.opt_connwait * 60 ;
                    lib_change_state (q660, LIBSTATE_WAIT, LIBERR_CONNSHUT) ;
                    libmsgadd (q660, LIBMSG_CONNSHUT, "") ;
                }
            }

            if ((q660->par_register.opt_end) && (lib_uround(q660->data_timetag) > q660->par_register.opt_end)) {
                q660->reg_wait_timer = 9999999 ;
                lib_change_state (q660, LIBSTATE_WAIT, LIBERR_ENDTIME) ;
                libmsgadd (q660, LIBMSG_ENDTIME, "") ;
            } else if ((q660->par_register.opt_disc_latency) && (q660->share.opstat.data_latency != INVALID_LATENCY) &&
                       (q660->share.opstat.data_latency <= q660->par_register.opt_disc_latency))

            {
                q660->reg_wait_timer = (int)q660->par_register.opt_connwait * 60 ;
                lib_change_state (q660, LIBSTATE_WAIT, LIBERR_BUFSHUT) ;
                libmsgadd (q660, LIBMSG_BUFSHUT, "") ;
            }

            if ((q660->calstat_clear_timer > 0) && (--(q660->calstat_clear_timer) <= 0)) {
                q660->calstat_clear_timer = -1 ; /* Done */
                clear_calstat (q660) ;
            }

            break ;

        case LIBSTATE_CONN :
        case LIBSTATE_REQ :
        case LIBSTATE_RESP :
            (q660->reg_timer)++ ;

            if (q660->reg_timer > 30) { /* 30 seconds */
                lib_change_state (q660, LIBSTATE_WAIT, LIBERR_REGTO) ;
                q660->registered = FALSE ;
                add_status (q660, LOGF_COMMATMP, 1) ;
                (q660->reg_tries)++ ;

                if ((q660->par_register.opt_hibertime) &&
                        (q660->par_register.opt_regattempts) &&
                        (q660->reg_tries >= q660->par_register.opt_regattempts))

                {
                    q660->reg_tries = 0 ;
                    q660->reg_wait_timer = (int)q660->par_register.opt_hibertime * 60 ;
                } else
                    q660->reg_wait_timer = 120 ;

                sprintf (s, "%d Minutes", q660->reg_wait_timer / 60) ;
                libmsgadd (q660, LIBMSG_SNR, s) ;
            }

            break ;

        default :
            break ;
        }

        if ((q660->libstate >= LIBSTATE_XML) & (q660->libstate <= LIBSTATE_DEALLOC)) {
            (q660->status_timer)++ ;

            if (q660->status_timer > q660->status_timeout) {
                lib_change_state (q660, LIBSTATE_WAIT, LIBERR_STATTO) ;
                q660->reg_wait_timer = q660->status_timeout_retry ;
                sprintf (s, "%d Minutes", q660->status_timeout_retry / 60) ;
                libmsgadd (q660, LIBMSG_STATTO, s) ;
                add_status (q660, LOGF_COMMATMP, 1) ;
            }
        }
    }
}

