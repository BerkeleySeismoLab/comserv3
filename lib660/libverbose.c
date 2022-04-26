



/*   Lib660 Status Dump Routine
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
    0 2017-06-11 rdr Created
    1 2017-06-16 rdr Make a possibly truncated version of config name to avoid
                     overflow.
    2 2018-04-04 rdr Add showing logger idents. Change unit for sys_temp.
    3 2018-06-25 rdr Add showing clock serial number.
    4 2018-08-28 rdr Add showing antenna voltage.
    5 2021-01-06 jms harden formatting of time error
    6 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "libverbose.h"
#include "libmsgs.h"
#include "libsupport.h"
#include "libsampglob.h"
#include "libsample.h"

const tgstats GPWRS = {"Off", "Off due to PLL lock", "Off due to time limit",
                       "Off by command", "Off due to a fault", "On automatically", "On by command",
                       "Coldstart"
                      } ;
const tgstats GFIXS = {"Off, unknown fix", "Off, no fix", "Off, last fix was 1D",
                       "Off, last fix was 2D", "Off, last fix was 3D", "On, no fix", "On, unknown fix",
                       "On, 1D fix", "On, 2D fix", "On, 3D fix"
                      } ;
const tgstats PSTATS = {"Off", "Hold", "Tracking", "Locked"} ;

static void report_channel_and_preamp_settings (pq660 q660)
{
    int i ;
    float pg ;
    enum tadgain adg ;
    float vpct[TOTAL_CHANNELS] ;
    string s ;
    tstat_sm *psm ;

    psm = &(q660->share.stat_sm) ;

    for (i = 0 ; i < SENS_CHANNELS ; i++) {
        adg = psm->gain_status[i / 3] ;
        vpct[i] = 0.000002384 ;

        if (adg == AG_LOW) {
            pg = 8.0 * q660->share.digis[i].pgagain ;
            vpct[i] = vpct[i] / pg ;
        } else
            vpct[i] = vpct[i] / q660->share.digis[i].pgagain ;
    }

    vpct[CAL_CHANNEL] = 0.000002384 ; /* Cal monitor are always gain of 1 */

    for (i = MAIN_CHANNELS ; i < TOTAL_CHANNELS ; i++)
        vpct[i] = 0.001 ; /* 1mg per count */

    for (i = 0 ; i < MAIN_CHANNELS  ; i++)
        if (q660->share.digis[i].freqmap) {
            sprintf (s, "Channel %d uv per count: %6.4f", i + 1, vpct[i] * 1.0e6) ;
            libmsgadd(q660, LIBMSG_SYSINFO, s) ;
        }

    for (i = MAIN_CHANNELS ; i < TOTAL_CHANNELS ; i++)
        if (q660->share.digis[i].freqmap) {
            sprintf (s, "Channel %d mg per count: %5.3f", i + 1, vpct[i] * 1.0e3) ;
            libmsgadd(q660, LIBMSG_SYSINFO, s) ;
        }
}

void log_all_info (pq660 q660)
{
    string s ;
    char s1[300] ;
    int i, j ;
    I32 l ;
    tstat_sm *psm ;
    tsysinfo *psi ;
    tstat_gps *psg ;
    tstat_pll *psp ;
    tstat_logger *psl ;
    pchar pc ;
    U8 b ;

    psm = &(q660->share.stat_sm) ;
    psi = &(q660->share.sysinfo) ;
    psg = &(q660->share.stat_gps) ;
    psp = &(q660->share.stat_pll) ;
    psl = &(q660->share.stat_logger) ;
    libmsgadd(q660, LIBMSG_SYSINFO, "Digitizer Information") ;
    sprintf (s, "Property Tag: %s", psi->prop_tag) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Network-Station: %s-%s", q660->share.seed.network,
             q660->share.seed.station) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Station Time: %s", jul_string(psm->sec_offset, s1)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Q660 Serial Number: %s", psi->fe_ser) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Configuration Name: %s", q660->share.seed.cfgname) ;
    s[95] = 0 ; /* limit size of suffix */
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Clock Type: %s", psi->clk_typ) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Clock Serial Number: %d", psi->clock_serial) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Clock Version: %s", psi->clock_ver) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Front End Version: %s", psi->fe_ver) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Back End Version: %s", psi->be_ver) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "System Supervisor Version: %s", psi->ss_ver) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "PLD Versions: %s", psi->pld_ver) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Sensor A Type: %s", psi->sa_type) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Sensor B Type: %s", psi->sb_type) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Time of Last FE Boot: %s", psi->last_rb) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Total Number of FE Boots: %d", (int)psi->reboots) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Time of Last BE Boot: %s", jul_string(psi->beboot, s1)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Time of Last SS Boot: %s", jul_string(psi->ssboot, s1)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    report_channel_and_preamp_settings (q660) ;
    libmsgadd(q660, LIBMSG_SYSINFO, "Digitizer Status") ;
    sprintf (s, "Total Hours: %4.2f", psm->total_time / 3600.0) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Power On Hours: %4.2f", psm->power_time / 3600.0) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Time of Last Re-Sync: %s", jul_string(psm->last_resync, s1)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Total Number of Re-Syncs: %d", (int)psm->resyncs) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Clock Quality: %d%%", psm->clock_qual) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Clock Phase: %dusec", (int)psm->usec_offset) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    strcpy (s, "Boom Positions: ") ;

    for (i = 0 ; i < SENS_CHANNELS ; i++) {
        sprintf(s1, "%5.3f", (int)psm->booms[i] * 0.001) ;

        if (i)
            strcat (s, ", ") ;

        strcat(s, s1) ;
    }

    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
#ifdef DEVSOH
    sprintf (s, "Input Voltage: %5.3fV", psm->input_volts * 0.002) ;
#else
    sprintf (s, "Input Voltage: %4.2fV", psm->input_volts * 0.01) ;
#endif
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "System Temperature: %3.1fC", psm->sys_temp * 0.1) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
#ifdef DEVSOH
    sprintf (s, "System Current: %3.1fma", psm->sys_cur * 0.1) ;
#else
    sprintf (s, "System Current: %dma", (int)psm->sys_cur) ;
#endif
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Antenna Current: %3.1fma", (int)psm->ant_cur * 0.1) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
#ifdef DEVSOH
    sprintf (s, "Antenna Voltage: %5.3fV", psm->ant_volts * 0.002) ;
#else
    sprintf (s, "Antenna Voltage: %4.2fV", psm->ant_volts * 0.01) ;
#endif
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;

    if (q660->sensora_ok) {
#ifdef DEVSOH
        sprintf (s, "Sensor A Current: %3.1fma", psm->sensor_cur[0] * 0.1) ;
#else
        sprintf (s, "Sensor A Current: %dma", (int)((U32)psm->sensor_cur[0])) ;
#endif
        libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    }

    if (q660->sensorb_ok) {
#ifdef DEVSOH
        sprintf (s, "Sensor B Current: %3.1fma", psm->sensor_cur[1] * 0.1) ;
#else
        sprintf (s, "Sensor B Current: %dma", (int)((U32)psm->sensor_cur[1])) ;
#endif
        libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    }

    sprintf (s, "Packet Buffer Used: %d%%", (int)psm->pkt_buf) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "General Purpose Input 1: %d%%", (int)psm->gpio1) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "General Purpose Input 2: %d%%", (int)psm->gpio2) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;

    if (psm->flags & SMF_OPTO)
        strcpy (s1, "On") ;
    else
        strcpy (s1, "Off") ;

    sprintf (s, "General Purpose Output: %s", s1) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;

    if (psm->fault_code) {
        sprintf (s, "Fault Code: %d-%d", (int)psm->fault_code >> 4,
                 (int)psm->fault_code & 0xF) ;
        libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    }

    libmsgadd(q660, LIBMSG_SYSINFO, "GPS Status") ;
    sprintf (s, "Power State: %s", (pchar)&(GPWRS[psm->gps_pwr])) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Fix Status: %s", (pchar)&(GFIXS[psm->gps_fix])) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Latitude: %5.3f", psm->latitude) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Longitude: %5.3f", psm->longitude) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Elevation: %dm", (int)lib_round(psm->elevation)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;

    if (psm->gps_fix >= GFIX_ONNF)
        strcpy (s1, "On Time") ;
    else
        strcpy (s1, "Off Time") ;

    sprintf (s, "%s: %dmin", s1, (int)((U32)psg->gpstime)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Number of Satellites Used: %d", (int)psg->sat_used) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Number of Satellites in View: %d", (int)psg->sat_view) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Checksum Errors: %d", (int)psg->check_errors) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    l = psg->last_good ;

    if (l) {
        sprintf (s, "Last GPS Timemark: %s", jul_string(l, s1)) ;
        libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    }

    libmsgadd(q660, LIBMSG_SYSINFO, "PLL Status") ;
    sprintf (s, "PLL State: %s", (pchar)&(PSTATS[psm->pll_status])) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Initial VCO: %3.1f", (float)psp->start_km) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Time Error: %.7g", (float)psp->time_error) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Best VCO: %4.2f", (float)psp->best_vco) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Seconds since Track or Lock: %3.1f", (float)(psp->ticks_track_lock / 1000.0)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "VCO Control: %d", (int)psp->cur_vco) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    libmsgadd(q660, LIBMSG_SYSINFO, "Logger Status") ;

    if (psl->last_on) {
        sprintf (s, "Last Power On: %s", jul_string(psl->last_on, s1)) ;
        libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    }

    sprintf (s, "Number of Power-ups: %d", (int)psl->powerups) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Number of Timeouts: %d", (int)((U32)psl->timeouts)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;
    sprintf (s, "Minutes since Activated: %d", (int)((U32)psl->baler_time)) ;
    libmsgadd(q660, LIBMSG_SYSINFO, s) ;

    if (psl->id_count) {
        pc = (pchar)&(psl->idents) ;

        for (i = 1 ; i <= psl->id_count ; i++) {
            b = (U8)*pc++ ;
            j = b & 15 ; /* get client */

            if (j < 4)
                sprintf (s, "Internal Data Logger %d: ", j + 1) ;
            else
                sprintf (s, "External Data Logger %d: ", j - 3) ;

            strcat (s, pc) ;
            pc = pc + (strlen(pc) + 1) ; /* skip string */
            sprintf (s1, ", Priority %d", (int)b >> 4) ;
            strcat (s, s1) ;
            libmsgadd(q660, LIBMSG_SYSINFO, s) ;
        }
    }
}

U32 print_generated_rectotals (pq660 q660)
{
    plcq q ;
    string m ;
    string31 s ;
    string15 s1 ;
    I32 futuremr ;
    U32 totrec ;
    BOOLEAN secondphase ;

    q = q660->lcqs ;
    strcpy(m, "written:") ;
    totrec = 0 ;
    secondphase = FALSE ;

    while (q) {
        futuremr = 0 ; /* forecast pending message record */

        if ((q == q660->msg_lcq) && (q->com->ring) && (q->com->frame >= 2))
            futuremr = 1 ;

        totrec = totrec + q->records_generated_session + futuremr ;

        if ((q->records_generated_session + futuremr) > 0) {
            if (strlen(m) >= 68) {
                libmsgadd(q660, LIBMSG_TOTAL,m) ;
                strcpy(m, "written:") ;
            }

            sprintf(s, " %s-%d", seed2string(q->location, q->seedname, s1),
                    (int)(q->records_generated_session + futuremr)) ;
            strcat (m, s) ;
        }

        q = q->link ;

        if (q == NIL)
            if (! secondphase) {
                secondphase = TRUE ;
                q = q660->dplcqs ;
            }
    }

    if (strlen(m) > 8)
        libmsgadd(q660, LIBMSG_TOTAL, m) ;

    return totrec ;
}

