



/*   Lib660 Statistics
     Copyright 2017-2019 by
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
    0 2017-06-11 rdr Created
    1 2017-07-05 rdr Add setting active_digis and pgagains in opstat.
    2 2019-07-11 rdr Add support for one second callback.
    3 2019-10-24 rdr Set sample_count for one second callback to 1.
    4 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libtypes.h"
#include "libstrucs.h"
#include "libclient.h"
#include "libsampglob.h"
#include "libsampcfg.h"
#include "libmsgs.h"
#include "libsupport.h"
#include "libsample.h"
#include "libcompress.h"

static void process_dpstat (pq660 q660, plcq q, I32 val)
{
#define DPSTAT_QUALITY 100
    int used ;
    tcom_packet *pcom ;
    string95 s ;
    string31 s1 ;
    string7 s2 ;

    pcom = q->com ;
    q->data_written = FALSE ;

    if ((q->last_timetag > 1) && (fabs(q660->dpstat_timestamp - q->last_timetag - q->gap_offset) > q->gap_secs)) {
        if (q660->cur_verbosity & VERB_LOGEXTRA) {
            sprintf(s, "%s %s", seed2string(q->location, q->seedname, s2),
                    realtostr(q660->dpstat_timestamp - q->last_timetag - q->gap_offset, 6, s1)) ;
            libdatamsg (q660, LIBMSG_TIMEDISC, s) ;
        }

        flush_lcq (q660, q, pcom) ; /* gap in the data */
    }

    q->last_timetag = q660->dpstat_timestamp ;

    if (q->timetag == 0) {
        q->timetag = q660->dpstat_timestamp ;
        q->timequal = DPSTAT_QUALITY ;
        pcom->time_mark_sample = pcom->peek_total + pcom->next_compressed_sample ;
    }

    if (q660->par_create.call_secdata) {
        q660->onesec_call.total_size = sizeof(tonesec_call) - ((MAX_RATE - 1) * sizeof(I32)) ;
        q660->onesec_call.context = q660 ;
        memcpy(&(q660->onesec_call.station_name), &(q660->station_ident), sizeof(string9)) ;
        strcpy(q660->onesec_call.location, q->slocation) ;
        strcpy(q660->onesec_call.channel, q->sseedname) ;
        q660->onesec_call.timestamp = q660->dpstat_timestamp ;
        q660->onesec_call.qual_perc = DPSTAT_QUALITY ;
        q660->onesec_call.rate = q->rate ;
        q660->onesec_call.activity_flags = 0 ;
        q660->onesec_call.sample_count = 1 ;

        if (q->gen_on)
            q660->onesec_call.activity_flags = q660->onesec_call.activity_flags | SAF_EVENT_IN_PROGRESS ;

        if (q->cal_on)
            q660->onesec_call.activity_flags = q660->onesec_call.activity_flags | SAF_CAL_IN_PROGRESS ;

        if (q->timequal >= Q_OFF)
            q660->onesec_call.io_flags = SIF_LOCKED ;
        else
            q660->onesec_call.io_flags = 0 ;

        if (q->timequal < Q_LOW)
            q660->onesec_call.data_quality_flags = SQF_QUESTIONABLE_TIMETAG ;
        else
            q660->onesec_call.data_quality_flags = 0 ;

        q660->onesec_call.src_gen = q->gen_src ;
        q660->onesec_call.src_subchan = q->sub_field ;
        q660->onesec_call.samples[0] = val ;
        q660->par_create.call_secdata (&(q660->onesec_call)) ;
    }

    pcom->peeks[pcom->next_in] = val ;
    pcom->next_in = (pcom->next_in + 1) & (PEEKELEMS - 1) ;
    (pcom->peek_total)++ ;

    if (pcom->peek_total < MAXSAMPPERWORD)
        return ;

    used = compress_block (q660, q, pcom) ;
    pcom->next_compressed_sample = pcom->next_compressed_sample + used ;

    if (pcom->frame >= (pcom->maxframes + 1))
        finish_record (q660, q, pcom) ;
}

void add_status (pq660 q660, enum tlogfld acctype, U32 count)
{

    q660->share.accmstats[acctype].accum = q660->share.accmstats[acctype].accum + (count) ;
    q660->share.accmstats[acctype].accum_ds = q660->share.accmstats[acctype].accum_ds + (count) ;
}

void lib_stats_timer (pq660 q660)
{
    enum tlogfld acctype ;
    int minute, last_minute, comeff_valids ;
    I32 total, val ;
    taccmstat *paccm ;

    lock (q660) ;

    if (q660->libstate == LIBSTATE_RUN) {
        add_status (q660, LOGF_COMMDUTY, 10) ;
        q660->share.accmstats[LOGF_COMMEFF].accum_ds = 1000 ;
    } else
        q660->share.accmstats[LOGF_COMMEFF].accum_ds = 0 ;

    for (acctype =AC_FIRST ; acctype <= AC_LAST ; acctype++) {
        paccm = &(q660->share.accmstats[acctype]) ;

        if (paccm->ds_lcq) {
            switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
                val = paccm->accum_ds / 10 ;
                break ;

            case LOGF_COMMDUTY :
                val = (paccm->accum_ds * 1000) / 10 ;
                break ;

            case LOGF_THRPUT :
                val = (paccm->accum_ds * 100) / 10 ;
                break ;

            default :
                val = paccm->accum_ds ;
                break ;
            }

            unlock (q660) ;
            process_dpstat (q660, paccm->ds_lcq, val) ;
            lock (q660) ;
        }

        paccm->accum_ds = 0 ;
    }

    unlock (q660) ;

    if ((q660->data_latency_lcq) && (q660->saved_data_timetag > 1))
        process_dpstat (q660, q660->data_latency_lcq, now () - q660->saved_data_timetag + 0.5) ;

    if ((q660->status_latency_lcq) && (q660->last_status_received > 1))
        process_dpstat (q660, q660->status_latency_lcq, now () - q660->last_status_received + 0.5) ;

    (q660->minute_counter)++ ;

    if (q660->minute_counter >= 6)
        q660->minute_counter = 0 ;
    else
        return ; /* not a new minute yet */

    lock (q660) ;

    if (q660->share.accmstats[LOGF_COMMDUTY].accum >= 5)
        q660->share.accmstats[LOGF_COMMEFF].accum = 1000 ; /* was connected */
    else
        q660->share.accmstats[LOGF_COMMEFF].accum = INVALID_ENTRY ;

    last_minute = q660->share.stat_minutes ;
    q660->share.stat_minutes = (q660->share.stat_minutes + 1) % 60 ;

    for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++) {
        paccm = &(q660->share.accmstats[acctype]) ;
        paccm->minutes[last_minute] = paccm->accum ;

        if (q660->share.stat_minutes == 0) {
            if (acctype != LOGF_COMMEFF) {
                /* new hour, update current hour */
                total = 0 ;

                for (minute = 0 ; minute <= 59 ; minute++)
                    total = total + paccm->minutes[minute] ;

                paccm->hours[q660->share.stat_hours] = total ;
            } else {
                /* same, but take into account invalid entries */
                comeff_valids = 0 ;
                total = 0 ;

                for (minute = 0 ; minute <= 59 ; minute++)
                    if (paccm->minutes[minute] != INVALID_ENTRY) {
                        (comeff_valids)++ ;
                        total = total + paccm->minutes[minute] ;
                    }

                if (comeff_valids > 0)
                    total = lib_round((double)total / (comeff_valids / 60.0)) ;
                else
                    total = INVALID_ENTRY ;

                paccm->hours[q660->share.stat_hours] = total ;
            }
        }

        paccm->accum = 0 ; /* start counting the new minute as zero */
    }

    if (q660->share.stat_minutes == 0) {
        /* move to next hour */
        q660->share.stat_hours = (q660->share.stat_hours + 1) % 24 ;

        for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++) {
            paccm = &(q660->share.accmstats[acctype]) ;
            paccm->hours[q660->share.stat_hours] = 0 ; /* start counting the new hour as zero */
        }
    }

    (q660->share.total_minutes)++ ;
    unlock (q660) ;

    if (q660->par_create.call_state) {
        q660->state_call.context = q660 ;
        q660->state_call.state_type = ST_OPSTAT ;
        q660->state_call.subtype = 0 ;
        memcpy(&(q660->state_call.station_name), &(q660->station_ident), sizeof(string9)) ;
        q660->state_call.info = 0 ;
        q660->state_call.subtype = 0 ;
        q660->par_create.call_state (&(q660->state_call)) ;
    }
}

void update_gps_stats (pq660 q660)
{
    q660->share.opstat.gps_pwr = q660->share.stat_sm.gps_pwr ;
    q660->share.opstat.gps_fix = q660->share.stat_sm.gps_fix ;
    q660->share.opstat.gps_lat = q660->share.stat_sm.latitude ;
    q660->share.opstat.gps_long = q660->share.stat_sm.longitude ;
    q660->share.opstat.gps_elev = q660->share.stat_sm.elevation ;

    if (q660->share.stat_gps.last_good) {
        q660->share.opstat.gps_age = lib_round(now()) - q660->share.stat_gps.last_good ;

        if (q660->share.opstat.gps_age < 0)
            q660->share.opstat.gps_age = 0 ;
    } else
        q660->share.opstat.gps_age = -1 ;
}

void update_op_stats (pq660 q660)
{
    enum tlogfld acctype ;
    int lastminute, minute, hour, valids, comeff_valids ;
    int i, good, value ;
    I32 total ;
    taccmstat *paccm ;
    topstat *pops ;

    pops = &(q660->share.opstat) ;

    if (q660->saved_data_timetag > 1)
        pops->data_latency = now () - q660->saved_data_timetag + 0.5 ;
    else
        pops->data_latency = INVALID_LATENCY ;

    if (q660->last_status_received != 0)
        pops->status_latency = now () - q660->last_status_received + 0.5 ;
    else
        pops->status_latency = INVALID_LATENCY ;

    memcpy(&(pops->station_name), &(q660->station_ident), sizeof(string9)) ;
    pops->station_prio = q660->par_create.q660id_priority ;

    if (q660->share.sysinfo.prop_tag[0]) {
        good = sscanf (q660->share.sysinfo.prop_tag, "%d", &(value)) ;

        if (good == 1)
            q660->share.opstat.station_tag = value ;
        else
            q660->share.opstat.station_tag = 0 ;
    } else
        q660->share.opstat.station_tag = 0 ;

    if (! stringtot64 (q660->share.sysinfo.fe_ser, &(pops->station_serial)))
        memclr(&(pops->station_serial), sizeof(t64)) ;

    pops->station_reboot = q660->share.sysinfo.boot ;
    pops->calibration_errors = 0 ;
    lastminute = q660->share.stat_minutes - 1 ;

    if (lastminute < 0)
        lastminute = 59 ;

    for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++) {
        paccm = &(q660->share.accmstats[acctype]) ;

        if (q660->share.total_minutes >= 1)
            switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
                pops->accstats[acctype][AD_MINUTE] = paccm->minutes[lastminute] / 60 ;
                break ;

            case LOGF_COMMDUTY :
                pops->accstats[acctype][AD_MINUTE] = (paccm->minutes[lastminute] * 1000) / 60 ;
                break ;

            case LOGF_THRPUT :
                pops->accstats[acctype][AD_MINUTE] = (paccm->minutes[lastminute] * 100) / 60 ;
                break ;

            default :
                pops->accstats[acctype][AD_MINUTE] = paccm->minutes[lastminute] ;
                break ;
            } else
            pops->accstats[acctype][AD_MINUTE] = INVALID_ENTRY ;

        valids = q660->share.total_minutes ;

        if (valids > 60)
            valids = 60 ;

        total = 0 ;
        comeff_valids = 0 ;

        for (minute = 0 ; minute <= valids - 1 ; minute++) {
            if (acctype == LOGF_COMMEFF) {
                if (paccm->minutes[minute] != INVALID_ENTRY) {
                    (comeff_valids)++ ;
                    total = total + paccm->minutes[minute] ;
                }
            } else
                total = total + paccm->minutes[minute] ;
        }

        if (valids == 0)
            total = INVALID_ENTRY ;

        pops->minutes_of_stats = valids ;

        if (total == INVALID_ENTRY)
            pops->accstats[acctype][AD_HOUR] = total ;
        else
            switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
                pops->accstats[acctype][AD_HOUR] = total / (60 * valids) ;
                break ;

            case LOGF_COMMDUTY :
                pops->accstats[acctype][AD_HOUR] = (total * 1000) / (60 * valids) ;
                break ;

            case LOGF_THRPUT :
                pops->accstats[acctype][AD_HOUR] = (total * 100) / (60 * valids) ;
                break ;

            case LOGF_COMMEFF :
                if (comeff_valids == 0) {
                    pops->accstats[acctype][AD_HOUR] = INVALID_ENTRY ;
                    total = INVALID_ENTRY ;
                } else {
                    pops->accstats[acctype][AD_HOUR] = total / comeff_valids ;
                    total = lib_round((double)total / (comeff_valids / 60.0)) ;
                }

                break ;

            default :
                pops->accstats[acctype][AD_HOUR] = total ;
                break ;
            }

        valids = q660->share.total_minutes / 60 ; /* hours */

        if (valids >= 24)
            valids = 24 ;
        else
            total = 0 ;

        comeff_valids = 0 ;

        for (hour = 0 ; hour <= valids - 1 ; hour++) {
            if (acctype == LOGF_COMMEFF) {
                if ((hour == q660->share.stat_hours) && (total != INVALID_ENTRY))
                    (comeff_valids)++ ;
                else if ((hour != q660->share.stat_hours) && (paccm->hours[hour] != INVALID_ENTRY)) {
                    (comeff_valids)++ ;
                    total = total + paccm->hours[hour] ;
                }
            } else
                total = total + paccm->hours[hour] ;
        }

        if (valids == 0)
            total = INVALID_ENTRY ; /* need at least one hour to extrapolate */

        pops->hours_of_stats = valids ;

        if (total == INVALID_ENTRY)
            pops->accstats[acctype][AD_DAY] = total ;
        else
            switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
                pops->accstats[acctype][AD_DAY] = total / (3600 * valids) ;
                break ;

            case LOGF_COMMDUTY :
                pops->accstats[acctype][AD_DAY] = (total * 1000) / (3600 * valids) ;
                break ;

            case LOGF_THRPUT :
                pops->accstats[acctype][AD_DAY] = (total * 100) / (3600 * valids) ;
                break ;

            case LOGF_COMMEFF :
                if (comeff_valids == 0)
                    pops->accstats[acctype][AD_DAY] = INVALID_ENTRY ;
                else
                    pops->accstats[acctype][AD_DAY] = total / (60 * comeff_valids) ;

                break ;

            default :
                pops->accstats[acctype][AD_DAY] = total ;
                break ;
            }

        pops->active_digis = 0 ;
        memclr (&(pops->pgagains), sizeof(pops->pgagains)) ;

        for (i = 0 ; i < TOTAL_CHANNELS ; i++)
            if (q660->share.digis[i].freqmap) {
                pops->active_digis = pops->active_digis | (1 << i) ;

                if (i < SENS_CHANNELS)
                    pops->pgagains[i] = q660->share.digis[i].pgagain ;
            }
    }
}
