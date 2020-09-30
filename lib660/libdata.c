/*   Lib660 Data Packet Processing
     Copyright 2017-2019 Certified Software Corporation

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
    3 2019-06-26 rdr Add GPS Data Stream processing
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
//#include "math.h"
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
                     "Power On - Command"} ;
const tgpscold GPSCOLD = {"Command Received", "Reception Timeout",
                          "GPS & RTC out of phase ", "Large time jump"} ;
const tsources SOURCES = {"[SS] ", "[WS] ", "[BE] ", "[I1]", "[I2]", "[I3]", "[I4]",
                          "[X1] ", "[X2] ", "[X3] ", "[X4] "} ;

void process_data (pq660 q660, pbyte p, integer lth)
begin
  boolean seqgap_occurred ;
  boolean update_ok, cal_running ;
  word w ;
  integer i, j, loops, hr ;
  pbyte pstart, p2 ;
  plcq q ;
  longword sec ;
  longint usec ;
  longint diff ;
  longint spread ;
  double t ;
  string95 s ;
  string31 s1, s2 ;
  enum tgdsrc gds ;
  pntrint bufend ;
  integer dlth ;
  tsohblk soh ;
  tengblk eng ;
  tgpsblk gps ;
  tuser_message umsg ;
  byte b1, b2 ;
  pbyte starts[100] ; /* Just used for debugging */

  seqgap_occurred = FALSE ;
  loops = 0 ;
  bufend = (pntrint)p + (pntrint)lth ;
  q660->data_timer = 0 ;
  sec = loadlongword (addr(p)) ;
  spread = seqspread(sec, q660->dt_data_sequence) ;
  if ((sec != q660->dt_data_sequence) land (sec != (q660->dt_data_sequence + 1)))
    then
      begin
        if (q660->dt_data_sequence == 0)
          then
            libdatamsg(q660, LIBMSG_SEQBEG, jul_string(sec, s1)) ; /* no continutity */
          else
            begin
              if ((q660->lastseq == 0) land (spread < 0) land (abs(spread) < MAXSPREAD))
                then
                  begin
                    sprintf (s, "%s to %s", jul_string(q660->dt_data_sequence, s1),
                             jul_string(sec, s2)) ;
                    libdatamsg (q660, LIBMSG_SEQOVER, s) ;
                  end
                else
                  begin
                    if (spread > q660->max_spread)
                      then
                        begin
                          sprintf (s, "%d, %s to %s", (integer)spread,
                             jul_string(q660->dt_data_sequence, s1), jul_string(sec, s2)) ;
                          libdatamsg (q660, LIBMSG_SEQGAP, s) ;
                          add_status (q660, LOGF_DATA_GAP, 1) ;
                          inc(q660->share.opstat.totalgaps) ;
                        end
                    seqgap_occurred = TRUE ;
                  end
            end
        q660->lasttime = 0 ;
        q660->data_timetag = 0 ; /* can't use any data until get a time mark blockette */
      end
  else if (q660->lastseq == 0)
    then
      begin
        sprintf (s, "%s to %s", jul_string(q660->dt_data_sequence, s1), jul_string(sec, s2)) ;
        libdatamsg (q660, LIBMSG_SEQRESUME, s) ;
      end
  else if (spread)
    then
      begin /* Check for missed timing blockette */
        gds = (enum tgdsrc)*p ; /* get channel */
        if (gds != GDS_TIM)
          then
            begin /* Missed, need to wait for next timing blockette */
              if (spread > q660->max_spread)
                then
                  begin
                    sprintf (s, "1, %s to %s", jul_string(q660->dt_data_sequence, s1),
                             jul_string(sec, s2)) ;
                    libdatamsg (q660, LIBMSG_SEQGAP, s) ;
                    add_status (q660, LOGF_DATA_GAP, 1) ;
                    inc(q660->share.opstat.totalgaps) ;
                  end ;
              seqgap_occurred = TRUE ;
              q660->lasttime = 0 ;
              q660->data_timetag = 0 ; /* can't use any data until get a time mark blockette */
            end
      end
  q660->dt_data_sequence = sec ; /* set global data record sequence number */
  q660->lastseq = q660->dt_data_sequence ;
  while ((loops < 100) land ((pntrint)p < bufend))
    begin
      starts[loops] = p ;
      inc(loops) ;
      pstart = p ;
      gds = (enum tgdsrc)*p ;
      p2 = (pbyte)((pntrint)p + 3) ;
      dlth = ((word)*p2 + 1) shl 2 ;
      switch (gds) begin
        case GDS_TIM :
          dlth = GDS_TIM_LENGTH ;
          if (q660->first_data)
            then
              begin
                q660->first_data = FALSE ;
                purge_continuity (q660) ;
              end ;
          update_ok = TRUE ;
          loadtimehdr (addr(p), addr(q660->time_block)) ;
          q660->data_qual = q660->time_block.clock_qual ;
          usec = q660->time_block.usec_offset ;
          t = sec + (usec / 1000000.0) ; /* Add in offset */
          if (q660->par_create.call_state)
            then
              begin
                q660->state_call.context = q660 ;
                q660->state_call.state_type = ST_TICK ;
                q660->state_call.subtype = 0 ;
                memcpy(addr(q660->state_call.station_name), addr(q660->station_ident), sizeof(string9)) ;
                q660->state_call.info = sec ;
                q660->state_call.subtype = usec ;
                q660->par_create.call_state (addr(q660->state_call)) ;
              end
          if (q660->data_timetag < 1)
            then
              begin
                q660->data_timetag = t ;
                q660->saved_data_timetag = t ;
#ifdef BSL
               /* We now have a reference timetag for a data packet. */
               /* Update the par_register.opt_start to OST_LAST for any */
               /* future reconnections to the Q8. */
               q660->par_register.opt_start = OST_LAST;
#endif
                write_ocf (q660) ;
              end
            else
              begin
                if ((t - q660->data_timetag - 1) > 0.5)
                  then
                    add_status (q660, LOGF_MISSDATA, lib_round(t - q660->data_timetag - 1)) ;
                q660->data_timetag = t ; /* we are running */
                q660->saved_data_timetag = t ;
                i = lib_round(t) mod 86400 ; /* Get within a day */
                if ((i == 0) lor (i == 43200))
                  then
                    write_ocf (q660) ; /* Write at 00:00 and 12:00 */
              end
          q660->share.opstat.last_data_time = lib_round(q660->data_timetag) ;
          if (q660->lasttime)
            then
              begin
                if (fabs(t - q660->lasttime - 1.0) > 0.5)
                  then
                    begin
                      sprintf (s1, "%9.6f", t - q660->lasttime - 1.0) ; /* conv if needed */
                      if (lnot seqgap_occurred)
                        then
                          begin
                            strcpy (s, s1) ;
                            strcat (s, " seconds") ;
                            libdatamsg(q660, LIBMSG_TIMEJMP, s) ;
                            log_clock (q660, CE_JUMP, s1) ; /* any jump that big is logged! */
                          end
                      update_ok = FALSE ;
                    end
                  else
                    begin
                      diff = lib_round((t - q660->lasttime - 1.0) * 1E6) ;
                      if (abs(diff) >= 10)
                        then
                          begin
                            sprintf (s, "%d usec", (integer)diff) ;
                            libdatamsg(q660, LIBMSG_TIMEJMP, s) ;
                          end
                      lock (q660) ;
                      if (abs(diff) >= JUMP_THRESH)
                        then
                          begin
                            sprintf (s1, "%9.6f", t - q660->lasttime - 1.0) ; /* conv if needed */
                            unlock (q660) ;
                            log_clock (q660, CE_JUMP, s1) ;
                            update_ok = FALSE ;
                          end
                        else
                          unlock (q660) ;
                    end
              end
          q660->lasttime = t ;
          add_status (q660, LOGF_THRPUT, 1) ;
          inc(q660->except_count) ;
          hr = (lib_round(t) mod 86400) div 3600 ; /* hours */
          if ((lnot q660->daily_done) land (hr == 0))
            then
              begin
                q660->daily_done = TRUE ;
                update_ok = FALSE ; /* don't it again */
                log_clock (q660, CE_DAILY, "") ;
              end
          else if (hr > 0)
            then
              q660->daily_done = FALSE ;
          if (update_ok)
            then
              if (q660->last_update < q660->data_timetag)
                then
                  begin
                    if (q660->data_qual != q660->last_data_qual)
                      then
                        log_clock (q660, CE_VALID, "") ;
                    q660->last_data_qual = q660->data_qual ;
                  end ;
          q = q660->timdispatch[TIMF_PH] ;
          if (q)
            then
              process_one (q660, q, usec) ;
          q = q660->timdispatch[TIMF_CQP] ;
          if (q)
            then
              process_one (q660, q, q660->data_qual) ;
          q = q660->timdispatch[TIMF_CLM] ;
          if (q)
            then
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
          loadsoh (addr(p), addr(soh)) ;
          q = q660->sohdispatch[SOHF_ANTCUR] ;
          if (q)
            then
              process_one (q660, q, soh.ant_cur) ;
          q = q660->sohdispatch[SOHF_SENSACUR] ;
          if (q)
            then
              process_one (q660, q, soh.sensor_cur[0]) ;
          q = q660->sohdispatch[SOHF_SENSBCUR] ;
          if (q)
            then
              process_one (q660, q, soh.sensor_cur[1]) ;
          for (i = 0 ; i <= BOOM_COUNT - 1 ; i++)
            begin
              q = q660->sohdispatch[(enum tsohfld)((integer)SOHF_BOOM1 + i)] ;
              if (q)
                then
                  process_one (q660, q, sex(soh.booms[i])) ;
            end
          q = q660->sohdispatch[SOHF_SYSTMP] ;
          if (q)
            then
              process_one (q660, q, soh.sys_temp) ;
          q = q660->sohdispatch[SOHF_HUMIDITY] ;
          if (q)
            then
              process_one (q660, q, soh.humidity) ;
          q = q660->sohdispatch[SOHF_INPVOLT] ;
          if (q)
            then
              process_one (q660, q, soh.input_volts) ;
          q = q660->sohdispatch[SOHF_VCOCTRL] ;
          if (q)
            then
              process_one (q660, q, sex(soh.vco_control)) ;
          q = q660->sohdispatch[SOHF_NEGAN] ;
          if (q)
            then
              process_one (q660, q, soh.neg_analog) ;
          q = q660->sohdispatch[SOHF_ISODC] ;
          if (q)
            then
              process_one (q660, q, soh.iso_dc) ;
          q = q660->sohdispatch[SOHF_GPIN1] ;
          if (q)
            then
              process_one (q660, q, soh.gpio1) ;
          q = q660->sohdispatch[SOHF_GPIN2] ;
          if (q)
            then
              process_one (q660, q, soh.gpio2) ;
          q = q660->sohdispatch[SOHF_SYSCUR] ;
          if (q)
            then
              process_one (q660, q, soh.sys_cur) ;
          q = q660->sohdispatch[SOHF_UPSVOLT] ;
          if (q)
            then
              process_one (q660, q, soh.ups_volts) ;
          q = q660->sohdispatch[SOHF_ANTVOLT] ;
          if (q)
            then
              process_one (q660, q, soh.ant_volts) ;
          q = q660->sohdispatch[SOHF_GPOUT] ;
          if (q)
            then
              begin
                if (soh.flags and SMF_OPTO)
                  then
                    process_one (q660, q, 1) ;
                  else
                    process_one (q660, q, 0) ;
              end
          q = q660->sohdispatch[SOHF_PKTPERC] ;
          if (q)
            then
              process_one (q660, q, soh.pkt_buf) ;
          break ;
        case GDS_ENG :
          loadeng (addr(p), addr(eng)) ;
          q = q660->engdispatch[ENGF_GPSSENS] ;
          if (q)
            then
              process_one (q660, q, eng.gps_sens) ;
          q = q660->engdispatch[ENGF_GPSCTRL] ;
          if (q)
            then
              process_one (q660, q, eng.gps_ctrl) ;
          q = q660->engdispatch[ENGF_SIOCTRLA] ;
          if (q)
            then
              process_one (q660, q, eng.sensa_dig) ;
          q = q660->engdispatch[ENGF_SIOCTRLB] ;
          if (q)
            then
              process_one (q660, q, eng.sensb_dig) ;
          q = q660->engdispatch[ENGF_SENSIO] ;
          if (q)
            then
              process_one (q660, q, eng.sens_serial) ;
          q = q660->engdispatch[ENGF_MISCIO] ;
          if (q)
            then
              process_one (q660, q, eng.misc_io) ;
          q = q660->engdispatch[ENGF_DUSTIO] ;
          if (q)
            then
              process_one (q660, q, eng.dust_io) ;
          break ;
        case GDS_GPS :
          loadgps (addr(p), addr(gps)) ;
          q = q660->gpsdispatch[GPSF_USED] ;
          if (q)
            then
              process_one (q660, q, gps.sat_used) ;
          q = q660->gpsdispatch[GPSF_FIXTYPE] ;
          if (q)
            then
              process_one (q660, q, gps.fix_type) ;
          q = q660->gpsdispatch[GPSF_LAT] ;
          if (q)
            then
              process_one (q660, q, gps.lat_udeg) ;
          q = q660->gpsdispatch[GPSF_LON] ;
          if (q)
            then
              process_one (q660, q, gps.lon_udeg) ;
          q = q660->gpsdispatch[GPSF_ELEV] ;
          if (q)
            then
              process_one (q660, q, gps.elev_dm) ;
          break ;
        default :
          switch ((byte)gds) begin
            case IB_GPSPH :
              loadbyte (addr(p)) ;
              b1 = loadbyte (addr(p)) ;
              b2 = loadbyte (addr(p)) ;
              strcpy (s, (pchar)addr(GPSPH[b1])) ;
              if (b1 == GPS_COLD)
                then
                  strcat (s, (pchar)addr(GPSCOLD[b2])) ;
              libdatamsg(q660, LIBMSG_GPSSTATUS, s) ;
              dlth = 4 ;
              break ;
            case IB_DIGPH :
              loadbyte (addr(p)) ;
              b1 = loadbyte (addr(p)) ;
              b2 = loadbyte (addr(p)) ;
              strcpy (s, (pchar)addr(DIGPH[b1])) ;
              if (b2)
                then
                  begin
                    strcat (s, ", Reason=") ;
                    strcat (s, (pchar)addr(DIGPHR[b2])) ;
                  end
              libdatamsg(q660, LIBMSG_DIGPHASE, s) ;
              dlth = 4 ;
              break ;
            case IB_LEAP :
              libdatamsg(q660, LIBMSG_LEAPDET, "") ;
              dlth = 4 ;
              break ;
            case IB_CALEVT :
              loadbyte (addr(p)) ;
              loadbyte (addr(p)) ;
              w = loadword (addr(p)) ;
              for (i = 0 ; i < SENS_CHANNELS ; i++)
                begin
                  if ((w and (3 shl (i shl 1))))
                    then
                      begin
                        cal_running = TRUE ;
                        dlth = 42 ;
                      end
                    else
                      cal_running = FALSE ;
                  for (j = 0 ; j < FREQS ; j++)
                    begin
                      q = q660->mdispatch[i][j] ;
                      if (q)
                        then
                          q->calstat = cal_running ;
                    end
                end
              dlth = 4 ;
              break ;
            case IB_DRIFT :
              loadbyte (addr(p)) ;
              loadbyte (addr(p)) ;
              loadword (addr(p)) ;
              t = loaddouble (addr(p)) ;
              sprintf (s, "%8.6f Seconds", t) ;
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
              loadumsg (addr(p), addr(umsg)) ;
              strcpy (s, (pchar)addr(SOURCES[umsg.source])) ;
              strcat (s, umsg.msg) ;
              libmsgadd (q660, LIBMSG_USER, s) ;
              break ;
            case IB_EOS :
              dlth = 4 ;
              break ;
          end
      end
      p = (pbyte)((pntrint)pstart + (pntrint)dlth) ;
    end
end

void process_low_latency (pq660 q660, pbyte p, integer lth)
begin
  integer loops ;
  pbyte pstart, p2 ;
  longword sec ;
  double t ;
  enum tgdsrc gds ;
  pntrint bufend ;
  integer dlth ;

  loops = 0 ;
  bufend = (pntrint)p + (pntrint)lth ;
  sec = loadlongword (addr(p)) ;
  while ((loops < 100) land ((pntrint)p < bufend))
    begin
      inc(loops) ;
      pstart = p ;
      gds = (enum tgdsrc)*p ;
      p2 = (pbyte)((pntrint)p + 3) ;
      dlth = ((word)*p2 + 1) shl 2 ;
      switch (gds) begin
        case GDS_TIM :
          dlth = GDS_TIM_LENGTH ;
          loadtimehdr (addr(p), addr(q660->ll_time_block)) ;
          q660->ll_offset = q660->ll_time_block.usec_offset / 1000000.0 ;
          t = sec + q660->ll_offset ;
          q660->ll_data_timetag = t ;
          q660->ll_data_qual = q660->ll_time_block.clock_qual ;
#ifdef BSL
          /* We now have a reference timetag for a data packet. */
          /* Update the par_register.opt_start to OST_LAST for any */
          /* future reconnections to the Q8. */
           q660->par_register.opt_start = OST_LAST;
#endif
          break ;
        case GDS_MD :
          process_ll (q660, p, 0, sec) ;
          break ;
        case GDS_CM :
          process_ll (q660, p, CAL_CHANNEL, sec) ;
          break ;
        case GDS_AC :
          process_ll (q660, p, MAIN_CHANNELS, sec) ;
          break ;
        default :
          break ;
      end
      p = (pbyte)((pntrint)pstart + (pntrint)dlth) ;
    end
end

