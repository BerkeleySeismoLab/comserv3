



/*   Lib660 client interface
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
    0 2017-06-06 rdr Created
    1 2017-06-27 rdr Add returning seed structure in lib_get_sysinfo.
    2 2018-11-14 rdr Add start_newer to tpar_register.
    3 2019-07-11 rdr Add one second callback structures.
    4 2020-02-12 rdr Add handling for network retry override.
    5 2020-02-19 rdr lib_request_status always buffer new request so only sent by
                     station thread. Likewise, in lib_send_usermessage add buffering
                     for same reason.
    6 2021-04-04 rdr Add Dust status handling.
    7 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
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
{

    lib_create_660 (ct, cfg) ;
}

enum tliberr lib_destroy_context (tcontext *ct) /* Return error if any */
{

    return lib_destroy_660 (ct) ;
}

enum tliberr lib_register (tcontext ct, tpar_register *rpar)
{
    pq660 q660 ;
    enum tliberr result ;

    q660 = ct ;

    if (q660 == NIL)
        return LIBERR_INVCTX ;

    rpar->opt_token = 0 ;
    result = lib_register_660 (q660, rpar) ;

    if (q660->share.target_state == LIBSTATE_WAIT)
        libmsgadd (q660, LIBMSG_NOIP, "") ;

    return result ;
}

enum tlibstate lib_get_state (tcontext ct, enum tliberr *err, topstat *retopstat)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL) {
        *err = LIBERR_INVCTX ;
        return LIBSTATE_IDLE ; /* not really */
    }

    *err = q660->share.liberr ;
    lock (q660) ;
    update_op_stats (q660) ;
    memcpy (retopstat, &(q660->share.opstat), sizeof(topstat)) ;
    unlock (q660) ;
    return q660->libstate ;
}

void lib_change_state (tcontext ct, enum tlibstate newstate, enum tliberr reason)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return ;

    lock (q660) ;
    q660->share.target_state = newstate ;
    q660->share.liberr = reason ;
    unlock (q660) ;
}

void lib_request_status (tcontext ct, U32 bitmap, U16 interval)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return ;

    lock (q660) ;

    if ((bitmap != q660->share.extra_status) || (interval != q660->share.status_interval)) {
        q660->share.extra_status = bitmap ;
        q660->share.status_interval = interval ;
        q660->status_change_pending = TRUE ;
    }

    unlock (q660) ;
}

enum tliberr lib_get_status (tcontext ct, enum tstype stat_type, pointer buf)
{
    pq660 q660 ;
    enum tliberr result ;

    q660 = ct ;

    if (q660 == NIL)
        return LIBERR_INVCTX ;

    lock (q660) ;

    if (q660->share.have_status & make_bitmap((U16)stat_type)) {
        result = LIBERR_NOERR ; /* assume good */

        switch (stat_type) {
        case ST_SM :
            memcpy(buf, &(q660->share.stat_sm), sizeof(tstat_sm)) ; /* Station Monitor Status */
            break ;

        case ST_GPS :
            memcpy(buf, &(q660->share.stat_gps), sizeof(tstat_gps)) ; /* GPS Status */
            break ;

        case ST_PLL :
            memcpy(buf, &(q660->share.stat_pll), sizeof(tstat_pll)) ; /* PLL Status */
            break ;

        case ST_LS :
            memcpy(buf, &(q660->share.stat_logger), sizeof(tstat_logger)) ; /* Logger Status */
            break ;

        case ST_DUST :
            memcpy(buf, &(q660->share.stat_dust), sizeof(tstat_dust)) ; /* Dust Status */
            q660->share.have_status = q660->share.have_status | make_bitmap((U16)ST_DUSTLAT) ; /* Always have this */
            break ;

        case ST_DUSTLAT :
            memcpy(buf, &(q660->share.stat_dustlat), sizeof(tstat_dustlat)) ; /* Dust newest data time */
            break ;

        default :
            result = LIBERR_INVSTAT ;
            break ;
        }
    } else
        result = LIBERR_NOSTAT ;

    unlock (q660) ;
    return result ;
}

enum tliberr lib_get_sysinfo (tcontext ct, tsysinfo *sinfo, tseed *sseed)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return LIBERR_INVCTX ;

    lock (q660) ;
    memcpy (sinfo, &(q660->share.sysinfo), sizeof(tsysinfo)) ;
    memcpy (sseed, &(q660->share.seed), sizeof(tseed)) ;
    unlock (q660) ;
    return LIBERR_NOERR ;
}

U16 lib_change_verbosity (tcontext ct, U16 newverb)
{

    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return newverb ;

    lock (q660) ;
    q660->cur_verbosity = newverb ;
    unlock (q660) ;
    return newverb ;
}

void lib_send_usermessage (tcontext ct, pchar umsg)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return ;

    strcpy(q660->share.user_message.msg, umsg) ;
    q660->share.usermessage_requested = TRUE ;
}

void lib_poc_received (tcontext ct, tpocmsg *poc)
{
    pq660 q660 ;
    string47 newaddr ;

    q660 = ct ;

    if (q660 == NIL)
        return ;

    lock (q660) ;
    add_status (q660, LOGF_POCSRECV, 1) ;

    if (q660->libstate == LIBSTATE_WAIT) {
        if (poc->ipv6)
            strcpy(newaddr, inet_ntop6 (&(poc->new_ipv6_address))) ;
        else
            showdot(poc->new_ip_address, newaddr) ;

        if (strcmp(q660->par_register.q660id_address, newaddr))
            add_status (q660, LOGF_IPCHANGE, 1) ;

        strcpy(q660->par_register.q660id_address, newaddr) ;
        q660->par_register.q660id_baseport = poc->new_base_port ;
        q660->par_register.opt_token = poc->poc_token ;
        libmsgadd (q660, LIBMSG_POCRECV, poc->log_info) ;
        q660->dynip_age = 0 ;
        unlock (q660) ; /* lib_change_state locks again */
        lib_change_state (q660, LIBSTATE_RUNWAIT, LIBERR_NOERR) ;
        return ;
    }

    unlock (q660) ;
}

enum tliberr lib_get_detstat (tcontext ct, tdetstat *detstat)
{

    if (ct == NIL)
        return LIBERR_INVCTX ;
    else
        return lib_detstat (ct, detstat) ; /* ask someone who knows */
}

enum tliberr lib_get_lcqstat (tcontext ct, tlcqstat *lcqstat)
{

    if (ct == NIL)
        return LIBERR_INVCTX ;
    else
        return lib_lcqstat (ct, lcqstat) ;
}

void lib_change_enable (tcontext ct, tdetchange *detchange)
{

    if (ct)
        lib_changeenable (ct, detchange) ;
}

enum tliberr lib_get_dpcfg (tcontext ct, tdpcfg *dpcfg)
{

    if (ct == NIL)
        return LIBERR_INVCTX ;
    else
        return lib_getdpcfg (ct, dpcfg) ;
}

void lib_msg_add (tcontext ct, U16 msgcode, U32 dt, pchar msgsuf)
{

    if (ct)
        msgadd (ct, msgcode, dt, msgsuf, TRUE) ;
}

const tmodules modules = {
    {/*name*/"LibClient", /*ver*/VER_LIBCLIENT},     {/*name*/"LibStrucs", /*ver*/VER_LIBSTRUCS},
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
    {/*name*/"XMLSup", /*ver*/VER_XMLSUP},           {/*name*/"", /*ver*/0}
} ;

pmodules lib_get_modules (void)
{

    return (pmodules)&(modules) ;
}

enum tliberr lib_conntiming (tcontext ct, tconntiming *conntiming, BOOLEAN setter)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return LIBERR_INVCTX ;

    if (setter) {
        q660->par_register.opt_conntime = conntiming->opt_conntime ;
        q660->par_register.opt_connwait = conntiming->opt_connwait ;
        q660->par_register.opt_disc_latency = conntiming->opt_disc_latency;
        q660->data_timeout = (int) conntiming->data_timeout * 60 ;
        q660->data_timeout_retry = (int) conntiming->data_timeout_retry * 60 ;
        q660->status_timeout = (int) conntiming->status_timeout * 60 ;
        q660->status_timeout_retry = (int) conntiming->status_timeout_retry * 60 ;
        q660->piu_retry = (int) conntiming->piu_retry * 60 ;
        q660->par_register.opt_nro = conntiming->opt_nro ;
    } else {
        conntiming->opt_conntime = q660->par_register.opt_conntime ;
        conntiming->opt_connwait = q660->par_register.opt_connwait ;
        conntiming->opt_disc_latency = q660->par_register.opt_disc_latency ;
        conntiming->data_timeout = q660->data_timeout / 60 ;
        conntiming->data_timeout_retry = q660->data_timeout_retry / 60 ;
        conntiming->status_timeout = q660->status_timeout / 60 ;
        conntiming->status_timeout_retry = q660->status_timeout_retry / 60 ;
        conntiming->piu_retry = q660->piu_retry / 60 ;
        conntiming->opt_nro = q660->par_register.opt_nro ;
    }

    return LIBERR_NOERR ;
}

I32 lib_crccalc (PU8 p, I32 len)
{

    return gcrccalc(p, len) ;
}

enum tliberr lib_set_freeze_timer (tcontext ct, int seconds)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return LIBERR_INVCTX ;

    lock (q660) ;
    q660->share.freeze_timer = seconds ;
//  printf ("Freeze Timer=%d\r\n", seconds) ;
    unlock (q660) ;
    return LIBERR_NOERR ;
}

enum tliberr lib_flush_data (tcontext ct)
{
    pq660 q660 ;

    q660 = ct ;

    if (q660 == NIL)
        return LIBERR_INVCTX ;

    q660->flush_all = TRUE ;
    return LIBERR_NOERR ;
}


