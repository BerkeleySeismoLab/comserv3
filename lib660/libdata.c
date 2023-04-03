



/*   Lib660 Data Packet Processing
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
    0 2017-06-08 rdr Created
    1 2018-06-26 rdr Remove Ext GPS current, change Ext GPS voltage to antenna volts.
    2 2018-05-14 rdr For IB_CALEVT handling set calstat in LCQS.
    3 2019-06-26 rdr Add GPS Data Stream processing.
    4 2020-01-22 rdr Remove process_low_latency.
    5 2020-02-05 rdr Add calling call_eos (if configured) when IB_EOS received.
    6 2020-02-13 rdr If get IB_EOS and we are waiting to disconnect, do it.
   4b 2020-05-28 jms prevent runaway string on phase error
   5b 2021-01-06 jms do same in more places. remove spurious mutex lock.
    6 2021-02-03 rdr Add new GPS engineering fields.
    7 2021-04-04 rdr Add Dust handling.
    8 2021-07-13 rdr Add Dust Devices to SOURCES lists.
    9 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
   10 2022-03-01 jms use dust embedded timestamp
*/
#include "libdata.h"
#include "libclient.h"
#include "libsampglob.h"
#include "libsample.h"
#include "libmsgs.h"
#include "libcompress.h"
#include "libcont.h"
#include "libstats.h"
#include "libopaque.h"
#include "liblogs.h"
#include "libcmds.h"
#include "libsupport.h"
#include "libdataserv.h"

#define JUMP_THRESH 250 /* Jump Threshold in usec */

typedef char string25[26] ;
typedef string25 tdigph[4] ;
typedef string25 tdigphr[4] ;
typedef string25 tgpsph[7] ;
typedef string25 tgpscold[4] ;
typedef string7 tsources[SRC_COUNT] ;

const tdigph DIGPH = {"Start", "Timemark Wait", "Internal Marks", "External Marks"} ;
const tdigphr DIGPHR = {"Normal", "Phase out of Range", "GPS Time Received", "Command Received"} ;
const tgpsph GPSPH = {"Power Off - GPS Lock", "Power Off - PLL Lock", "Power Off - Max. Time",
                      "Power Off - Command", "Coldstart - ", "Power On - Automatic",
                      "Power On - Command"
                     } ;
const tgpscold GPSCOLD = {"Command Received", "Reception Timeout",
                          "GPS & RTC out of phase ", "Large time jump"
                         } ;
const tsources SOURCES = {"[SS] ", "[WS] ", "[BE] ", "[I1] ", "[I2] ", "[I3] ", "[I4] ",
                          "[X1] ", "[X2] ", "[X3] ", "[X4] ", "[D1] ", "[D2] ", "[D3] ",
                          "[D4] ", "[D5] ", "[D6] ", "[D7] ", "[D8] "
                         } ;

void process_data (pq660 q660, PU8 p, int lth)
{
    BOOLEAN seqgap_occurred ;
    BOOLEAN update_ok, cal_running ;
    U16 w ;
    int i, j, loops, hr ;
    PU8 pstart, p2 ;
    plcq q ;
    U32 sec ;
    I32 usec ;
    I32 diff ;
    I32 spread ;
    double t ;
    string95 s ;
    string31 s1, s2 ;
    enum tgdsrc gds ;
    PNTRINT bufend ;
    int dlth ;
    tsohblk soh ;
    tengblk eng ;
    tgpsblk gps ;
    tdustblk dust ;
    tuser_message umsg ;
    U8 b1, b2, dev ;
    PU8 starts[100] ; /* Just used for debugging */

    seqgap_occurred = FALSE ;
    loops = 0 ;
    bufend = (PNTRINT)p + (PNTRINT)lth ;
    q660->data_timer = 0 ;
    sec = loadlongword (&(p)) ;
    spread = seqspread(sec, q660->dt_data_sequence) ;

    if ((sec != q660->dt_data_sequence) && (sec != (q660->dt_data_sequence + 1))) {
        if (q660->dt_data_sequence == 0)
            libdatamsg(q660, LIBMSG_SEQBEG, jul_string(sec, s1)) ; /* no continutity */
        else {
            if ((q660->lastseq == 0) && (spread < 0) && (abs(spread) < MAXSPREAD)) {
                sprintf (s, "%s to %s", jul_string(q660->dt_data_sequence, s1),
                         jul_string(sec, s2)) ;
                libdatamsg (q660, LIBMSG_SEQOVER, s) ;
            } else {
                if (spread > q660->max_spread) {
                    sprintf (s, "%d, %s to %s", (int)spread,
                             jul_string(q660->dt_data_sequence, s1), jul_string(sec, s2)) ;
                    libdatamsg (q660, LIBMSG_SEQGAP, s) ;
                    add_status (q660, LOGF_DATA_GAP, 1) ;
                    (q660->share.opstat.totalgaps)++ ;
                }

                seqgap_occurred = TRUE ;
            }
        }

        q660->lasttime = 0 ;
        q660->data_timetag = 0 ; /* can't use any data until get a time mark blockette */
    } else if (q660->lastseq == 0) {
        sprintf (s, "%s to %s", jul_string(q660->dt_data_sequence, s1), jul_string(sec, s2)) ;
        libdatamsg (q660, LIBMSG_SEQRESUME, s) ;
    } else if (spread) {
        /* Check for missed timing blockette */
        gds = (enum tgdsrc)*p ; /* get channel */

        if (gds != GDS_TIM) {
            /* Missed, need to wait for next timing blockette */
            if (spread > q660->max_spread) {
                sprintf (s, "1, %s to %s", jul_string(q660->dt_data_sequence, s1),
                         jul_string(sec, s2)) ;
                libdatamsg (q660, LIBMSG_SEQGAP, s) ;
                add_status (q660, LOGF_DATA_GAP, 1) ;
                (q660->share.opstat.totalgaps)++ ;
            } ;

            seqgap_occurred = TRUE ;

            q660->lasttime = 0 ;

            q660->data_timetag = 0 ; /* can't use any data until get a time mark blockette */
        }
    }

    q660->dt_data_sequence = sec ; /* set global data record sequence number */
    q660->lastseq = q660->dt_data_sequence ;

    while ((loops < 100) && ((PNTRINT)p < bufend)) {
        starts[loops] = p ;
        (loops)++ ;
        pstart = p ;
        gds = (enum tgdsrc)*p ;
        p2 = (PU8)((PNTRINT)p + 3) ;
        dlth = ((U16)*p2 + 1) << 2 ;
        q660->within_second = TRUE ; /* new data for second */

        switch (gds) {
        case GDS_TIM :
            dlth = GDS_TIM_LENGTH ;

            if (q660->first_data) {
                q660->first_data = FALSE ;
                purge_continuity (q660) ;
            } ;

            update_ok = TRUE ;

            loadtimehdr (&(p), &(q660->time_block)) ;

            q660->data_qual = q660->time_block.clock_qual ;

            usec = q660->time_block.usec_offset ;

            t = sec + (usec / 1000000.0) ; /* Add in offset */

            if (q660->par_create.call_state) {
                q660->state_call.context = q660 ;
                q660->state_call.state_type = ST_TICK ;
                q660->state_call.subtype = 0 ;
                memcpy(&(q660->state_call.station_name), &(q660->station_ident), sizeof(string9)) ;
                q660->state_call.info = sec ;
                q660->state_call.subtype = usec ;
                q660->par_create.call_state (&(q660->state_call)) ;
            }

            if (q660->data_timetag < 1) {
                q660->data_timetag = t ;
                q660->saved_data_timetag = t ;

                if (q660->par_register.opt_client_mode & LMODE_BSL)
                {
                    /* We now have a reference timetag for a data packet. */
                    /* Update the par_register.opt_start to OST_LAST for any */
                    /* future reconnections to the Q8. */
                    q660->par_register.opt_start = OST_LAST;
                }
                write_ocf (q660) ;

            } else {
                if ((t - q660->data_timetag - 1) > 0.5)
                    add_status (q660, LOGF_MISSDATA, lib_round(t - q660->data_timetag - 1)) ;

                q660->data_timetag = t ; /* we are running */
                q660->saved_data_timetag = t ;
                i = lib_round(t) % 86400 ; /* Get within a day */

                if ((i == 0) || (i == 43200))
                    write_ocf (q660) ; /* Write at 00:00 and 12:00 */
            }

            q660->share.opstat.last_data_time = lib_round(q660->data_timetag) ;

            if (q660->lasttime) {
                if (fabs(t - q660->lasttime - 1.0) > 0.5) {
                    sprintf (s1, "%.7g", t - q660->lasttime - 1.0) ; /* conv if needed */

                    if (! seqgap_occurred) {
                        strcpy (s, s1) ;
                        strcat (s, " seconds") ;
                        libdatamsg(q660, LIBMSG_TIMEJMP, s) ;
                        log_clock (q660, CE_JUMP, s1) ; /* any jump that big is logged! */
                    }

                    update_ok = FALSE ;
                } else {
                    diff = lib_round((t - q660->lasttime - 1.0) * 1E6) ;

                    if (abs(diff) >= 10) {
                        sprintf (s, "%d usec", (int)diff) ;
                        libdatamsg(q660, LIBMSG_TIMEJMP, s) ;
                    }

                    if (abs(diff) >= JUMP_THRESH) {
                        sprintf (s1, "%.7g", t - q660->lasttime - 1.0) ; /* conv if needed */
                        log_clock (q660, CE_JUMP, s1) ;
                        update_ok = FALSE ;
                    }
                }
            }

            q660->lasttime = t ;
            add_status (q660, LOGF_THRPUT, 1) ;
            (q660->except_count)++ ;
            hr = (lib_round(t) % 86400) / 3600 ; /* hours */

            if ((! q660->daily_done) && (hr == 0)) {
                q660->daily_done = TRUE ;
                update_ok = FALSE ; /* don't it again */
                log_clock (q660, CE_DAILY, "") ;
            } else if (hr > 0)
                q660->daily_done = FALSE ;

            if (update_ok)
                if (q660->last_update < q660->data_timetag) {
                    if (q660->data_qual != q660->last_data_qual)
                        log_clock (q660, CE_VALID, "") ;

                    q660->last_data_qual = q660->data_qual ;
                } ;

            q = q660->timdispatch[TIMF_PH] ;

            if (q)
                process_one (q660, q, usec) ;

            q = q660->timdispatch[TIMF_CQP] ;

            if (q)
                process_one (q660, q, q660->data_qual) ;

            q = q660->timdispatch[TIMF_CLM] ;

            if (q)
                process_one (q660, q, q660->time_block.since_loss) ;

            break ;

        case GDS_MD :
            process_compressed (q660, p, 0, sec) ;
            break ;

        case GDS_CM :
            process_compressed (q660, p, CAL_CHANNEL, sec) ;
            break ;

        case GDS_AC :
            process_compressed (q660, p, MAIN_CHANNELS, sec) ;
            break ;

        case GDS_SOH :
            loadsoh (&(p), &(soh)) ;
            q = q660->sohdispatch[SOHF_ANTCUR] ;

            if (q)
                process_one (q660, q, soh.ant_cur) ;

            q = q660->sohdispatch[SOHF_SENSACUR] ;

            if (q)
                process_one (q660, q, soh.sensor_cur[0]) ;

            q = q660->sohdispatch[SOHF_SENSBCUR] ;

            if (q)
                process_one (q660, q, soh.sensor_cur[1]) ;

            for (i = 0 ; i <= BOOM_COUNT - 1 ; i++) {
                q = q660->sohdispatch[(enum tsohfld)((int)SOHF_BOOM1 + i)] ;

                if (q)
                    process_one (q660, q, sex(soh.booms[i])) ;
            }

            q = q660->sohdispatch[SOHF_SYSTMP] ;

            if (q)
                process_one (q660, q, soh.sys_temp) ;

            q = q660->sohdispatch[SOHF_HUMIDITY] ;

            if (q)
                process_one (q660, q, soh.humidity) ;

            q = q660->sohdispatch[SOHF_INPVOLT] ;

            if (q)
                process_one (q660, q, soh.input_volts) ;

            q = q660->sohdispatch[SOHF_VCOCTRL] ;

            if (q)
                process_one (q660, q, sex(soh.vco_control)) ;

            q = q660->sohdispatch[SOHF_NEGAN] ;

            if (q)
                process_one (q660, q, soh.neg_analog) ;

            q = q660->sohdispatch[SOHF_ISODC] ;

            if (q)
                process_one (q660, q, soh.iso_dc) ;

            q = q660->sohdispatch[SOHF_GPIN1] ;

            if (q)
                process_one (q660, q, soh.gpio1) ;

            q = q660->sohdispatch[SOHF_GPIN2] ;

            if (q)
                process_one (q660, q, soh.gpio2) ;

            q = q660->sohdispatch[SOHF_SYSCUR] ;

            if (q)
                process_one (q660, q, soh.sys_cur) ;

            q = q660->sohdispatch[SOHF_UPSVOLT] ;

            if (q)
                process_one (q660, q, soh.ups_volts) ;

            q = q660->sohdispatch[SOHF_ANTVOLT] ;

            if (q)
                process_one (q660, q, soh.ant_volts) ;

            q = q660->sohdispatch[SOHF_GPOUT] ;

            if (q) {
                if (soh.flags & SMF_OPTO)
                    process_one (q660, q, 1) ;
                else
                    process_one (q660, q, 0) ;
            }

            q = q660->sohdispatch[SOHF_PKTPERC] ;

            if (q)
                process_one (q660, q, soh.pkt_buf) ;

            break ;

        case GDS_ENG :
            loadeng (&(p), &(eng)) ;
            q = q660->engdispatch[ENGF_GPSSENS] ;

            if (q)
                process_one (q660, q, eng.gps_sens) ;

            q = q660->engdispatch[ENGF_GPSCTRL] ;

            if (q)
                process_one (q660, q, eng.gps_ctrl) ;

            q = q660->engdispatch[ENGF_SIOCTRLA] ;

            if (q)
                process_one (q660, q, eng.sensa_dig) ;

            q = q660->engdispatch[ENGF_SIOCTRLB] ;

            if (q)
                process_one (q660, q, eng.sensb_dig) ;

            q = q660->engdispatch[ENGF_SENSIO] ;

            if (q)
                process_one (q660, q, eng.sens_serial) ;

            q = q660->engdispatch[ENGF_MISCIO] ;

            if (q)
                process_one (q660, q, eng.misc_io) ;

            q = q660->engdispatch[ENGF_DUSTIO] ;

            if (q)
                process_one (q660, q, eng.dust_io) ;

            q = q660->engdispatch[ENGF_GPSHAL] ;

            if (q)
                process_one (q660, q, eng.gps_hal) ;

            q = q660->engdispatch[ENGF_GPSSTATE] ;

            if (q)
                process_one (q660, q, eng.gps_state) ;

            break ;

        case GDS_GPS :
            loadgps (&(p), &(gps)) ;
            q = q660->gpsdispatch[GPSF_USED] ;

            if (q)
                process_one (q660, q, gps.sat_used) ;

            q = q660->gpsdispatch[GPSF_FIXTYPE] ;

            if (q)
                process_one (q660, q, gps.fix_type) ;

            q = q660->gpsdispatch[GPSF_LAT] ;

            if (q)
                process_one (q660, q, gps.lat_udeg) ;

            q = q660->gpsdispatch[GPSF_LON] ;

            if (q)
                process_one (q660, q, gps.lon_udeg) ;

            q = q660->gpsdispatch[GPSF_ELEV] ;

            if (q)
                process_one (q660, q, gps.elev_dm) ;

            break ;

        case GDS_DUST :
            loaddust (&(p), &(dust)) ;
            dev = dust.flags_slot & 7 ; /* device number */
            lock (q660) ;

            if (dust.time > q660->share.stat_dustlat[dev])
                q660->share.stat_dustlat[dev] = dust.time ; /* newest time for this device */

            unlock (q660) ;

            if (dust.flags_slot & NOT_TIME_SERIES)
                break ; /* Unknown format */

            for (i = 0 ; i < DUST_CHANNELS ; i++)
                if ((dust.map >> (i * 2)) & 3) {
                    /* Have data on this channel */
                    q = q660->dustdispatch[(dev << 4) | i] ;

                    if (q) 
                    {
                        /* use the embedded mesh network timestamp to adjust end-user stamp */
                        t = q660->data_timetag - dust.time ;
                        if (fabs(t - q->delay) > 0.1) /* deetect a significant change in time */
                        {
                            flush_lcq (q660, q, q->com) ;
                            q->delay = t ;
                        }
                        process_one (q660, q, dust.samples[i]) ;
                    }
                }

            break ;

        default :
            switch ((U8)gds) {
            case IB_GPSPH :
                loadbyte (&(p)) ;
                b1 = loadbyte (&(p)) ;
                b2 = loadbyte (&(p)) ;
                strcpy (s, (pchar)&(GPSPH[b1])) ;

                if (b1 == GPS_COLD)
                    strcat (s, (pchar)&(GPSCOLD[b2])) ;

                libdatamsg(q660, LIBMSG_GPSSTATUS, s) ;
                dlth = 4 ;
                break ;

            case IB_DIGPH :
                loadbyte (&(p)) ;
                b1 = loadbyte (&(p)) ;
                b2 = loadbyte (&(p)) ;
                strcpy (s, (pchar)&(DIGPH[b1])) ;

                if (b2) {
                    strcat (s, ", Reason=") ;
                    strcat (s, (pchar)&(DIGPHR[b2])) ;
                }

                libdatamsg(q660, LIBMSG_DIGPHASE, s) ;
                dlth = 4 ;
                break ;

            case IB_LEAP :
                libdatamsg(q660, LIBMSG_LEAPDET, "") ;
                dlth = 4 ;
                break ;

            case IB_CALEVT :
                loadbyte (&(p)) ;
                loadbyte (&(p)) ;
                w = loadword (&(p)) ;

                for (i = 0 ; i < SENS_CHANNELS ; i++) {
                    if ((w & (3 << (i << 1)))) {
                        cal_running = TRUE ;
                        dlth = 42 ;
                    } else
                        cal_running = FALSE ;

                    for (j = 0 ; j < FREQS ; j++) {
                        q = q660->mdispatch[i][j] ;

                        if (q)
                            q->calstat = cal_running ;
                    }
                }

                dlth = 4 ;
                break ;

            case IB_DRIFT :
                loadbyte (&(p)) ;
                loadbyte (&(p)) ;
                loadword (&(p)) ;
                t = loaddouble (&(p)) ;
                sprintf (s, "%.7g Seconds", t) ;
                libdatamsg (q660, LIBMSG_PHASERANGE, s) ;
                dlth = 12 ;
                break ;

            case IB_CAL_START :
                log_cal (q660, p, TRUE) ;
                break ;

            case IB_CAL_ABORT :
                log_cal (q660, p, FALSE) ;
                dlth = 4 ;
                break ;

            case IB_UMSG :
                loadumsg (&(p), &(umsg)) ;
                strcpy (s, (pchar)&(SOURCES[umsg.source])) ;
                strcat (s, umsg.msg) ;
                libmsgadd (q660, LIBMSG_USER, s) ;
                break ;

            case IB_EOS :
                dlth = 4 ;

                if (q660->par_create.call_eos)
                    q660->par_create.call_eos (NIL) ;

                q660->within_second = FALSE ;

                if (q660->discon_pending)
                    disconnect_me (q660) ;

                break ;
            }
        }

        p = (PU8)((PNTRINT)pstart + (PNTRINT)dlth) ;
    }
}



