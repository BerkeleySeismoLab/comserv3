/*   Lib660 client interface
     Copyright 2017 Certified Software Corporation

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
    0 2017-06-06 rdr Created
    1 2017-06-27 rdr Add returning seed structure in lib_get_sysinfo.
    2 2018-11-14 rdr Add start_newer to tpar_register.
    3 2019-07-11 rdr Add one second callback structures.
*/
#include "libstrucs.h"
#include "libmsgs.h"
#include "libsampcfg.h"
#include "libstats.h"
#include "libcont.h"
#include "libsampcfg.h"
#include "libsample.h"
#include "libsupport.h"
#include "libcompress.h"
#include "libcmds.h"
#include "libverbose.h"
#include "memutil.h"
#include "sha256.h"
#include "xmlsup.h"
#include "xmlcfg.h"
#include "libpoc.h"
#include "libdetect.h"
#include "libfilters.h"
#include "libopaque.h"
#include "liblogs.h"
#include "libarchive.h"
#include "libdata.h"

void lib_create_context (tcontext *ct, tpar_create *cfg) /* If ct = NIL return, check resp_err */
begin

  lib_create_660 (ct, cfg) ;
end

enum tliberr lib_destroy_context (tcontext *ct) /* Return error if any */
begin

  return lib_destroy_660 (ct) ;
end

enum tliberr lib_register (tcontext ct, tpar_register *rpar)
begin
  pq660 q660 ;
  enum tliberr result ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return LIBERR_INVCTX ;
  rpar->opt_token = 0 ;
  result = lib_register_660 (q660, rpar) ;
  if (q660->share.target_state == LIBSTATE_WAIT)
    then
      libmsgadd (q660, LIBMSG_NOIP, "") ;
  return result ;
end

enum tlibstate lib_get_state (tcontext ct, enum tliberr *err, topstat *retopstat)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      begin
        *err = LIBERR_INVCTX ;
        return LIBSTATE_IDLE ; /* not really */
      end
  *err = q660->share.liberr ;
  lock (q660) ;
  update_op_stats (q660) ;
  memcpy (retopstat, addr(q660->share.opstat), sizeof(topstat)) ;
  unlock (q660) ;
  return q660->libstate ;
end

void lib_change_state (tcontext ct, enum tlibstate newstate, enum tliberr reason)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return ;
  lock (q660) ;
  q660->share.target_state = newstate ;
  q660->share.liberr = reason ;
  unlock (q660) ;
end

void lib_request_status (tcontext ct, longword bitmap, word interval)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return ;
  lock (q660) ;
  if ((bitmap != q660->share.extra_status) lor (interval != q660->share.status_interval))
    then
      begin
        q660->share.extra_status = bitmap ;
        q660->share.status_interval = interval ;
        if (q660->initial_status)
          then
            q660->status_change_pending = TRUE ;
          else
            change_status_request (q660) ;
      end
  unlock (q660) ;
end

enum tliberr lib_get_status (tcontext ct, enum tstype stat_type, pointer buf)
begin
  pq660 q660 ;
  enum tliberr result ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return LIBERR_INVCTX ;
  lock (q660) ;
  if (q660->share.have_status and make_bitmap((word)stat_type))
    then
      begin
        result = LIBERR_NOERR ; /* assume good */
        switch (stat_type) begin
          case ST_SM :
            memcpy(buf, addr(q660->share.stat_sm), sizeof(tstat_sm)) ; /* Station Monitor Status */
            break ;
          case ST_GPS :
            memcpy(buf, addr(q660->share.stat_gps), sizeof(tstat_gps)) ; /* GPS Status */
            break ;
          case ST_PLL :
            memcpy(buf, addr(q660->share.stat_pll), sizeof(tstat_pll)) ; /* PLL Status */
            break ;
          case ST_LS :
            memcpy(buf, addr(q660->share.stat_logger),  sizeof(tstat_logger)) ; /* Logger Status */
            break ;
          default :
            result = LIBERR_INVSTAT ;
            break ;
        end
      end
    else
      result = LIBERR_NOSTAT ;
  unlock (q660) ;
  return result ;
end

enum tliberr lib_get_sysinfo (tcontext ct, tsysinfo *sinfo, tseed *sseed)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return LIBERR_INVCTX ;
  lock (q660) ;
  memcpy (sinfo, addr(q660->share.sysinfo), sizeof(tsysinfo)) ;
  memcpy (sseed, addr(q660->share.seed), sizeof(tseed)) ;
  unlock (q660) ;
  return LIBERR_NOERR ;
end

word lib_change_verbosity (tcontext ct, word newverb)
begin

  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return newverb ;
  lock (q660) ;
  q660->cur_verbosity = newverb ;
  unlock (q660) ;
  return newverb ;
end

void lib_send_usermessage (tcontext ct, pchar umsg)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return ;
  send_user_message (q660, umsg) ;
end

void lib_poc_received (tcontext ct, tpocmsg *poc)
begin
  pq660 q660 ;
  string47 newaddr ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return ;
  lock (q660) ;
  add_status (q660, LOGF_POCSRECV, 1) ;
  if (q660->libstate == LIBSTATE_WAIT)
    then
      begin
        if (poc->ipv6)
          then
            strcpy(newaddr, inet_ntop6 (addr(poc->new_ipv6_address))) ;
          else
            showdot(poc->new_ip_address, newaddr) ;
        if (strcmp(q660->par_register.q660id_address, newaddr))
          then
            add_status (q660, LOGF_IPCHANGE, 1) ;
        strcpy(q660->par_register.q660id_address, newaddr) ;
        q660->par_register.q660id_baseport = poc->new_base_port ;
        q660->par_register.opt_token = poc->poc_token ;
        libmsgadd (q660, LIBMSG_POCRECV, poc->log_info) ;
        q660->dynip_age = 0 ;
        unlock (q660) ; /* lib_change_state locks again */
        lib_change_state (q660, LIBSTATE_RUNWAIT, LIBERR_NOERR) ;
        return ;
      end
  unlock (q660) ;
end

enum tliberr lib_get_detstat (tcontext ct, tdetstat *detstat)
begin

  if (ct == NIL)
    then
      return LIBERR_INVCTX ;
    else
      return lib_detstat (ct, detstat) ; /* ask someone who knows */
end

enum tliberr lib_get_lcqstat (tcontext ct, tlcqstat *lcqstat)
begin

  if (ct == NIL)
    then
      return LIBERR_INVCTX ;
    else
      return lib_lcqstat (ct, lcqstat) ;
end

void lib_change_enable (tcontext ct, tdetchange *detchange)
begin

  if (ct)
    then
      lib_changeenable (ct, detchange) ;
end

enum tliberr lib_get_dpcfg (tcontext ct, tdpcfg *dpcfg)
begin

  if (ct == NIL)
    then
      return LIBERR_INVCTX ;
    else
      return lib_getdpcfg (ct, dpcfg) ;
end

void lib_msg_add (tcontext ct, word msgcode, longword dt, pchar msgsuf)
begin

  if (ct)
    then
      msgadd (ct, msgcode, dt, msgsuf, TRUE) ;
end

const tmodules modules =
   {{/*name*/"LibClient", /*ver*/VER_LIBCLIENT},     {/*name*/"LibStrucs", /*ver*/VER_LIBSTRUCS},
    {/*name*/"LibTypes", /*ver*/VER_LIBTYPES},       {/*name*/"LibMsgs", /*ver*/VER_LIBMSGS},
    {/*name*/"LibStats", /*ver*/VER_LIBSTATS},       {/*name*/"LibVerbose", /*ver*/VER_LIBVERBOSE},
    {/*name*/"LibData", /*ver*/VER_LIBDATA},         {/*name*/"LibSeed", /*ver*/VER_LIBSEED},
    {/*name*/"LibSample", /*ver*/VER_LIBSAMPLE},     {/*name*/"LibSampglob", /*ver*/VER_LIBSAMPGLOB},
    {/*name*/"LibSampcfg", /*ver*/VER_LIBSAMPCFG},   {/*name*/"LibCont", /*ver*/VER_LIBCONT},
    {/*name*/"LibCompress", /*ver*/VER_LIBCOMPRESS}, {/*name*/"LibCmds", /*ver*/VER_LIBCMDS},
    {/*name*/"LibOpaque", /*ver*/VER_LIBOPAQUE},     {/*name*/"LibLogs", /*ver*/VER_LIBLOGS},
    {/*name*/"LibFilters", /*ver*/VER_LIBFILTERS},   {/*name*/"LibDetect", /*ver*/VER_LIBDETECT},
    {/*name*/"LibArchive", /*ver*/VER_LIBARCHIVE},   {/*name*/"LibSupport", /*ver*/VER_LIBSUPPORT},
    {/*name*/"LibPOC", /*ver*/VER_LIBPOC},           {/*name*/"MemUtil", /*ver*/VER_MEMUTIL},
    {/*name*/"ReadPackets", /*ver*/VER_READPACKETS}, {/*name*/"SHA256", /*ver*/VER_SHA256},
    {/*name*/"XMLCfg", /*ver*/VER_XMLCFG},           {/*name*/"XMLSeed", /*ver*/VER_XMLSEED},
    {/*name*/"XMLSup", /*ver*/VER_XMLSUP},           {/*name*/"", /*ver*/0}} ;

pmodules lib_get_modules (void)
begin

  return (pmodules)addr(modules) ;
end

enum tliberr lib_conntiming (tcontext ct, tconntiming *conntiming, boolean setter)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return LIBERR_INVCTX ;
  if (setter)
    then
      begin
        q660->par_register.opt_conntime = conntiming->opt_conntime ;
        q660->par_register.opt_connwait = conntiming->opt_connwait ;
        q660->par_register.opt_disc_latency = conntiming->opt_disc_latency;
        q660->data_timeout = (integer) conntiming->data_timeout * 60 ;
        q660->data_timeout_retry = (integer) conntiming->data_timeout_retry * 60 ;
        q660->status_timeout = (integer) conntiming->status_timeout * 60 ;
        q660->status_timeout_retry = (integer) conntiming->status_timeout_retry * 60 ;
        q660->piu_retry = (integer) conntiming->piu_retry * 60 ;
      end
    else
      begin
        conntiming->opt_conntime = q660->par_register.opt_conntime ;
        conntiming->opt_connwait = q660->par_register.opt_connwait ;
        conntiming->opt_disc_latency = q660->par_register.opt_disc_latency ;
        conntiming->data_timeout = q660->data_timeout div 60 ;
        conntiming->data_timeout_retry = q660->data_timeout_retry div 60 ;
        conntiming->status_timeout = q660->status_timeout div 60 ;
        conntiming->status_timeout_retry = q660->status_timeout_retry div 60 ;
        conntiming->piu_retry = q660->piu_retry div 60 ;
      end
  return LIBERR_NOERR ;
end

longint lib_crccalc (pbyte p, longint len)
begin

  return gcrccalc(p, len) ;
end

enum tliberr lib_set_freeze_timer (tcontext ct, integer seconds)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return LIBERR_INVCTX ;
  lock (q660) ;
  q660->share.freeze_timer = seconds ;
//  printf ("Freeze Timer=%d\r\n", seconds) ;
  unlock (q660) ;
  return LIBERR_NOERR ;
end

enum tliberr lib_flush_data (tcontext ct)
begin
  pq660 q660 ;

  q660 = ct ;
  if (q660 == NIL)
    then
      return LIBERR_INVCTX ;
  q660->flush_all = TRUE ;
  return LIBERR_NOERR ;
end


