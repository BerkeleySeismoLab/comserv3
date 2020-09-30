/*
    Q660 Data & Status Record Conversion Routines
    Copyright 2016 - 2019 Certified Software Corporation

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
    0 2016-07-09 rdr Created.
    1 2017-01-18 rdr remove rms_vco from tstat_pll.
    2 2017-01-23 rdr Fix loadumsg to terminate the message, not the byte after.
    3 2017-02-21 rdr Loadstatsm and loadtimhdr updated. Loaddouble removed.
    4 2017-03-27 rdr New format for loadstatsm, loadstatsensor removed.
    5 2017-04-03 rdr New format for ttimeblk.
    6 2017-06-09 rdr Loaddouble returns. Add loaddatahdr, loadpoc, loadsoh, loadeng
                     and loadcalstart.
    7 2017-06-10 rdr Add cast to loaddouble.
    8 2018-04-04 rdr Add loading logger idents in loadstatlogger
    9 2018-04-05 rdr Changes to loadcalstart. In loadstatsm sys_temp is now 0.1C, same
                     in loadsoh.
   10 2018-06-25 rdr Changes to loadsoh.
   11 2018-08-27 rdr Changes to loadstatsm.
   12 2018-10-30 rdr Add loadstatidl
   13 2019-06-25 rdr Add loadgps.
*/
#include "pascal.h"
#include "xmlsup.h"
#include "readpackets.h"

byte loadbyte (pbyte *p)
begin
  byte temp ;

  temp = **p ;
  incn(*p, 1) ;
  return temp ;
end

shortint loadshortint (pbyte *p)
begin
  byte temp ;

  temp = **p ;
  incn(*p, 1) ;
  return (shortint) temp ;
end

word loadword (pbyte *p)
begin
  word w ;

  memcpy(addr(w), *p, 2) ;
#ifdef ENDIAN_LITTLE
  w = ntohs(w) ;
#endif
  incn(*p, 2) ;
  return w ;
end

int16 loadint16 (pbyte *p)
begin
  int16 i ;

  memcpy(addr(i), *p, 2) ;
#ifdef ENDIAN_LITTLE
  i = ntohs(i) ;
#endif
  incn(*p, 2) ;
  return i ;
end

longword loadlongword (pbyte *p)
begin
  longword lw ;

  memcpy(addr(lw), *p, 4) ;
#ifdef ENDIAN_LITTLE
  lw = ntohl(lw) ;
#endif
  incn(*p, 4) ;
  return lw ;
end

longint loadlongint (pbyte *p)
begin
  longint li ;

  memcpy(addr(li), *p, 4) ;
#ifdef ENDIAN_LITTLE
  li = ntohl(li) ;
#endif
  incn(*p, 4) ;
  return li ;
end

single loadsingle (pbyte *p)
begin
  longint li, exp ;
  single s ;

  memcpy(addr(li), *p, 4) ;
#ifdef ENDIAN_LITTLE
  li = ntohl(li) ;
#endif
  exp = (li shr 23) and 0xFF ;
  if ((exp < 1) lor (exp > 254))
    then
      li = 0 ; /* replace with zero */
  else if (li == (longint)0x80000000)
    then
      li = 0 ; /* replace with positive zero */
  memcpy(addr(s), addr(li), 4) ;
  incn(*p, 4) ;
  return s ;
end

double loaddouble (pbyte *p)
begin
  double d ;
  plword plw ;

  plw = (plword)addr(d) ;
#ifdef DOUBLE_HYBRID_ENDIAN
  *plw = loadlongword (p) ;
  inc(plw) ;
  *plw = loadlongword (p) ;
#else
#ifdef ENDIAN_LITTLE
  inc(plw) ;
  *plw = loadlongword (p) ;
  plw = (plword)addr(d) ;
  *plw = loadlongword (p) ;
#else
  *plw = loadlongword (p) ;
  inc(plw) ;
  *plw = loadlongword (p) ;
#endif
#endif
  return d ;
end

void loadstring (pbyte *p, integer fieldwidth, pchar s)
begin

  memcpy(s, *p, fieldwidth) ;
  incn(*p, fieldwidth) ;
end

void loadt64 (pbyte *p, t64 *six4)
begin

#ifdef ENDIAN_LITTLE
  (*six4)[1] = loadlongword (p) ;
  (*six4)[0] = loadlongword (p) ;
#else
  (*six4)[0] = loadlongword (p) ;
  (*six4)[1] = loadlongword (p) ;
#endif
end

void loadblock (pbyte *p, integer size, pointer pdest)
begin

  memcpy(pdest, *p, size) ;
  incn(*p, size) ;
end

/* Returns Datalength, zero if error */
integer loadqdphdr (pbyte *p, tqdp *hdr)
begin
  pbyte pstart ;
  longint *pcrc ;
  longint crc ;
  integer lth ;

  pstart = *p ;
  hdr->cmd_ver = loadbyte (p) ;
  hdr->seqs = loadbyte (p) ;
  hdr->dlength = loadbyte (p) ;
  hdr->complth = loadbyte (p) ;
  lth = ((word)hdr->dlength + 1) shl 2 ;
  if ((lth > MAXDATA) lor (hdr->complth != ((not hdr->dlength) and 0xFF)))
    then
      return 0 ;
  pcrc = (pointer)((pntrint)pstart + lth + QDP_HDR_LTH) ;
  crc = gcrccalc (pstart, lth + QDP_HDR_LTH) ;
  if (crc == loadlongint((pbyte *)addr(pcrc)))
    then
      return lth ;
    else
      return 0 ;
end

void loadstatsm (pbyte *p, tstat_sm *psm)
begin
  integer i ;

  psm->stype = (enum tstype) loadbyte (p) ;
  psm->flags = loadbyte (p) ;
  psm->pll_status = (enum tpllstat) loadbyte (p) ;
  psm->blk_size = loadbyte (p) ;
  psm->gpio1 = loadbyte (p) ;
  psm->gpio2 = loadbyte (p) ;
  psm->pkt_buf = loadbyte (p) ;
  psm->humidity = loadbyte (p) ;
  for (i = 0 ; i < SENSOR_COUNT ; i++)
    psm->sensor_cur[i] = loadword (p) ;
  for (i = 0 ; i < BOOM_COUNT ; i++)
    psm->booms[i] = loadint16 (p) ;
  psm->sys_cur = loadword (p) ;
  psm->ant_cur = loadword (p) ;
  psm->input_volts = loadword (p) ;
  psm->sys_temp = loadint16 (p) ;
  psm->spare3 = loadlongint (p) ;
  psm->sec_offset = loadlongword (p) ;
  psm->usec_offset = loadlongint (p) ;
  psm->total_time = loadlongword (p) ;
  psm->power_time = loadlongword (p) ;
  psm->last_resync = loadlongword (p) ;
  psm->resyncs = loadlongword (p) ;
  psm->clock_loss = loadword (p) ;
  for (i = 0 ; i < SENSOR_COUNT ; i++)
    psm->gain_status[i] = (enum tadgain) loadbyte (p) ;
  psm->sensor_ctrl_map = loadword (p) ;
  psm->fault_code = loadbyte (p) ;
  psm->spare4 = loadbyte (p) ;
  psm->gps_pwr = (enum tgpspwr) loadbyte (p) ;
  psm->gps_fix = (enum tgpsfix) loadbyte (p) ;
  psm->clock_qual = loadbyte (p) ;
  psm->cal_stat = loadbyte (p) ;
  psm->elevation = loadsingle (p) ;
  psm->latitude = loadsingle (p) ;
  psm->longitude = loadsingle (p) ;
  psm->ant_volts = loadword (p) ;
  psm->spare5 = loadword (p) ;
end

void loadstatgps (pbyte *p, tstat_gps *pgp)
begin
  integer i ;
  tstat_gpssat *psat ;

  pgp->stype = (enum tstype) loadbyte (p) ;
  pgp->spare1 = loadbyte (p) ;
  pgp->sat_count = loadbyte (p) ;
  pgp->blk_size = loadbyte (p) ;
  pgp->sat_used = loadbyte (p) ;
  pgp->sat_view = loadbyte (p) ;
  pgp->check_errors = loadword (p) ;
  pgp->gpstime = loadword (p) ;
  pgp->spare2 = loadword (p) ;
  pgp->last_good = loadlongword (p) ;
  for (i = 0 ; i < MAX_SAT ; i++)
    begin
      psat = addr(pgp->gps_sats[i]) ;
      psat->num = loadword (p) ;
      psat->elevation = loadint16 (p) ;
      psat->azimuth = loadint16 (p) ;
      psat->snr = loadword (p) ;
    end
end

void loadstatpll (pbyte *p, tstat_pll *pll)
begin

  pll->stype = (enum tstype) loadbyte (p) ;
  pll->spare1 = loadbyte (p) ;
  pll->spare2 = loadbyte (p) ;
  pll->blk_size = loadbyte (p) ;
  pll->start_km = loadsingle (p) ;
  pll->time_error = loadsingle (p) ;
  pll->best_vco = loadsingle (p) ;
  pll->ticks_track_lock = loadlongword (p) ;
  pll->km = loadint16 (p) ;
  pll->cur_vco = loadint16 (p) ;
end

void loadstatlogger (pbyte *p, tstat_logger *plog)
begin
  pbyte pb, pstart ;
  integer i, j ;

  pstart = *p ;
  plog->stype = (enum tstype) loadbyte (p) ;
  plog->id_count = loadbyte (p) ;
  plog->status = loadbyte (p) ;
  plog->blk_size = loadbyte (p) ;
  plog->last_on = loadlongword (p) ;
  plog->powerups = loadlongword (p) ;
  plog->timeouts = loadword (p) ;
  plog->baler_time = loadword (p) ;
  if (plog->id_count)
    then
      begin /* need to copy idents in */
        pb = *p ;
        j = (pntrint)pb - (pntrint)pstart ; /* Size of the above */
        i = ((word)plog->blk_size + 1) shl 2 ; /* Actual size with idents */
        i = i - j ; /* size of idents */
        if ((i > 0) land (i <= MAX_IDENTS_LTH))
          then
            begin /* Looks OK */
              memcpy (addr(plog->idents), pb, i) ;
              incn (pb, i) ;
              *p = pb ; /* update pointer */
            end
      end
end

void loadstatidl (pbyte *p, tstat_idl *pidl)
begin
  integer lth ;
  pbyte pb ;

  pidl->stype = (enum tstype)loadbyte (p) ;
  pidl->spare1 = loadbyte (p) ;
  pidl->spare2 = loadbyte (p) ;
  pidl->blk_size = loadbyte (p) ;
  lth = ((word)pidl->blk_size + 1) shl 2 ; /* Actual size of blockette */
  pb = *p ;
  memcpy (addr(pidl->idlstat), pb, lth - 4) ;
  incn (pb, lth - 4) ;
  *p = pb ;
end


void loadumsg (pbyte *p, tuser_message *pu)
begin
  integer mlth, lth ;
  pbyte psave ;

  psave = *p ;
  pu->blk_type = loadbyte (p) ;
  pu->source = (enum tsource) loadbyte (p) ;
  pu->msglth = loadbyte (p) ;
  pu->blk_size = loadbyte (p) ;
  lth = ((word)pu->blk_size + 1) shl 2 ;
  if (lth < 4)
    then
      lth = 4 ;
  if (pu->msglth)
    then
      begin
        mlth = (pu->msglth + 3) and 0xFFFC ; /* Round up to multiple of 4 bytes */
        memcpy (addr(pu->msg), *p, mlth) ;
      end
  (pu->msg)[pu->msglth - 1] = 0 ; /* make sure terminated */
  incn (psave, lth) ; /* Move past blockette */
  *p = psave ;
end

void loadtimehdr (pbyte *p, ttimeblk *pt)
begin
  longint l ;

  pt->gds = (enum tgdsrc) loadbyte (p) ;
  pt->clock_qual = loadbyte (p) ;
  pt->since_loss = loadword (p) ;
  l = loadlongint (p) ;
  pt->flags = l shr 24 ;
  l = l and 0xFFFFFF ; /* LS 24 bits only */
  if (l and 0x800000)
    then
      l = l or 0xFF000000 ; /* sign extend to 32 bits */
  pt->usec_offset = l ;
end

/* Reads a variable number of bytes depending on flags. Leaves p pointing
  at start of map */
void loaddatahdr (pbyte *p, tdatahdr *datahdr)
begin

  datahdr->gds = (enum tgdsrc)loadbyte (p) ;
  datahdr->chan = loadbyte (p) ;
  datahdr->offset_flag = loadbyte (p) ;
  datahdr->blk_size = loadbyte (p) ;
  if (datahdr->offset_flag and DHOF_PREV)
    then
      datahdr->prev_offset = loadlongint (p) ;
    else
      datahdr->prev_offset = 0 ;
  if (datahdr->offset_flag and DHOF_OFF)
    then
      datahdr->sampoff = loadword (p) ;
    else
      datahdr->sampoff = 0 ;
end

void loadsoh (pbyte *p, tsohblk *soh)
begin
  integer i ;

  soh->gds = (enum tgdsrc)loadbyte (p) ;
  soh->flags = loadbyte (p) ;
  soh->spare1 = loadbyte (p) ;
  soh->blk_size = loadbyte (p) ;
  soh->gpio1 = loadbyte (p) ; /* General Purpose Input 1 % */
  soh->gpio2 = loadbyte (p) ; /* General Purpose Input 2 % */
  soh->pkt_buf = loadbyte (p) ; /* Packet Buffer Percent */
  soh->humidity = loadbyte (p) ; /* Sensor Humidity % */
  for (i = 0 ; i < SENSOR_COUNT ; i++)
    soh->sensor_cur[i] = loadword (p) ;
  for (i = 0 ; i < BOOM_COUNT ; i++)
    soh->booms[i] = loadint16 (p) ; /* Booms - 1mv */
  soh->sys_cur = loadword (p) ; /* System Current - 1ma (100ua) */
  soh->ant_cur = loadword (p) ; /* GPS antenna current - 100ua */
  soh->input_volts = loadword (p) ; /* Input Voltage - 10mv (2mv) */
  soh->sys_temp = loadint16 (p) ; /* System temperature - 0.1C */
  soh->spare3 = loadlongint (p) ;
  soh->neg_analog = loadword(p) ;
  soh->iso_dc = loadword(p) ;
  soh->vco_control = loadint16(p) ;
  soh->ups_volts = loadword(p) ;
  soh->ant_volts = loadword(p) ;
  soh->spare6 = loadword(p) ;
  soh->spare4 = loadword(p) ;
  soh->spare5 = loadword(p) ;
end

void loadeng (pbyte *p, tengblk *eng)
begin

  eng->gds = (enum tgdsrc)loadbyte (p) ;
  eng->gps_sens = loadbyte (p) ;
  eng->gps_ctrl = loadbyte (p) ;
  eng->blk_size = loadbyte (p) ;
  eng->sensa_dig = loadword (p) ;
  eng->sensb_dig = loadword (p) ;
  eng->sens_serial = loadbyte (p) ;
  eng->misc_io = loadbyte (p) ;
  eng->dust_io = loadword (p) ;
  eng->spare = loadlongword (p) ;
end

void loadgps (pbyte *p, tgpsblk *gps)
begin

  gps->gds = (enum tgdsrc)loadbyte (p) ;
  gps->sat_used = loadbyte (p) ;
  gps->fix_type = loadbyte (p) ;
  gps->blk_size = loadbyte (p) ;
  gps->lat_udeg = loadlongint (p) ;
  gps->lon_udeg = loadlongint (p) ;
  gps->elev_dm = loadlongint (p) ;
end

void loadcalstart (pbyte *p, tcalstart *cals)
begin

  cals->chan = loadbyte (p) ;
  cals->amplitude = loadbyte (p) ;
  cals->waveform = loadbyte (p) ;
  cals->blk_size = loadbyte (p) ;
  cals->freqdiv = loadword (p) ;
  cals->duration = loadword (p) ;
  cals->calbit_map = loadword (p) ;
  cals->spare = loadword (p) ;
  cals->settle = loadword (p) ;
  cals->trailer = loadword (p) ;
  cals->spare2 = loadlongword (p) ;
  cals->spare3 = loadlongword (p) ;
end

void loadpoc (pbyte *p, tpoc_message *poc)
begin

  poc->ptype = loadbyte (p) ;
  poc->spare1 = loadbyte (p) ;
  poc->spare2 = loadbyte (p) ;
  poc->blk_size = loadbyte (p) ; /* Blockette Size */
#ifdef ENDIAN_LITTLE
  poc->q660_sn[1] = loadlongword (p) ;
  poc->q660_sn[0] = loadlongword (p) ;
#else
  poc->q660_sn[0] = loadlongword (p) ;
  poc->q660_sn[1] = loadlongword (p) ;
#endif
  poc->baseport = loadword (p) ; /* Base Port */
  poc->token = loadword (p) ; /* Registration token */
end


