



/*
    Q660 Data & Status Record Conversion Routines
    Copyright 2016 - 2021 by
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
   14 2020-05-28 jms remove DOUBLE_HYBRID_ENDIAN handling of BE double
   15 2021-02-03 rdr Add new GPS field to engineering data.
   16 2021-03-20 rdr Add loadstatdust.
   17 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "platform.h"
#include "xmlsup.h"
#include "readpackets.h"

U8 loadbyte (PU8 *p)
{
    U8 temp ;

    temp = **p ;
    *p = *p + (1) ;
    return temp ;
}

I8 loadshortint (PU8 *p)
{
    U8 temp ;

    temp = **p ;
    *p = *p + (1) ;
    return (I8) temp ;
}

U16 loadword (PU8 *p)
{
    U16 w ;

    memcpy(&(w), *p, 2) ;
#ifdef ENDIAN_LITTLE
    w = ntohs(w) ;
#endif
    *p = *p + (2) ;
    return w ;
}

I16 loadint16 (PU8 *p)
{
    I16 i ;

    memcpy(&(i), *p, 2) ;
#ifdef ENDIAN_LITTLE
    i = ntohs(i) ;
#endif
    *p = *p + (2) ;
    return i ;
}

U32 loadlongword (PU8 *p)
{
    U32 lw ;

    memcpy(&(lw), *p, 4) ;
#ifdef ENDIAN_LITTLE
    lw = ntohl(lw) ;
#endif
    *p = *p + (4) ;
    return lw ;
}

I32 loadlongint (PU8 *p)
{
    I32 li ;

    memcpy(&(li), *p, 4) ;
#ifdef ENDIAN_LITTLE
    li = ntohl(li) ;
#endif
    *p = *p + (4) ;
    return li ;
}

float loadsingle (PU8 *p)
{
    I32 li, exp ;
    float s ;

    memcpy(&(li), *p, 4) ;
#ifdef ENDIAN_LITTLE
    li = ntohl(li) ;
#endif
    exp = (li >> 23) & 0xFF ;

    if ((exp < 1) || (exp > 254))
        li = 0 ; /* replace with zero */
    else if (li == (I32)0x80000000)
        li = 0 ; /* replace with positive zero */

    memcpy(&(s), &(li), 4) ;
    *p = *p + (4) ;
    return s ;
}

double loaddouble (PU8 *p)
{
    double d ;
    PU32 plw ;

    plw = (PU32)&(d) ;
#ifdef ENDIAN_LITTLE
    (plw)++ ;
    *plw = loadlongword (p) ;
    plw = (PU32)&(d) ;
    *plw = loadlongword (p) ;
#else
    *plw = loadlongword (p) ;
    (plw)++ ;
    *plw = loadlongword (p) ;
#endif
    return d ;
}

void loadstring (PU8 *p, int fieldwidth, pchar s)
{

    memcpy(s, *p, fieldwidth) ;
    *p = *p + (fieldwidth) ;
}

void loadt64 (PU8 *p, t64 *six4)
{

#ifdef ENDIAN_LITTLE
    (*six4)[1] = loadlongword (p) ;
    (*six4)[0] = loadlongword (p) ;
#else
    (*six4)[0] = loadlongword (p) ;
    (*six4)[1] = loadlongword (p) ;
#endif
}

void loadblock (PU8 *p, int size, pointer pdest)
{

    memcpy(pdest, *p, size) ;
    *p = *p + (size) ;
}

/* Returns Datalength, zero if error */
int loadqdphdr (PU8 *p, tqdp *hdr)
{
    PU8 pstart ;
    I32 *pcrc ;
    I32 crc ;
    int lth ;

    pstart = *p ;
    hdr->cmd_ver = loadbyte (p) ;
    hdr->seqs = loadbyte (p) ;
    hdr->dlength = loadbyte (p) ;
    hdr->complth = loadbyte (p) ;
    lth = ((U16)hdr->dlength + 1) << 2 ;

    if ((lth > MAXDATA) || (hdr->complth != ((~ hdr->dlength) & 0xFF)))
        return 0 ;

    pcrc = (pointer)((PNTRINT)pstart + lth + QDP_HDR_LTH) ;
    crc = gcrccalc (pstart, lth + QDP_HDR_LTH) ;

    if (crc == loadlongint((PU8 *)&(pcrc)))
        return lth ;
    else
        return 0 ;
}

void loadstatsm (PU8 *p, tstat_sm *psm)
{
    int i ;

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
}

void loadstatgps (PU8 *p, tstat_gps *pgp)
{
    int i ;
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

    for (i = 0 ; i < MAX_SAT ; i++) {
        psat = &(pgp->gps_sats[i]) ;
        psat->num = loadword (p) ;
        psat->elevation = loadint16 (p) ;
        psat->azimuth = loadint16 (p) ;
        psat->snr = loadword (p) ;
    }
}

void loadstatpll (PU8 *p, tstat_pll *pll)
{

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
}

void loadstatlogger (PU8 *p, tstat_logger *plog)
{
    PU8 pb, pstart ;
    int i, j ;

    pstart = *p ;
    plog->stype = (enum tstype) loadbyte (p) ;
    plog->id_count = loadbyte (p) ;
    plog->status = loadbyte (p) ;
    plog->blk_size = loadbyte (p) ;
    plog->last_on = loadlongword (p) ;
    plog->powerups = loadlongword (p) ;
    plog->timeouts = loadword (p) ;
    plog->baler_time = loadword (p) ;

    if (plog->id_count) {
        /* need to copy idents in */
        pb = *p ;
        j = (PNTRINT)pb - (PNTRINT)pstart ; /* Size of the above */
        i = ((U16)plog->blk_size + 1) << 2 ; /* Actual size with idents */
        i = i - j ; /* size of idents */

        if ((i > 0) && (i <= MAX_IDENTS_LTH)) {
            /* Looks OK */
            memcpy (&(plog->idents), pb, i) ;
            pb = pb + (i) ;
            *p = pb ; /* update pointer */
        }
    }
}

void loadstatidl (PU8 *p, tstat_idl *pidl)
{
    int lth ;
    PU8 pb ;

    pidl->stype = (enum tstype)loadbyte (p) ;
    pidl->spare1 = loadbyte (p) ;
    pidl->spare2 = loadbyte (p) ;
    pidl->blk_size = loadbyte (p) ;
    lth = ((U16)pidl->blk_size + 1) << 2 ; /* Actual size of blockette */
    pb = *p ;
    memcpy (&(pidl->idlstat), pb, lth - 4) ;
    pb = pb + (lth - 4) ;
    *p = pb ;
}

void loadstatdust (PU8 *p, tstat_dust *pdst)
{
    tone_dust_stat *pone ;
    int i, lth ;

    pdst->stype = (enum tstype)loadbyte (p) ;
    pdst->flags = loadbyte (p) ;
    pdst->count = loadbyte (p) ;
    pdst->blk_size = loadbyte (p) ;

    for (i = 0 ; i < pdst->count ; i++) {
        pone = &(pdst->dust_stats[i]) ;
        pone->stype = (enum tstype)loadbyte (p) ;
        pone->string_count = loadbyte (p) ;
        pone->flags_slot = loadbyte (p) ;
        pone->blk_size = loadbyte (p) ;
        pone->time = loadlongword (p) ;
        lth = ((U16)pone->blk_size + 1) << 2 ; /* Actual size of sub-blockette */
        loadblock (p, lth - 8, &(pone->strings)) ;
    }
}

void loadumsg (PU8 *p, tuser_message *pu)
{
    int mlth, lth ;
    PU8 psave ;

    psave = *p ;
    pu->blk_type = loadbyte (p) ;
    pu->source = (enum tsource) loadbyte (p) ;
    pu->msglth = loadbyte (p) ;
    pu->blk_size = loadbyte (p) ;
    lth = ((U16)pu->blk_size + 1) << 2 ;

    if (lth < 4)
        lth = 4 ;

    if (pu->msglth) {
        mlth = (pu->msglth + 3) & 0xFFFC ; /* Round up to multiple of 4 bytes */
        memcpy (&(pu->msg), *p, mlth) ;
    }

    (pu->msg)[pu->msglth - 1] = 0 ; /* make sure terminated */
    psave = psave + (lth) ; /* Move past blockette */
    *p = psave ;
}

void loadtimehdr (PU8 *p, ttimeblk *pt)
{
    I32 l ;

    pt->gds = (enum tgdsrc) loadbyte (p) ;
    pt->clock_qual = loadbyte (p) ;
    pt->since_loss = loadword (p) ;
    l = loadlongint (p) ;
    pt->flags = l >> 24 ;
    l = l & 0xFFFFFF ; /* LS 24 bits only */

    if (l & 0x800000)
        l = l | 0xFF000000 ; /* sign extend to 32 bits */

    pt->usec_offset = l ;
}

/* Reads a variable number of bytes depending on flags. Leaves p pointing
  at start of map */
void loaddatahdr (PU8 *p, tdatahdr *datahdr)
{

    datahdr->gds = (enum tgdsrc)loadbyte (p) ;
    datahdr->chan = loadbyte (p) ;
    datahdr->offset_flag = loadbyte (p) ;
    datahdr->blk_size = loadbyte (p) ;

    if (datahdr->offset_flag & DHOF_PREV)
        datahdr->prev_offset = loadlongint (p) ;
    else
        datahdr->prev_offset = 0 ;

    if (datahdr->offset_flag & DHOF_OFF)
        datahdr->sampoff = loadword (p) ;
    else
        datahdr->sampoff = 0 ;
}

void loadsoh (PU8 *p, tsohblk *soh)
{
    int i ;

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
}

void loadeng (PU8 *p, tengblk *eng)
{

    eng->gds = (enum tgdsrc)loadbyte (p) ;
    eng->gps_sens = loadbyte (p) ;
    eng->gps_ctrl = loadbyte (p) ;
    eng->blk_size = loadbyte (p) ;
    eng->sensa_dig = loadword (p) ;
    eng->sensb_dig = loadword (p) ;
    eng->sens_serial = loadbyte (p) ;
    eng->misc_io = loadbyte (p) ;
    eng->dust_io = loadword (p) ;
    eng->gps_hal = loadbyte (p) ;
    eng->gps_state = loadbyte (p) ;
    eng->spare = loadword (p) ;
}

void loadgps (PU8 *p, tgpsblk *gps)
{

    gps->gds = (enum tgdsrc)loadbyte (p) ;
    gps->sat_used = loadbyte (p) ;
    gps->fix_type = loadbyte (p) ;
    gps->blk_size = loadbyte (p) ;
    gps->lat_udeg = loadlongint (p) ;
    gps->lon_udeg = loadlongint (p) ;
    gps->elev_dm = loadlongint (p) ;
}

void loaddust (PU8 *p, tdustblk *dust)
{
    int i, size ;
    PU8 pb ;

    dust->gds = (enum tgdsrc)loadbyte (p) ;
    dust->count = loadbyte (p) ;
    dust->flags_slot = loadbyte (p) ;
    dust->blk_size = loadbyte (p) ;
    dust->time = loadlongword (p) ;
    dust->map = loadlongword (p) ;
    pb = (PU8)(&(dust->samples)) ;
    memclr (&(dust->samples), sizeof(I32) * DUST_CHANNELS) ;

    for (i = 0 ; i < DUST_CHANNELS ; i++) {
        size = (dust->map >> (i * 2)) & 3 ;

        switch (size) {
        case 1 :
            dust->samples[i] = loadshortint (p) ;
            break ;

        case 2 :
            dust->samples[i] = loadint16 (p) ;
            break ;

        case 3 :
            dust->samples[i] = loadlongint (p) ;
            break ;
        }
    }
}

void loadcalstart (PU8 *p, tcalstart *cals)
{

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
}

void loadpoc (PU8 *p, tpoc_message *poc)
{

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
}


