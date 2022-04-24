/*   Lib660 Command Processing
     Copyright 2017-2018 Certified Software Corporation

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
   12 2020-02-20 jms buffer all XML dialog to prevent reception confusion at BE660.
                     add various debugging, default off.
                     shorten longesst entries in RETRY_TAB                     
   13 2020-03-10 jms assure that tcpidx and lastidx are zeroed at start of XML dialog.
                     to prevent reading stale data after incomplete prior registration.
                     added handling of "Not Ready" error from BE660 that may occur now
                     on an XDL connection prior to BE660 startup complete. reduces
                     connection time, since this case is distinguished from other errors.
                     additional conditional debugging of registration negotiations.                     
*/
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


#undef FAKEERROR
#undef DODBPRINT
#ifdef BSL_DEBUG
/* #define DODBPRINT */
#endif

#ifdef DODBPRINT
#define DBPRINT(A) A
#else
#define DBPRINT(A) 
#endif

typedef word tretrytab[RETRY_ENTRIES] ;

typedef string3 tstatstrs[ST_SIZE] ;
const tstatstrs STATSTRS = {"SM", "GPS", "PLL", "LS", "IDL"} ;
const tretrytab RETRY_TAB = {30, 10, 10, 15, 30, 60, 60} ; /* Retry times in seconds */

void close_socket (pq660 q660)
begin

  if (q660->cpath != INVALID_SOCKET)
    then
      begin
#ifdef X86_WIN32
        closesocket (q660->cpath) ;
#else
        close (q660->cpath) ;
#endif
        q660->cpath = INVALID_SOCKET ;
      end
end

void start_deallocation (pq660 q660)
begin

  libmsgadd (q660, LIBMSG_DEALLOC, "") ;
  new_state (q660, LIBSTATE_DEALLOC) ;
  q660->saved_data_timetag = q660->data_timetag ; /* For resume */
  deallocate_sg (q660) ;
  new_state (q660, LIBSTATE_WAIT) ;
  q660->registered = FALSE ;
  close_socket (q660) ;
end

void tcp_error (pq660 q660, pchar msgsuf)
begin
  string95 s ;

DBPRINT (printf("TCP error: %s\n", (char *) msgsuf);)
  lib_change_state (q660, LIBSTATE_WAIT, LIBERR_NOTR) ;
  close_socket (q660) ;
  q660->reg_wait_timer = RETRY_TAB[q660->retry_index] ;
  if (q660->retry_index < (RETRY_ENTRIES - 1))
    then
      inc(q660->retry_index) ;
  q660->registered = FALSE ;
  sprintf (s, "%s, Waiting %d seconds", msgsuf, q660->reg_wait_timer) ;
  libmsgadd (q660, LIBMSG_PKTERR, s) ;
end

void tcp_write_socket (pq660 q660, pchar s)
begin
  integer err ;
  string31 s1 ;

  if (q660->cpath != INVALID_SOCKET)
    then
      begin
        err = (integer)send(q660->cpath, s, strlen(s), 0) ;
        if (err == SOCKET_ERROR)
          then
            begin
              err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
              sprintf (s1, "%d", err) ;
              tcp_error (q660, s1) ;
            end
          else
            add_status (q660, LOGF_SENTBPS, (integer)strlen(s)) ;
      end
end


#define BUFSOCKETSIZE 4000


// points to start of buffer of size size
void init_bufsocket (char *wb, int size)
begin
  memset (wb, 0, size);
end


// points to start of buffer 
void push_bufsocket (char *wb, pq660 q660)
begin
  tcp_write_socket (q660, wb);
end


// points to address of current buffer pointer. pointer is updated by strlen(s)+1
void write_socket (char **wbp, pchar s)
begin
  char *wb = *wbp;
  char *from = (char *)s;

  if (*from) {
    while (*from)
      *wb++ = *from++;
    *wbp = wb; // update caller's pointer
  }
end


// points to address of current buffer pointer. pointer is updated by strlen(s)+1
void writeln_socket (char **wbp, pchar s)
begin
  write_socket (wbp, s) ;
  write_socket (wbp, "\r\n") ;
end

void change_status_request (pq660 q660)
begin
  string63 s ;
  enum tstype idx ;
  char bufso[BUFSOCKETSIZE];
  char *wb;

  wb = bufso;
  init_bufsocket (bufso, BUFSOCKETSIZE);

  writeln_socket (&wb, "<Q660_Data>") ;
  writeln_socket (&wb, "<status>") ;
  if (q660->initial_status)
    then
      begin
        writeln_socket (&wb, " <interval>1</interval>") ;
        strcpy (s, "SM,GPS,PLL,LS") ;
      end
    else
      begin /* Build custom list */
        sprintf (s, " <interval>%d</interval>", q660->share.status_interval) ;
        writeln_socket (&wb, s) ;
        strcpy (s, "SM") ;
        for (idx = ST_GPS ; idx <= ST_LS ; idx++)
          begin
            strcat (s, ",") ;
            strcat (s, (pchar)addr(STATSTRS[idx])) ;
            if ((q660->share.extra_status and (1 shl (word)idx)) == 0)
              then
                strcat (s, ":3") ;
          end
      end
  write_socket (&wb, " <include>") ;
  write_socket (&wb, s) ;
  writeln_socket (&wb, "</include>") ;
  writeln_socket (&wb, "</status>") ;
  writeln_socket (&wb, "</Q660_Data>") ;
  push_bufsocket (bufso, q660) ;
end

void lib_continue_registration (pq660 q660)
begin
  word ctrlport ;
  integer lth ;
  struct sockaddr_in xyz ;
  struct sockaddr_in6 xyz6 ;
  string63 s ;
  
  char bufso[BUFSOCKETSIZE];
  char *wb;
  
  
  wb = bufso;
  init_bufsocket (bufso, BUFSOCKETSIZE);

  if (q660->ipv6)
    then
      begin
        lth = sizeof(struct sockaddr_in6) ;
        getsockname (q660->cpath, (pvoid)addr(xyz6), (pvoid)addr(lth)) ;
        ctrlport = ntohs(xyz6.sin6_port) ;
        memcpy (addr(q660->share.opstat.current_ipv6), addr(xyz6.sin6_addr), sizeof(tip_v6)) ;
      end
    else
      begin
        lth = sizeof(struct sockaddr) ;
        getsockname (q660->cpath, (pvoid)addr(xyz), (pvoid)addr(lth)) ;
        ctrlport = ntohs(xyz.sin_port) ;
        q660->share.opstat.current_ip4 = ntohl(xyz.sin_addr.s_addr) ;
      end
  q660->share.opstat.ipv6 = q660->ipv6 ;
  sprintf (s, "on port %d", (integer)ctrlport) ;
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
  q660->tcpidx = 0; 
  q660->lastidx = 0;
  writeln_socket (&wb, "<Q660_Data>") ;
  writeln_socket (&wb, "<regreq>") ;
  write_socket (&wb, " <sn>") ;
  write_socket (&wb, t64tostring (addr(q660->par_create.q660id_serial), s)) ;
  writeln_socket (&wb, "</sn>") ;
  writeln_socket (&wb, "</regreq>") ;
  writeln_socket (&wb, "</Q660_Data>") ;
  push_bufsocket (bufso, q660) ;
  q660->reg_timer = 0 ;
end

void enable_packet_streaming (pq660 q660)
begin
  char bufso[BUFSOCKETSIZE];
  char *wb;

  wb = bufso;
  init_bufsocket (bufso, BUFSOCKETSIZE);

  writeln_socket (&wb, "<Q660_Data>") ;
  writeln_socket (&wb, " <packet_mode>1</packet_mode>") ;
  writeln_socket (&wb, "</Q660_Data>") ;
  push_bufsocket (bufso, q660) ;
  
  q660->tcpidx = 0 ;
  q660->lastidx = 0;
  q660->retry_index = 0 ; /* go back to starting retry time */
end

void send_user_message (pq660 q660, pchar msg)
begin
  char bufso[BUFSOCKETSIZE];
  char *wb;

  wb = bufso;
  init_bufsocket (bufso, BUFSOCKETSIZE);
  writeln_socket (&wb, "<Q660_Data>") ;
  write_socket (&wb, " <usermsg>") ;
  write_socket (&wb, msg) ;
  writeln_socket (&wb, "</usermsg>") ;
  writeln_socket (&wb, "</Q660_Data>") ;
  push_bufsocket (bufso, q660) ;
  
end

void send_logger_stat (pq660 q660, integer val)
begin
  string15 s ;
  char bufso[BUFSOCKETSIZE];
  char *wb;

  wb = bufso;
  init_bufsocket (bufso, BUFSOCKETSIZE);

  writeln_socket (&wb, "<Q660_Data>") ;
  write_socket (&wb, " <log_stat>") ;
  sprintf (s, "%d", val) ;
  write_socket (&wb, s) ;
  writeln_socket (&wb, "</log_stat>") ;
  writeln_socket (&wb, "</Q660_Data>") ;
  push_bufsocket (bufso, q660) ;
  
end ;

void send_chan_mask (pq660 q660)
begin
  integer i ;
  string95 s ;
  string15 s1 ;
  char bufso[BUFSOCKETSIZE];
  char *wb;

  wb = bufso;
  init_bufsocket (bufso, BUFSOCKETSIZE);

  s[0] = 0 ;
  for (i = 0 ; i <= TOTAL_CHANNELS ; i++)
    begin
      if (i)
        then
          strcat (s, ",") ;
      sprintf (s1, "%X", (integer)q660->mymask[i]) ;
      strcat (s, s1) ;
    end
  writeln_socket (&wb, "<Q660_Data>") ;
  write_socket (&wb, " <block>") ;
  write_socket (&wb, s) ;
  writeln_socket (&wb, "</block>") ;
  writeln_socket (&wb, "</Q660_Data>") ;
  push_bufsocket (bufso, q660) ;
  
end ;

static boolean open_socket (pq660 q660)
begin
  integer flag, j, err  ;
  longword ip ;
  word port ;
  boolean is_ipv6 ;
  boolean internal_dl ;
  struct sockaddr_in *psock ;
  struct sockaddr_in6 *psock6 ;
  tip_v6 ip_v6 ;
  string s ;
  pchar pc ;
#ifdef X86_WIN32
  BOOL flag2 ;
#else
  integer flag2 ;
#endif

  close_socket (q660) ;
  is_ipv6 = FALSE ;
  strcpy (s, q660->par_register.q660id_address) ;
  pc = strstr (s, "@IDL") ;
  if (pc)
    then
      begin
        internal_dl = TRUE ;
        *pc = 0 ; /* Remove from address */
      end
    else
      internal_dl = FALSE ;
  ip = get_ip_address (s, addr(ip_v6), addr(is_ipv6), q660->par_register.prefer_ipv4) ;
  if ((lnot is_ipv6) land (ip == INADDR_NONE))
    then
      begin
        libmsgadd(q660, LIBMSG_ADDRERR, q660->par_register.q660id_address) ;
        return TRUE ;
      end
  if (is_ipv6)
    then
      q660->cpath = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP) ;
    else
      q660->cpath = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
  if (q660->cpath == INVALID_SOCKET)
    then
      begin
        err =
#ifdef X86_WIN32
              WSAGetLastError() ;
#else
              errno ;
#endif
        sprintf (s, "%d on Q660 port", err) ;
        libmsgadd(q660, LIBMSG_SOCKETERR, s) ;
        return TRUE ;
      end
  if (internal_dl)
    then
      port = q660->par_register.q660id_baseport + 5 ;
    else
      port = q660->par_register.q660id_baseport + 2 ;
  if (is_ipv6)
    then
      begin
        psock6 = (pointer) addr(q660->csock6) ;
        memset(psock6, 0, sizeof(struct sockaddr_in6)) ;
        psock6->sin6_family = AF_INET6 ;
        psock6->sin6_port = htons(port) ;
        memcpy (addr(psock6->sin6_addr), addr(ip_v6), sizeof(tip_v6)) ;
      end
    else
      begin
        psock = (pointer) addr(q660->csock) ;
        memset(psock, 0, sizeof(struct sockaddr)) ;
        psock->sin_family = AF_INET ;
        psock->sin_port = htons(port) ;
        psock->sin_addr.s_addr = ip ; /* always big-endian */
      end
  flag = 1 ;
#ifdef X86_WIN32
  ioctlsocket (q660->cpath, FIONBIO, (pvoid)addr(flag)) ;
#else
  if (q660->cpath > q660->high_socket)
    then
      q660->high_socket = q660->cpath ;
  flag = fcntl (q660->cpath, F_GETFL, 0) ;
  fcntl (q660->cpath, F_SETFL, flag or O_NONBLOCK) ;
#endif
  flag2 = 1 ;
#ifdef X86_WIN32
  j = sizeof(BOOL) ;
#else
  j = sizeof(int) ;
#endif
  setsockopt (q660->cpath, SOL_SOCKET, SO_REUSEADDR, (pvoid)addr(flag2), j) ;
  if (is_ipv6)
    then
      err = connect (q660->cpath, (pvoid)addr(q660->csock6), sizeof(struct sockaddr_in6)) ;
    else
      err = connect (q660->cpath, (pvoid)addr(q660->csock), sizeof(struct sockaddr)) ;
  if (err)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        if ((err != EWOULDBLOCK) land (err != EINPROGRESS))
          then
            begin
              close_socket (q660) ;
              sprintf (s, "%d on Q660 port", err) ;
              libmsgadd(q660, LIBMSG_SOCKETERR, s) ;
              return TRUE ;
            end
      end
  q660->ipv6 = is_ipv6 ;
  return FALSE ;
end

void lib_start_registration (pq660 q660)
begin

  if (open_socket (q660))
    then
      begin
        lib_change_state (q660, LIBSTATE_WAIT, LIBERR_NETFAIL) ;
        q660->reg_wait_timer = 60 * 10 ;
        q660->registered = FALSE ;
        libmsgadd (q660, LIBMSG_SNR, "10 Minutes") ;
        return ;
      end ;
  q660->reg_timer = 0 ;
  new_state (q660, LIBSTATE_CONN) ;
end

static void build_strings (pq660 q660)
begin
  pchar pc, buf, limit ;

  limit = (pchar)((pntrint)q660->xmlbuf + q660->xmlsize - 1) ;
  (*(q660->xmlbuf))[q660->xmlsize - 1] = 0 ; /* Null terminate to use search routines */
  buf = (pchar)q660->xmlbuf ;
  repeat
    pc = strchr (buf, 0xD) ;
    if (pc == NIL)
      then
        pc = strchr(buf, 0xA) ; /* some sort of Unix BS */
    if (pc)
      then
        begin
          if (srccount < (MAXLINES - 1))
            then
              srcptrs[srccount++] = buf ;
          *pc++ = 0 ; /* terminate and skip to next */
          if (*pc == 0xA)
            then
              pc++ ; /* skip the LF following CR */
          buf = pc ;
        end
  until ((pc == NIL) lor (buf >= limit))) ;
end

/* Header is in qdphdr, payload is in qdp_data */
static void process_packet (pq660 q660, integer lth)
begin
  pbyte p, pb ;
  integer i ;
  longint mask ;
  enum tstype idx ;
  string95 s ;
  string63 s1 ;

  add_status (q660, LOGF_PACKRECV, 1) ;
  p = (pbyte)addr(q660->qdp_data) ;
  if (q660->cur_verbosity and VERB_PACKET)
    then
      begin /* log the packet received */
        switch ((byte)(q660->qdphdr.cmd_ver and CMD_MASK)) begin
          case DT_DATA :
            packet_time(lib_round(q660->data_timetag), s) ;
            break ;
          case DT_LLDATA :
            packet_time(lib_round(q660->ll_data_timetag), s) ;
            break ;
          default :
            strcpy (s, " ") ;
            break ;
        end
        strcpy (s, command_name (q660->qdphdr.cmd_ver and CMD_MASK, s1)) ;
        sprintf (s1, ", Seqs=%d, Dlength=%d", (integer)q660->qdphdr.seqs,
                 (integer)(((word)q660->qdphdr.dlength + 1) shl 2)) ;
        strcat (s, s1) ;
        pb = p ;
        switch ((byte)(q660->qdphdr.cmd_ver and CMD_MASK)) begin
          case DT_DATA :
          case DT_LLDATA :
            sprintf (s1, ", Sec=%d, FB=$%2X", (integer)loadlongword(addr(pb)),
                     (integer)loadbyte(addr(pb))) ;
            strcat (s, s1) ;
            break ;
          case DT_STATUS :
            sprintf (s1, ", Mask=$%3X", (integer)loadlongint(addr(pb))) ;
            strcat (s, s1) ;
            break ;
        end ;
        libmsgadd(q660, LIBMSG_PKTIN, s) ;
      end
  switch ((byte)(q660->qdphdr.cmd_ver and CMD_MASK)) begin
    case DT_DATA :
      process_data (q660, p, lth) ;
      break ;
    case DT_LLDATA :
      process_low_latency (q660, p, lth) ;
      break ;
    case DT_STATUS :
      mask = loadlongint (addr(p)) ;
      for (idx = ST_SM ; idx <= ST_LS ; idx++)
        if (mask and (1 shl (word)idx))
          then
            switch (idx) begin
              case ST_SM :
                loadstatsm (addr(p), addr(q660->share.stat_sm)) ;
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
                if ((q660->share.stat_sm.cal_stat and (CAL_SGON or CAL_ENON)) == 0)
                  then
                    begin
                      if (q660->calstat_clear_timer == 0)
                        then
                          q660->calstat_clear_timer = CALSTAT_CLEAR_TIMEOUT ;
                    end
                  else
                    q660->calstat_clear_timer = 0 ; /* not running */
                break ;
              case ST_GPS :
                loadstatgps (addr(p), addr(q660->share.stat_gps)) ;
                update_gps_stats (q660) ;
                if (q660->need_sats)
                  then
                    finish_log_clock (q660) ;
                break ;
              case ST_PLL :
                loadstatpll (addr(p), addr(q660->share.stat_pll)) ;
                break ;
              case ST_LS :
                loadstatlogger (addr(p), addr(q660->share.stat_logger)) ;
                if (q660->need_regmsg)
                  then
                    begin
                      q660->need_regmsg = FALSE ;
                      strcpy (s, q660->par_create.host_software) ;
                      strcat (s, " registered") ;
                      send_user_message (q660, s) ;
                    end
                break ;
              default :
                break ;
           end
      q660->status_timer = 0 ;
      q660->last_status_received = now () ;
      new_status (q660, mask) ; /* Let client know what we have */
      if (q660->initial_status)
        then
          begin
            q660->initial_status = FALSE ;
            log_all_info (q660) ;
          end
      q660->initial_status = FALSE ;
      if (q660->status_change_pending)
        then
          begin
            q660->status_change_pending = FALSE ;
            change_status_request (q660) ;
          end
    end
end

static void extract_value (pchar src, pchar dest)
begin
  pchar pc ;

  *dest = 0 ; /* nothing there as default */
  src = strstr (src, ">") ;
  if (src == NIL)
    then
      return ;
  src++ ; /* skip > */
  pc = strstr (src, "<") ;
  if (pc == NIL)
    then
      return ;
  *pc = 0 ; /* null terminate */
  strcpy (dest, src) ;
end

static void check_for_disconnect (pq660 q660, integer err)
begin
  string31 s ;

  if (q660->par_register.opt_disc_latency)
    then
      begin /* Power Cycled, this is ok */
        close_socket (q660) ;
        q660->reg_wait_timer = (integer)q660->par_register.opt_connwait * 60 ;
        lib_change_state (q660, LIBSTATE_WAIT, LIBERR_BUFSHUT) ;
        libmsgadd (q660, LIBMSG_BUFSHUT, "") ;
      end
  else if (err == ECONNRESET)
    then
      tcp_error (q660, "Connection Reset") ;
    else
      begin
        sprintf (s, "%d", err) ;
        tcp_error (q660, s) ;
      end
end


void read_cmd_socket (pq660 q660)
begin
  integer i, err, idx, lth, flth ;
  integer start_inner, end_inner ;
  integer value, good ;
  txmlline buf ;
  string95 s1, s2, s3, s4 ;
  byte dlength, complth ;
  string s ;
  pchar pc ;
  pbyte p ;
  longword t ;
#ifdef BSL
  longword t_limit ;
#endif
#ifdef FAKEERROR
  static struct stat fsbuf;
  static int mypid;
  static char crcerr[20];
#endif

  char bufso[BUFSOCKETSIZE];
  char *wb;

#ifdef FAKEERROR
  mypid = getpid();
  sprintf (crcerr,"rm -f /tmp/ce%d",mypid);
#endif

  if (q660->cpath == INVALID_SOCKET)
    then
      return ;
  switch (q660->libstate) begin
    case LIBSTATE_XML :
      i = q660->xmlsize - q660->tcpidx + 1 ;
      if (i > CHUNK_SIZE)
        then
          i = CHUNK_SIZE ;
      pc = (pchar)((pntrint)q660->xmlbuf + q660->tcpidx) ;
      err = (integer)recv (q660->cpath, pc, i, 0) ;
      break ;
    default :
      err = (integer)recv (q660->cpath, (pvoid)addr(q660->tcpbuf[q660->tcpidx]),
                  TCPBUFSZ - q660->tcpidx, 0) ;
      break ;
  end
  if (err == SOCKET_ERROR)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
#ifdef X86_WIN32
        if ((err == ECONNRESET) lor (err == ECONNABORTED))
#else
        if (err == EPIPE)
#endif
          then
            check_for_disconnect (q660, err) ;
        else if (err != EWOULDBLOCK)
          then
            begin
              sprintf (s, "%d", err) ;
              tcp_error (q660, s) ;
            end
        return ;
      end
  else if (err == 0)
    then
      begin
DBPRINT (printf("state=%d recv err 0, read size=%d\n",q660->libstate, TCPBUFSZ - q660->tcpidx);)
        check_for_disconnect (q660, ECONNRESET) ;
        return ;
      end
  else if (err < 0)
    then
      return ;
  add_status (q660, LOGF_RECVBPS, err) ;
  switch (q660->libstate) begin
    case LIBSTATE_RUN : /* Streaming Binary Packets */
      q660->tcpidx = q660->tcpidx + err ;
      while (q660->tcpidx >= QDP_HDR_LTH)
        begin
          p = (pbyte)addr(q660->tcpbuf[2]) ;
          dlength = loadbyte (addr(p)) ;
          complth = loadbyte (addr(p)) ;
          lth = ((word)dlength + 1) shl 2 ;
          if ((lth > MAXDATA) lor (complth != ((not dlength) and 0xFF)))
            then
              begin
                tcp_error (q660, "Invalid Data Packet") ;
                return ;
              end
          if (q660->tcpidx >= (lth + QDP_HDR_LTH + 4))
            then
              begin /* have a full packet, or more */
                p = (pbyte)addr(q660->tcpbuf) ;
                lth = loadqdphdr (addr(p), addr(q660->qdphdr)) ;
#ifdef FAKEERROR
                if ((lth == 0) || (stat(crcerr+6, &fsbuf)==0))
#else
                if (lth == 0)
#endif
                  then
                    begin
                      tcp_error (q660, "Invalid Data Packet CRC") ;
                      add_status (q660, LOGF_CHKERR, 1) ;
                      return ;
                    end
                memcpy (addr(q660->qdp_data[0]), addr(q660->tcpbuf[QDP_HDR_LTH]), lth) ; /* actual data */
                flth = lth + QDP_HDR_LTH + 4 ; /* amount actually used */
                q660->tcpidx = q660->tcpidx - flth ; /* amount left over */
                if (q660->tcpidx > 0)
                  then
                    memmove (addr(q660->tcpbuf), addr(q660->tcpbuf[flth]), q660->tcpidx) ;
                process_packet (q660, lth) ;
              end
            else
              break ; /* not enough to process */
        end
      break ;
    case LIBSTATE_XML :
      q660->tcpidx = q660->tcpidx + err ;
      if (q660->tcpidx >= q660->xmlsize)
        then
          begin /* done with download */
            xml_lock () ;
            initialize_xml_reader (addr(q660->connmem), FALSE) ;
            build_strings (q660) ;
            read_sysinfo () ;
            read_maindigis () ;
            read_accel () ;
            read_seed () ;
            q660->xmlcopy.psysinfo = addr(q660->share.sysinfo) ;
            q660->xmlcopy.pdigis = addr(q660->share.digis) ;
            q660->xmlcopy.pdispatch = NIL ;
            q660->xmlcopy.pmdispatch = NIL ;
            q660->xmlcopy.pseed = addr(q660->share.seed) ;
            make_xml_copy (addr(q660->xmlcopy)) ;
            xml_unlock () ;
            q660->chanchain = q660->xmlcopy.pchanchain ;
            q660->dlchain = q660->xmlcopy.pdlchain ;
            q660->xiirchain = q660->xmlcopy.piirchain ;
            q660->share.extra_status = 0 ;
            q660->share.status_interval = 10 ;
            q660->share.opstat.runtime = 0 ;
            q660->registered = TRUE ;
            if (q660->cur_verbosity and VERB_REGMSG)
              then
                q660->need_regmsg = TRUE ;
            if (q660->cur_verbosity and VERB_SDUMP)
              then
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
              if (q660->share.digis[i].freqmap)
                then
                  begin
                    q660->sensora_ok = TRUE ;
                    break ;
                  end
            for (i = 3 ; i <= 5 ; i++)
              if (q660->share.digis[i].freqmap)
                then
                  begin
                    q660->sensorb_ok = TRUE ;
                    break ;
                  end
            value = q660->share.sysinfo.spslimit ;
            if (value > q660->par_register.opt_maxsps)
              then
                value = q660->par_register.opt_maxsps ; /* reduce to my request */
            if (value <= 1)
              then
                q660->max_spread = 10 ;
              else
                q660->max_spread = 1 ;
            if (q660->share.sysinfo.spslimit < q660->par_register.opt_maxsps)
              then
                begin
                  sprintf (s, "SPS Limited to %d", (integer)q660->share.sysinfo.spslimit) ;
                  libmsgadd (q660, LIBMSG_PRILIMIT, s) ;
                end
            if ((lnot q660->share.sysinfo.low_lat) land (q660->par_create.call_lowlatency))
              then
                libmsgadd (q660, LIBMSG_PRILIMIT, "Low Latency Disabled") ;
            set_net_station (q660) ;
            new_cfg (q660, 1) ;
          end
      break ;
    default :
      q660->tcpidx = q660->tcpidx + err ;
DBPRINT (printf("\nline %d: state=%d tcpidx=%d read=%d\n",__LINE__,q660->libstate,q660->tcpidx,err);)
      idx = q660->lastidx ; /* normally zero */
      while (idx < q660->tcpidx)
        begin
          if ((q660->tcpbuf[idx] == 0xd) lor (q660->tcpbuf[idx] == 0xa))
            then
              begin /* found end of line */
                memcpy (addr(q660->srclines[q660->linecount]), addr(q660->tcpbuf[q660->lastidx]),
                        idx - q660->lastidx) ;
                q660->srclines[q660->linecount][idx - q660->lastidx] = 0 ;
                inc(q660->linecount) ;
                if ((q660->tcpbuf[idx] == 0xd) land (q660->tcpbuf[idx + 1] == 0xa))
                  then
                    inc(idx) ; /* skip over first line ending */
                q660->lastidx = idx + 1 ; /* start of next line */
                if (q660->linecount >= MAXRESPLINES)
                  then
                    begin
                      strcpy (buf, (pchar)addr(q660->srclines[1])) ;
                      pc = strstr (buf, "<cfgsize>") ;
                      if (pc)
                        then
                          begin
                            extract_value (buf, s2) ;
                            good = sscanf (s2, "%d", addr(value)) ;
                            if (good == 1)
                              then
                                begin
                                  q660->xmlsize = value ;
                                  getxmlbuf (addr(q660->connmem),
                                            (pvoid)addr(q660->xmlbuf), q660->xmlsize + 2) ;
                                  memcpy (q660->xmlbuf, addr(q660->tcpbuf[1]), q660->tcpidx - 1) ;
                                  dec (q660->tcpidx) ; /* Skipping leading character */
                                  new_state (q660, LIBSTATE_XML) ;
                                end
                          end
                      q660->linecount = 0 ;
                      break ;
                    end
              end
          inc(idx) ;
        end
      start_inner = -1 ;
      end_inner = 0 ;
#ifdef FAKEERROR
      if (((q660->linecount >= 1) land
          (strstr((pchar)addr(q660->srclines[0]), "<Q660_D") == NIL))  || (stat(crcerr+6, &fsbuf)==0))
#else
      if ((q660->linecount >= 1) land
          (strstr((pchar)addr(q660->srclines[0]), "<Q660_D") == NIL))
#endif
        then
          begin
#ifdef FAKEERROR
            system(crcerr);
#endif
            lib_change_state (q660, LIBSTATE_WAIT, LIBERR_XMLERR) ;
            q660->registered = FALSE ;
            sprintf (s, "%d seconds", q660->piu_retry) ;
            libmsgadd(q660, LIBMSG_XMLERR, s) ;
            q660->reg_wait_timer = q660->piu_retry ;
            return ;
          end
      for (idx = 0 ; idx <= q660->linecount - 1 ; idx++)
        begin
          strcpy (buf, (pchar)addr(q660->srclines[idx])) ;
          pc = strstr(buf, "<Q660_D") ;
          if (pc)
            then
              start_inner = idx + 1 ;
          else if (start_inner >= 0)
            then
              begin
                pc = strstr (buf, "</Q660_Data>") ;
                if (pc)
                  then
                    begin
                      end_inner = idx - 1 ;
                      break ;
                    end
              end
        end
      if ((start_inner >= 0) land (end_inner > 0) land (end_inner >= start_inner))
        then
          begin /* Have complete request */
DBPRINT(printf ("\n Have complete: start=%d end=%d src[start]=%s\n",start_inner,end_inner,q660->srclines[start_inner]);)
            add_status (q660, LOGF_PACKRECV, 1) ;
            switch (q660->libstate) begin
              case LIBSTATE_REQ :
                strcpy (buf, (pchar)addr(q660->srclines[start_inner])) ;
                pc = strstr (buf, "<challenge>") ;
                if (pc)
                  then
                    begin
                      extract_value (buf, s2) ;

                      wb = bufso;
                      init_bufsocket (bufso, BUFSOCKETSIZE);

                      writeln_socket (&wb, "<Q660_Data>") ;
                      writeln_socket (&wb, "<regresp>") ;
                      write_socket (&wb, " <sn>") ;
                      write_socket (&wb, t64tostring (addr(q660->par_create.q660id_serial), s4)) ;
                      writeln_socket (&wb, "</sn>") ;
DBPRINT (printf("sn=%s\n",s4);)
                      write_socket (&wb, " <priority>") ;
                      sprintf (s, "%d", (integer)q660->par_create.q660id_priority) ;
                      write_socket (&wb, s) ;
                      writeln_socket (&wb, "</priority>") ;
                      write_socket (&wb, " <challenge>") ;
                      write_socket (&wb, s2) ;
                      writeln_socket (&wb, "</challenge>") ;
DBPRINT (printf("chal=%s\n",s2);)
                      s3[0] = 0 ;
                      for (i = 0 ; i < 8 ; i++)
                        begin
                          value = (longword)newrand (addr(q660->rsum)) ; /* make sure doesn't sign extend */
                          sprintf (s1, "%x", value) ;
                          zpad (s1, 4) ;
                          strcat (s3, s1) ;
                        end
                      write_socket (&wb, " <random>") ;
                      write_socket (&wb, s3) ;
                      writeln_socket (&wb, "</random>") ;
DBPRINT (printf("rand=%s\n",s3);)
                      strcpy (s, s4) ;
                      strcat (s, s2) ;
                      strcat (s, q660->par_register.q660id_pass) ;
                      strcat (s, s3) ;
DBPRINT (printf("pass=|%s|\n",q660->par_register.q660id_pass);)
                      sha256 (s, q660->hash) ;
                      write_socket (&wb, " <hash>") ;
                      write_socket (&wb, q660->hash) ;
                      writeln_socket (&wb, "</hash>") ;
DBPRINT (printf("hash=%s\n",q660->hash);)
                      switch (q660->par_register.opt_start) begin
                        case OST_NEW :
                          t = 0 ;
                          break ;
                        case OST_LAST :
                          t = lib_uround(q660->saved_data_timetag) ;
#if defined(linux)
                          if ((((int)t) <= ((int)OST_LAST)) || (!isnormal(q660->saved_data_timetag))) {
                              t = 0 ; // invalid saved timetag.
                              q660->par_register.opt_start = OST_NEW ;
                          } else {
#ifdef BSL
                              /* Optionally limit the amount of backfill requested. */
                              if (q660->par_register.opt_limit_last_backfill != 0) {
                                  t_limit = now() - q660->par_register.opt_limit_last_backfill;
                                  if (t < t_limit) {
                                    DBPRINT(printf("q660 starttime limited to %u (%u from state file)\n",t_limit, t);)
                                      t = t_limit;
                                  }
                              }
                              else {
				DBPRINT(printf("q660 telemetry starttime = %u (from state file)\n",t);)
			      }
#endif
                              t-- ; // Move back one second for overlap. time is within the last 68 years.
                          }
#else
                          if (((int)t) <= ((int)OST_LAST))
                            then
                              begin
                                t = 0 ;
                                q660->par_register.opt_start = OST_NEW ;
                              end
                            else
                              t-- ; /* Move back one second for overlap */
#endif
                          break ;
                        default :
                          t = q660->par_register.opt_start ;
                          break ;
                      end
                      sprintf (s, "%u", (unsigned int)t) ;
                      if ((q660->par_register.opt_start == OST_LAST) lor
                          (q660->par_register.start_newer))
                        then
                          begin
                            write_socket (&wb, " <resume>") ;
                            write_socket (&wb, s) ;
DBPRINT (printf("resume=%s\n",s);)
                            writeln_socket (&wb, "</resume>") ;
                          end
                        else
                          begin
                            write_socket (&wb, " <start>") ;
                            write_socket (&wb, s) ;
DBPRINT (printf("start=%s\n",s);)
                            writeln_socket (&wb, "</start>") ;
                          end
#ifndef BSL
                      q660->par_register.opt_start = OST_LAST ;
#endif
                      if (q660->par_register.low_lat)
                        then
                          begin
                            write_socket (&wb, " <lowlat>") ;
                            write_socket (&wb, q660->par_register.opt_lookback) ;
                            writeln_socket (&wb, "</lowlat>") ;
                          end
                      sprintf (s, "%d", (integer)q660->par_register.opt_maxsps) ;
                      write_socket (&wb, " <maxsps>") ;
                      write_socket (&wb, s) ;
DBPRINT (printf("maxsps=%s\n",s);)
                      writeln_socket (&wb, "</maxsps>") ;
                      if (q660->par_register.opt_token)
                        then
                          begin
                            sprintf (s, "%u", (unsigned int)q660->par_register.opt_token) ;
                            write_socket (&wb, " <poc_token>") ;
                            write_socket (&wb, s) ;
                            writeln_socket (&wb, "</poc_token>") ;
                          end
                      if (q660->par_create.host_ident[0])
                        then
                          begin
                            write_socket (&wb, " <ident>") ;
                            write_socket (&wb, q660->par_create.host_ident ) ;
                            writeln_socket (&wb, "</ident>") ;
                          end
                      writeln_socket (&wb, "</regresp>") ;
                      writeln_socket (&wb, "</Q660_Data>") ;
                      push_bufsocket (bufso, q660) ;
                      
                      new_state (q660, LIBSTATE_RESP) ;
DBPRINT (printf("changed to LIBSTATE_RESP\n");)
                      q660->linecount = 0 ;
                      q660->tcpidx = 1 ;
                      q660->lastidx = 0 ;
                    end
                break ;
              case LIBSTATE_RESP :
                strcpy (buf, (pchar)addr(q660->srclines[start_inner])) ;
                pc = strstr (buf, "<error>") ;
                if (pc)
                  then
                    begin
                      value = q660->piu_retry ; // assume piu retry interval
                      extract_value (buf, s2) ;
                      lib_change_state (q660, LIBSTATE_WAIT, LIBERR_INVREG) ;
                      q660->registered = FALSE ;
                      pc = strstr (s2, "Requested") ;
                      if (pc)
                        then
                          libmsgadd(q660, LIBMSG_BADTIME, "") ;
                        else
                          begin
                            pc = strstr (s2, "Ready") ;
                            if (pc)        
                              then
                                begin
                                  libmsgadd(q660, LIBMSG_NOTREADY, "") ;
                                  value = 5 ; // default retry for server not ready
DBPRINT (printf("lib660: NOT READY error\n");)
                                end
                          end
                      sprintf (s, "%d seconds", value) ;
                            libmsgadd(q660, LIBMSG_INVREG, s) ;
                      q660->reg_wait_timer = value ;
                    end
                break ;
              default :
                break ;
            end
          end
      break ;
    end
end

void lib_timer (pq660 q660)
begin
#ifdef TEST_BLOCKING
  integer i ;
#endif
  string s ;

  if (q660->contmsg[0])
    then
      begin
        libmsgadd (q660, LIBMSG_CONTIN, q660->contmsg) ;
        q660->contmsg[0] = 0 ;
      end
  if (q660->libstate != LIBSTATE_RUN)
    then
      begin
        q660->conn_timer = 0 ;
        q660->share.opstat.gps_age = -1 ;
      end
    else
      q660->dynip_age = 0 ;
  if (q660->libstate != q660->share.target_state)
    then
      begin
        q660->share.opstat.runtime = 0 ;
        switch (q660->share.target_state) begin
          case LIBSTATE_WAIT :
          case LIBSTATE_IDLE :
            switch (q660->libstate) begin
              case LIBSTATE_IDLE :
#ifdef BSL_DEBUG
DBPRINT (printf("calling restore_thread_continuity\n");)
#endif
                restore_thread_continuity (q660, FALSE, NIL) ; /* load up DP statistics LCQ's */
                purge_thread_continuity (q660) ; /* and mark as purged */
                new_state (q660, q660->share.target_state) ;
DBPRINT (printf("settarg #7 = %d\n",q660->share.target_state);)
                break ;
              case LIBSTATE_CONN :
              case LIBSTATE_REQ :
              case LIBSTATE_RESP :
DBPRINT (printf("state=%d targy=%d close socket\n",q660->libstate,q660->share.target_state);)
                close_socket (q660) ;
                new_state (q660, q660->share.target_state) ;
DBPRINT (printf("settarg #6 = %d\n",q660->share.target_state);)
                break ;
              case LIBSTATE_XML :
              case LIBSTATE_CFG :
              case LIBSTATE_RUNWAIT :
              case LIBSTATE_RUN :
DBPRINT (printf("doing deall #1\n");)
                start_deallocation(q660) ;
                break ;
              case LIBSTATE_WAIT :
                new_state (q660, q660->share.target_state) ;
DBPRINT (printf("settarg #5 = %d\n",q660->share.target_state);)
                break ;
              default :
                break ;
            end
            break ;
          case LIBSTATE_RUNWAIT :
            switch (q660->libstate) begin
              case LIBSTATE_CFG :
                strcpy (q660->station_ident, q660->share.seed.network) ;
                strcat (q660->station_ident, "-") ;
                strcat (q660->station_ident, q660->share.seed.station) ;
                init_lcq (q660) ;
//              setup_control_detectors (q660) ;
                init_ll_lcq (q660) ;
                process_dplcqs (q660) ;
                strcpy (s, q660->station_ident) ;
                strcat (s, ", ") ;
                strcat (s, q660->par_create.host_software) ;
                libmsgadd(q660, LIBMSG_NETSTN, s) ;
                check_continuity (q660) ;
                if (q660->data_timetag > 1) /* if non-zero continuity was restored, write cfg blks at start of session */
                  then
                    begin
                      libdatamsg (q660, LIBMSG_RESTCONT, "") ;
                      q660->contingood = restore_continuity (q660) ;
                      if (lnot q660->contingood)
                        then
                          libdatamsg (q660, LIBMSG_CONTNR, "") ;
                    end
                q660->first_data = TRUE ;
                q660->last_data_qual = NO_LAST_DATA_QUAL ;
                q660->data_timer = 0 ;
                q660->status_timer = 0 ;
                if (q660->arc_size > 0)
                  then
                    preload_archive (q660, TRUE, NIL) ;
                new_state (q660, q660->share.target_state) ;
DBPRINT (printf("settarg #4 = %d\n",q660->share.target_state);)
                break ;
              case LIBSTATE_RUN :
DBPRINT (printf("doing deall #2\n");)
                start_deallocation(q660) ;
                break ;
              case LIBSTATE_IDLE :
#ifdef BSL_DEBUG
DBPRINT (printf("calling restore_thread_continuity\n");)
#endif
                restore_thread_continuity (q660, FALSE, NIL) ; /* load up DP statistics LCQ's */
                purge_thread_continuity (q660) ; /* and mark as purged */
                lib_start_registration (q660) ;
                break ;
              case LIBSTATE_WAIT :
                lib_start_registration (q660) ;
                break ;
              default :
                break ;
            end
            break ;
          case LIBSTATE_RUN :
            switch (q660->libstate) begin
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
            end
            break ;
          case LIBSTATE_TERM :
            if (lnot q660->terminate)
              then
                begin
                  flush_dplcqs (q660) ;
#ifdef BSL_DEBUG
DBPRINT (printf("calling save_thread_continuity\n");)
#endif
                  save_thread_continuity (q660) ;
                  q660->terminate = TRUE ;
                end
            break ;
          default :
            break ;
        end
      end
  switch (q660->libstate) begin
    case LIBSTATE_IDLE :
      inc(q660->timercnt) ;
      if (q660->timercnt >= 10)
        then
          begin /* about 1 second */
            q660->timercnt = 0 ;
            q660->share.opstat.runtime = q660->share.opstat.runtime - 1 ;
            continuity_timer (q660) ;
          end
      return ;
    case LIBSTATE_TERM :
      return ;
    case LIBSTATE_WAIT :
      inc(q660->timercnt) ;
      if (q660->timercnt >= 10)
        then
          begin /* about 1 second */
            q660->timercnt = 0 ;
            inc(q660->dynip_age) ;
            dec(q660->reg_wait_timer) ;
            if (q660->reg_wait_timer <= 0)
              then
                begin
                  if ((q660->par_register.opt_dynamic_ip) land
                      ((q660->par_register.q660id_address[0] == 0) lor
                       ((q660->par_register.opt_ipexpire) land ((q660->dynip_age div 60) >= q660->par_register.opt_ipexpire))))
                    then
                      q660->reg_wait_timer = 1 ;
                    else
                      begin
                        q660->par_register.opt_token = 0 ;
                        lib_change_state (q660, LIBSTATE_RUNWAIT, LIBERR_NOERR) ;
                      end
                end
            q660->share.opstat.runtime = q660->share.opstat.runtime - 1 ;
            continuity_timer (q660) ;
          end
      return ;
    case LIBSTATE_RUN :
      if (++(q660->logstat_timer) >= 600)
        then
          begin /* about 1 minute */
            q660->logstat_timer = 0 ;
            send_logger_stat (q660, 0) ;
          end
      break ;
    default :
      break ;
  end
  lock (q660) ;
  if (q660->share.usermessage_requested)
    then
      begin
        q660->share.usermessage_requested = FALSE ;
        send_user_message (q660, q660->share.user_message.msg) ;
      end
  unlock (q660) ;
  dump_msgqueue (q660) ;
  if (q660->flush_all)
    then
      begin
        q660->flush_all = FALSE ;
        if (q660->libstate == LIBSTATE_RUN)
          then
            flush_lcqs (q660) ;
        if (q660->libstate >= LIBSTATE_RESP)
          then
            flush_dplcqs (q660) ;
      end
  inc(q660->timercnt) ;
  if (q660->timercnt >= 10)
    then
      begin /* about 1 second */
        q660->timercnt = 0 ;
        if (q660->share.freeze_timer > 0)
          then
            begin
              lock (q660) ;
              dec(q660->share.freeze_timer) ;
              unlock (q660) ;
            end
          else
            continuity_timer (q660) ;
        if (q660->libstate != LIBSTATE_RUN)
          then
            begin
              inc(q660->dynip_age) ;
              dec(q660->share.opstat.runtime) ;
            end
          else
            begin
              inc(q660->share.opstat.runtime) ;
              if (q660->log_timer > 0)
                then
                  begin
                    dec(q660->log_timer) ;
                    if (q660->log_timer <= 0)
                      then
                        flush_messages (q660) ;
                  end
            end
        switch (q660->libstate) begin
          case LIBSTATE_RUN :
            if (q660->share.freeze_timer <= 0)
              then
                inc(q660->data_timer) ;
            if (q660->data_timer > q660->data_timeout)
              then
                begin
                  lib_change_state (q660, LIBSTATE_WAIT, LIBERR_DATATO) ;
                  q660->reg_wait_timer = q660->data_timeout_retry ;
                  sprintf (s, "%d Minutes", q660->data_timeout_retry div 60) ;
                  libmsgadd (q660, LIBMSG_DATATO, s) ;
                  add_status (q660, LOGF_COMMATMP, 1) ;
                end
            if (q660->conn_timer < MAXLINT)
              then
                begin
                  inc(q660->conn_timer) ;
                  if ((q660->par_register.opt_conntime) land (q660->share.target_state != LIBSTATE_WAIT) land
                      ((q660->conn_timer div 60) >= q660->par_register.opt_conntime) land (q660->share.freeze_timer <= 0))
                    then
                      begin
                        q660->reg_wait_timer = (integer)q660->par_register.opt_connwait * 60 ;
                        lib_change_state (q660, LIBSTATE_WAIT, LIBERR_CONNSHUT) ;
                        libmsgadd (q660, LIBMSG_CONNSHUT, "") ;
                      end
                end
            if ((q660->par_register.opt_end) land (lib_uround(q660->data_timetag) > q660->par_register.opt_end))
              then
                begin
                  q660->reg_wait_timer = 9999999 ;
                  lib_change_state (q660, LIBSTATE_WAIT, LIBERR_ENDTIME) ;
                  libmsgadd (q660, LIBMSG_ENDTIME, "") ;
                end
            else if ((q660->par_register.opt_disc_latency) land (q660->share.opstat.data_latency != INVALID_LATENCY) land
                   (q660->share.opstat.data_latency <= q660->par_register.opt_disc_latency))
              then
                begin
                  q660->reg_wait_timer = (integer)q660->par_register.opt_connwait * 60 ;
                  lib_change_state (q660, LIBSTATE_WAIT, LIBERR_BUFSHUT) ;
                  libmsgadd (q660, LIBMSG_BUFSHUT, "") ;
                end
            if ((q660->calstat_clear_timer > 0) land (--(q660->calstat_clear_timer) <= 0))
              then
                begin
                  q660->calstat_clear_timer = -1 ; /* Done */
                  clear_calstat (q660) ;
                end
            break ;
          case LIBSTATE_CONN :
          case LIBSTATE_REQ :
          case LIBSTATE_RESP :
            inc(q660->reg_timer) ;
            if (q660->reg_timer > 30) /* 30 seconds */
              then
                begin
DBPRINT (printf("REGTIMER\n");)
                  lib_change_state (q660, LIBSTATE_WAIT, LIBERR_REGTO) ;
                  q660->registered = FALSE ;
                  add_status (q660, LOGF_COMMATMP, 1) ;
                  inc(q660->reg_tries) ;
                  if ((q660->par_register.opt_hibertime) land
                      (q660->par_register.opt_regattempts) land
                      (q660->reg_tries >= q660->par_register.opt_regattempts))
                    then
                      begin
                        q660->reg_tries = 0 ;
                        q660->reg_wait_timer = (integer)q660->par_register.opt_hibertime * 60 ;
                      end
                    else
                      q660->reg_wait_timer = 120 ;
                  sprintf (s, "%d Minutes", q660->reg_wait_timer div 60) ;
                  libmsgadd (q660, LIBMSG_SNR, s) ;
                end
            break ;
          default :
            break ;
        end
        if ((q660->libstate >= LIBSTATE_XML) and (q660->libstate <= LIBSTATE_DEALLOC))
          then
            begin
              inc(q660->status_timer) ;
              if (q660->status_timer > q660->status_timeout)
                then
                  begin
                    lib_change_state (q660, LIBSTATE_WAIT, LIBERR_STATTO) ;
                    q660->reg_wait_timer = q660->status_timeout_retry ;
                    sprintf (s, "%d Minutes", q660->status_timeout_retry div 60) ;
                    libmsgadd (q660, LIBMSG_STATTO, s) ;
                    add_status (q660, LOGF_COMMATMP, 1) ;
                  end
            end
      end
end

