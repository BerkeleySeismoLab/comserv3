



/*
     XML Read Configuration Routines (excluding Seed area)
     Copyright 2015-2021 by
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
    0 2015-12-21 rdr Adapted from WS330
    1 2016-01-19 rdr Add parsing IPV6 addresses.
    2 2016-02-09 rdr Change engine list, type of external engine is irrelevant.
    3 2016-03-02 rdr Change GPS resync hour to hour bitmap to match network.
    4 2016-03-08 rdr Add handling of low latency target.
    5 2016-06-07 rdr Add handling of quick-view OK flag.
    6 2016-07-20 rdr Split up Deploy/Remove to separate entries.
    7 2017-03-23 rdr Fix converting long digitizer frequency list to bitmap.
    8 2017-04-03 rdr Add reading BE and SS boot times from sysinfo.
    9 2018-04-17 rdr Add being able to specify control line 0 function. Add new
                     control functions.
   10 2018-05-01 rdr Add tcxooff field for timing.
   11 2018-05-03 rdr Add fields for global timeout for announcements.
   12 2018-06-06 rdr Add CMG-3T to sensor list.
   13 2018-06-25 rdr Internal Antenna and Voltage Boost options added to timing.
   14 2018-07-16 rdr In read_options increase size of s.
   15 2018-08-27 rdr Add options field for sensors.
   16 2018-09-05 rdr Fix first aux line for xml, should be aux0, not aux5.
   17 2018-09-12 rdr Add mtu, dns, and enables to networking.
   18 2018-09-23 rdr If a digitizer channel is not configured (empty frequency
                     list) then set target to default. If any digitizer channel has
                     a gain of 0, set to 1.
   19 2018-11-06 rdr Don't preset any network values if network section missing.
                     However set to reasonable values if currently zero.
   20 2019-06-20 rdr Add new Timing fields.
   21 2019-08-06 rdr Add new options fields for WOS.
   22 2019-10-22 rdr Change low latency target minimum to 150ms.
   23 2019-11-13 rdr Remove low_lat in sysinfo.
  23b 2019-12-31 jms added new sensor types and added SLIP IP to sensor XML
   24 2021-03-16 rdr Add DUST XML handling.
   25 2021-04-07 rdr Add dust_on flag.
   26 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "platform.h"
#include "readpackets.h"
#include "xmlsup.h"
#include "xmlcfg.h"

terr_counts error_counts ; /* Number of errors per section */
BOOLEAN fatal ; /* If XML load had a fatal error */

/* #pragma comment (lib, "Ws2_32.lib") */

#if defined(SECT_NETW) || defined(SECT_ANNC)

/* Cleaned up version derived from BIND 4.9.4 release. Returns
   TRUE if good result */
BOOLEAN inet_pton6 (pchar src, tip_v6 *dst)
{
    static const char xdigits_l[] = "0123456789abcdef",
                                    xdigits_u[] = "0123456789ABCDEF" ;
    U8 tmp[IN6ADDRSZ] ;
    PU8 tp, endp, colonp ;
    pchar xdigits, pch ;
    int ch, saw_xdigit ;
    U16 val ;
    I32 n, i ;

    tp = (PU8)&(tmp) ;
    memclr (tp, IN6ADDRSZ) ;
    endp = tp + IN6ADDRSZ ;
    colonp = NIL ;

    /* Leading :: requires some special handling. */
    if ((*src == ':') && (*++src != ':'))
        return FALSE ;

    saw_xdigit = 0 ;
    val = 0 ;

    while ((ch = *src++) != '\0') {
        if ((pch = strchr ((xdigits = (pchar)xdigits_l), ch)) == NIL)
            pch = strchr ((xdigits = (pchar)xdigits_u), ch) ;

        if (pch) {
            val = val << 4 ;
            val = val | (pch - xdigits) ;

            if (++saw_xdigit > 4)
                return FALSE ;

            continue ;
        }

        if (ch == ':') {
            if (saw_xdigit == 0) {
                if (colonp)
                    return FALSE ;

                colonp = tp ;
                continue ;
            }

            if ((tp + sizeof(U16)) > endp)
                return FALSE ;

            *tp++ = (U8) (val >> 8) & 0xff ;
            *tp++ = (U8) val & 0xff ;
            saw_xdigit = 0 ;
            val = 0 ;
            continue ;
        }

        return FALSE ;
    }

    if (saw_xdigit) {
        if ((tp + sizeof(U16)) > endp)
            return FALSE ;

        *tp++ = (U8) (val >> 8) & 0xff ;
        *tp++ = (U8) val & 0xff ;
    }

    if (colonp) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        if (tp == endp)
            return FALSE ;

        n = tp - colonp ;

        for (i = 1 ; i <= n ; i++) {
            endp[- i] = colonp[n - i] ;
            colonp[n - i] = 0 ;
        }

        tp = endp ;
    }

    if (tp != endp)
        return FALSE ;

    memcpy (dst, &(tmp), IN6ADDRSZ) ;
    return TRUE ;
}

#endif

#ifdef SECT_SENS
tsensors sensors ;
const txmldef XSENSORS[SENSCNT] = {
    {"sensors", X_REC, 0, 0, 0, 0, 0, 0},
    {"sensor", X_ARRAY, 0, /*start*/0x41, /*compact*/FALSE, /*count*/SENSOR_COUNT, /*basesize*/sizeof(tsensor), &(sensors[0].name)},
    {"type", X_SIMP, T_STRING, /*opts*/23, 0, 0, 0, &(sensors[0].name)},
    {"ctrl0", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sensors[0].controls[0])},
    {"ctrl1", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sensors[0].controls[1])},
    {"ctrl2", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sensors[0].controls[2])},
    {"ctrl3", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sensors[0].controls[3])},
    {"ctrl4", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sensors[0].controls[4])},
    {"use_serial", X_SIMP, T_BYTE, 0, 0, 0, 0, &(sensors[0].use_serial)},
    {"highres", X_SIMP, T_BYTE, 0, 0, 0, 0, &(sensors[0].highres)},
    {"options", X_SIMP, T_BYTE, 0, 0, 0, 0, &(sensors[0].options)},
    {"slipip", X_SIMP, T_STRING, /*opts*/40, 0, 0, 0, &(sensors[0].ip_slip_text)},
    {"desc1", X_SIMP, T_STRING, /*opts*/63, 0, 0, 0, &(sensors[0].desc1)},
    {"desc2", X_SIMP, T_STRING, /*opts*/63, 0, 0, 0, &(sensors[0].desc2)},
    {"sensor", X_EARRAY, 0, /*start*/0x41, /*compact*/FALSE, /*count*/SENSOR_COUNT, /*basesize*/sizeof(tsensor), 0},
    {"sensors", X_EREC, 0, 0, 0, 0, 0, 0}
} ;
const string23 FSENSTYPES[SENSTYPE_CNT] = {
    "None", "Multiple Sources", "Generic 3CH", "STS-2", "STS-2.5", "STS-5", "CMG-3T",
    "MBB-2", "Episensor ES-T", "TCompact", "T120", "T240", "STS-6", "STS-2 High-Gain", "STS-5-360",
    "Spare 1", "Spare 2"
} ;
const string23 FCTRLTYPES[CTRLTYPE_CNT] = {
    "None", "Center-Hi", "Center-Lo", "Lock-Hi", "Lock-Lo", "Unlock-Hi",
    "Unlock-Lo", "Deploy-Hi", "Deploy-Lo", "Remove-Hi", "Remove-Lo",
    "CalEnab-Hi", "CalEnab-Lo", "Aux0-Hi", "Aux0-Lo", "Aux1-Hi", "Aux1-Lo",
    "Aux2-Hi", "Aux2-Lo", "Aux3-Hi", "Aux3-Lo", "Aux4-Hi", "Aux4-Lo"
} ;

BOOLEAN read_sensors (void) /* True for section not found */
{
    int i, j, idx ;
    string23 s ;
    U32 ip;

    error_counts[XS_SENS] = 0 ;

    if (read_xml_start ((pchar)&(XSENSORS[0].name))) {
        memclr (&(sensors), sizeof(tsensors)) ;
        read_xml ((pxmldefarray)&(XSENSORS), SENSCNT, XS_SENS) ;
    } else
        return TRUE ;

    /* Now convert sensor type from string to enum and control lines to enum */
    for (i = 0 ; i < SENSOR_COUNT ; i++) {
        idx = match_field ((pfieldarray)&(FSENSTYPES), SENSTYPE_CNT, (pchar)&(sensors[i].name)) ;

        if (idx < 0)
            (error_counts[XS_SENS])++ ;
        else
            sensors[i].senstype = (enum tsenstype)idx ;

        for (j = 0 ; j < CONTROL_COUNT ; j++) {
            strcpy (s, (pchar)&(sensors[i].controls[j])) ;

            if (s[0]) {
                /* is non-null field */
                idx = match_field ((pfieldarray)&(FCTRLTYPES), CTRLTYPE_CNT, s) ;

                if (idx < 0)
                    (error_counts[XS_SENS])++ ;
                else
                    sensors[i].ctrltype[j] = (enum tctrltype)idx ;
            } else
                sensors[i].ctrltype[j] = CT_NONE ;
        }

        ip = inet_addr(sensors[i].ip_slip_text) ;

        if (ip == INADDR_NONE)
            ip = 0 ;

        sensors[i].ip_slip = ip ;
    }

    return FALSE ;
}
#endif

#if defined(SECT_MAIN) || defined(SECT_ACCL)
tdigis digis ;
const tfreqtab FREQTAB = {1, 10, 20, 40, 50, 100, 200, 250, 500, 1000} ;
const string23 LINTYPES[LINTYPE_CNT] = {"All", "100", "40", "20"} ;

/* Convert frequency to bit number, -1 for error */
int freq_to_bit (U16 freq)
{
    int idx ;

    for (idx = 0 ; idx < FREQS ; idx++)
        if (FREQTAB[idx] == freq)
            return idx ;

    return -1 ;
}
#endif

#ifdef SECT_MAIN
const txmldef XMAINDIGIS[MAINCNT] = {
    {"maindigi", X_REC, 0, 0, 0, 0, 0, 0},
    {"chan", X_ARRAY, 0, /*start*/1, /*compact*/FALSE, /*count*/MAIN_CHANNELS, /*basesize*/sizeof(tdigi), &(digis[0].freqmap)},
    {"lowvolt", X_SIMP, T_BYTE, 0, 0, 0, 0, &(digis[0].lowvolt)},
    {"gain", X_SIMP, T_BYTE, 0, 0, 0, 0, &(digis[0].pgagain)},
    {"freqs", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(digis[0].freqlist)},
    {"low_latency", X_SIMP, T_WORD, 0, 0, 0, 0, &(digis[0].lowfreq)},
    {"ll_target", X_SIMP, T_WORD, 0, 0, 0, 0, &(digis[0].target)},
    {"linear_below", X_SIMP, T_STRING, /*opts*/4, 0, 0, 0, &(digis[0].linfreq)},
    {"quick", X_SIMP, T_BYTE, 0, 0, 0,0, &(digis[0].quick_ok)},
    {"chan", X_EARRAY, 0, /*start*/1, /*compact*/FALSE, /*count*/MAIN_CHANNELS, /*basesize*/sizeof(tdigi), 0},
    {"maindigi", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_maindigis (void) /* TRUE if section not found */
{
    int i, idx, good, value ;
    string47 s, src ;
    pchar pc, psrc ;

    error_counts[XS_MAIN] = 0 ;

    if (read_xml_start ((pchar)&(XMAINDIGIS[0].name))) {
        memclr (&(digis[0]), sizeof(tdigi) * MAIN_CHANNELS) ;
        read_xml ((pxmldefarray)&(XMAINDIGIS), MAINCNT, XS_MAIN) ;
    } else
        return TRUE ;

    for (i = 0 ; i < MAIN_CHANNELS ; i++) {
        if (digis[i].pgagain == 0)
            digis[i].pgagain = 1 ; /* reasonable default */

        if (digis[i].freqlist[0] == 0) {
            /* set reasonable default */
            digis[i].target = 250 ;
            continue ; /* This channel not configured */
        }

        /* Now convert linear filter spec from string to enum */
        idx = match_field ((pfieldarray)&(LINTYPES), LINTYPE_CNT, (pchar)&(digis[i].linfreq)) ;

        if (idx < 0)
            (error_counts[XS_MAIN])++ ;
        else
            digis[i].linear = (enum tlinear)idx ;

        /* Convert low latency frequency to index */
        if ((i != CAL_CHANNEL) && (digis[i].lowfreq)) {
            /* Not for cal monitor channel and have a low frequency */
            idx = freq_to_bit (digis[i].lowfreq) ;

            if (idx < 0)
                (error_counts[XS_MAIN])++ ;
            else
                digis[i].lowlat = idx ;
        }

        /* Make sure target in range */
        if (digis[i].target == 0)
            digis[i].target = 250 ; /* reasonable default */
        else if (digis[i].target < 150)
            digis[i].target = 150 ; /* Minimum */
        else if (digis[i].target > 750)
            digis[i].target = 750 ; /* maximum */

        /* Convert frequency list to bitmap */
        strcpy (src, digis[i].freqlist) ; /* make local copy */
        psrc = (pchar)&(src) ;

        do {
            pc = strstr(psrc, ",") ;

            if (pc) {
                *pc = 0 ;
                strncpy (s, psrc, 47) ;
                psrc = psrc + (strlen(s) + 1) ;
            } else
                strncpy (s, psrc, 47) ; /* last one in list */

            good = sscanf(s, "%d", &(value)) ;

            if (good != 1)
                (error_counts[XS_MAIN])++ ;
            else {
                idx = freq_to_bit ((U16)value) ;

                if (idx < 0)
                    (error_counts[XS_MAIN])++ ;
                else
                    digis[i].freqmap = digis[i].freqmap | (1 << idx) ;
            }
        } while (! (pc == NIL)) ;
    }

    return FALSE ;
}
#endif

#ifdef SECT_ACCL
const txmldef XACCEL[ACCLCNT] = {
    {"accel", X_REC, 0, 0, 0, 0, 0, 0},
    {"chan", X_ARRAY, 0, /*start*/1, /*compact*/FALSE, /*count*/ACC_CHANNELS, /*basesize*/sizeof(tdigi), &(digis[7].freqmap)},
    {"freqs", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(digis[7].freqlist)},
    {"low_latency", X_SIMP, T_WORD, 0, 0, 0, 0, &(digis[7].lowfreq)},
    {"ll_target", X_SIMP, T_WORD, 0, 0, 0, 0, &(digis[7].target)},
    {"linear_below", X_SIMP, T_STRING, /*opts*/4, 0, 0, 0, &(digis[7].linfreq)},
    {"quick", X_SIMP, T_BYTE, 0, 0, 0,0, &(digis[7].quick_ok)},
    {"chan", X_EARRAY, 0, /*start*/1, /*compact*/FALSE, /*count*/ACC_CHANNELS, /*basesize*/sizeof(tdigi), 0},
    {"accel", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_accel (void) /* TRUE if section not found */
{
    int i, idx, good, value ;
    string47 src ;
    string23 s ;
    pchar pc, psrc ;

    error_counts[XS_ACCL] = 0 ;

    if (read_xml_start ((pchar)&(XACCEL[0].name))) {
        memclr (&(digis[7]), sizeof(tdigi) * ACC_CHANNELS) ;
        read_xml ((pxmldefarray)&(XACCEL), ACCLCNT, XS_ACCL) ;
    } else
        return TRUE ;

    for (i = MAIN_CHANNELS ; i < TOTAL_CHANNELS ; i++) {
        if (digis[i].freqlist[0] == 0)
            continue ; /* This channel not configured */

        /* Now convert linear filter spec from string to enum */
        idx = match_field ((pfieldarray)&(LINTYPES), LINTYPE_CNT, (pchar)&(digis[i].linfreq)) ;

        if (idx < 0)
            (error_counts[XS_ACCL])++ ;
        else
            digis[i].linear = (enum tlinear)idx ;

        /* Convert low latency frequency to index */
        if (digis[i].lowfreq) {
            idx = freq_to_bit (digis[i].lowfreq) ;

            if (idx < 0)
                (error_counts[XS_ACCL])++ ;
            else
                digis[i].lowlat = idx ;
        }

        /* Make sure target in range */
        if (digis[i].target == 0)
            digis[i].target = 250 ; /* reasonable default */
        else if (digis[i].target < 150)
            digis[i].target = 150 ; /* Minimum */
        else if (digis[i].target > 750)
            digis[i].target = 750 ; /* maximum */

        /* Convert frequency list to bitmap */
        strcpy (src, digis[i].freqlist) ; /* make local copy */
        psrc = (pchar)&(src) ;

        do {
            pc = strstr(psrc, ",") ;

            if (pc) {
                *pc = 0 ;
                strncpy (s, psrc, 23) ;
                psrc = psrc + (strlen(s) + 1) ;
            } else
                strncpy (s, psrc, 23) ; /* last one in list */

            good = sscanf(s, "%d", &(value)) ;

            if (good != 1)
                (error_counts[XS_ACCL])++ ;
            else {
                idx = freq_to_bit ((U16)value) ;

                if (idx < 0)
                    (error_counts[XS_ACCL])++ ;
                else
                    digis[i].freqmap = digis[i].freqmap | (1 << idx) ;
            }
        } while (! (pc == NIL)) ;
    }

    return FALSE ;
}
#endif

#ifdef SECT_SINF
#define SINFCNT 21
tsysinfo sysinfo ;
const txmldef XSINF[SINFCNT] = {
    {"sysinfo", X_REC, 0, 0, 0, 0, 0, 0},
    {"reboots", X_SIMP, T_LINT, 0, 0, 0, 0, &(sysinfo.reboots)},
    {"boot", X_SIMP, T_LWORD, 0, 0, 0, 0, &(sysinfo.boot)},
    {"first", X_SIMP, T_LWORD, 0, 0, 0, 0, &(sysinfo.first)},
    {"be_boot", X_SIMP, T_LWORD, 0, 0, 0, 0, &(sysinfo.beboot)},
    {"ss_boot", X_SIMP, T_LWORD, 0, 0, 0, 0, &(sysinfo.ssboot)},
    {"spslimit", X_SIMP, T_LINT, 0, 0, 0, 0, &(sysinfo.spslimit)},
    {"clock_serial", X_SIMP, T_LINT, 0, 0, 0, 0, &(sysinfo.clock_serial)},
    {"BE_version", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.be_ver)},
    {"FE_version", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.fe_ver)},
    {"SS_version", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.ss_ver)},
    {"clock_type", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.clk_typ)},
    {"sensorA_type", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.sa_type)},
    {"sensorB_type", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.sb_type)},
    {"pld_ver", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(sysinfo.pld_ver)},
    {"clock_ver", X_SIMP, T_STRING, /*opts*/63, 0, 0, 0, &(sysinfo.clock_ver)},
    {"property_tag", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(sysinfo.prop_tag)},
    {"FE_serial", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(sysinfo.fe_ser)},
    {"last_reboot", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(sysinfo.last_rb)},
    {"first_boot", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(sysinfo.first_bt)},
    {"sysinfo", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_sysinfo (void) /* TRUE if section not found */
{

    error_counts[XS_SINF] = 0 ;

    if (read_xml_start ((pchar)&(XSINF[0].name))) {
        memclr (&(sysinfo), sizeof(tsysinfo)) ;
        read_xml ((pxmldefarray)&(XSINF), SINFCNT, XS_SINF) ;
    } else
        return TRUE ;

    return FALSE ;
}
#endif

#ifdef SECT_WRIT
twritercfg writercfg ;
const txmldef XWRIT[WRITCNT] = {
    {"writer", X_REC, 0, 0, 0, 0, 0, 0},
    {"proto_ver", X_SIMP, T_WORD, 0, 0, 0, 0, &(writercfg.proto_ver)},
    {"name", X_SIMP, T_STRING, /*opts*/32, 0, 0, 0, &(writercfg.name)},
    {"soft_ver", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(writercfg.version)},
    {"created", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(writercfg.created)},
    {"updated", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(writercfg.updated)},
    {"writer", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_writer (void) /* TRUE if section not found */
{

    error_counts[XS_WRIT] = 0 ;

    if (read_xml_start ((pchar)&(XWRIT[0].name))) {
        memclr (&(writercfg), sizeof(twritercfg)) ;
        read_xml ((pxmldefarray)&(XWRIT), WRITCNT, XS_WRIT) ;
    } else
        return TRUE ;

    return FALSE ;
}
#endif

#ifdef SECT_TIME
ttiming timing ;
const txmldef XTIME[TIMECNT] = {
    {"timing", X_REC, 0, 0, 0, 0, 0, 0},
    {"engine", X_SIMP, T_STRING, /*opts*/16, 0, 0, 0, &(timing.name)},
    {"cycled", X_SIMP, T_BYTE, 0, 0, 0, 0, &(timing.cycled)},
    {"intant", X_SIMP, T_BYTE, 0, 0, 0, 0, &(timing.intant)},
    {"boost", X_SIMP, T_BYTE, 0, 0, 0, 0, &(timing.boost)},
    {"dataon", X_SIMP, T_BYTE, 0, 0, 0, 0, &(timing.dataon)},
    {"tempon", X_SIMP, T_BYTE, 0, 0, 0, 0, &(timing.tempon)},
    {"hours", X_SIMP, T_STRING, /*opts*/95, 0, 0, 0, &(timing.hours_text)},
    {"tcxooff", X_SIMP, T_INT, 0, 0, 0, 0, &(timing.tcxo_off)},
    {"temptol", X_SIMP, T_SINGLE, /*prec*/1, 0, 0, 0, &(timing.temp_tol)},
    {"timing", X_EREC, 0, 0, 0, 0, 0, 0}
} ;
#define ENGTYPE_CNT 4
const string23 FENGTYPES[ENGTYPE_CNT] = {
    "Internal", "External", "Int-Mediatek", "Int-MAX8"
} ;

BOOLEAN read_timing (void) /* TRUE if section not found */
{
    int idx, good, value ;
    string95 src ;
    string31 s ;
    pchar pc, psrc ;

    error_counts[XS_TIME] = 0 ;

    if (read_xml_start ((pchar)&(XTIME[0].name))) {
        memclr (&(timing), sizeof(ttiming)) ;
        read_xml ((pxmldefarray)&(XTIME), TIMECNT, XS_TIME) ;
    } else
        return TRUE ;

    /* Now convert engine type from string to enum */
    idx = match_field ((pfieldarray)&(FENGTYPES), ENGTYPE_CNT, (pchar)&(timing.name)) ;

    if (idx < 0)
        (error_counts[XS_TIME])++ ;
    else
        timing.enginetype = (enum tenginetype)idx ;

    /* Convert hours list to binary if cycled and there is a list */
    if ((timing.cycled) && (timing.hours_text[0])) {
        strcpy (src, timing.hours_text) ; /* make local copy */
        psrc = (pchar)&(src) ;

        do {
            pc = strstr(psrc, ",") ;

            if (pc) {
                *pc = 0 ;
                strncpy (s, psrc, 23) ;
                psrc = psrc + (strlen(s) + 1) ;
            } else
                strncpy (s, psrc, 23) ; /* last one in list */

            good = sscanf(s, "%d", &(value)) ;

            if (good != 1)
                (error_counts[XS_TIME])++ ;
            else {
                if ((value < 0) || (value > 23))
                    (error_counts[XS_TIME])++ ;
                else
                    timing.hourmap = timing.hourmap | (1 << value) ; /* Add to bitmap */
            }
        } while (! (pc == NIL)) ;
    }

    /* Make sure temperature tolerance is not zero */
    if (timing.temp_tol < 0.199)
        timing.temp_tol = 0.2 ;

    return FALSE ;
}
#endif

#ifdef SECT_OPTS
toptions options ;
string iso_lists[ISO_COUNT] ; /* For reading */
const txmldef XOPTS[OPTCNT] = {
    {"options", X_REC, 0, 0, 0, 0, 0, 0},
    {"enable_eng", X_SIMP, T_BYTE, 0, 0, 0, 0, &(options.eng_on)},
    {"iso_mode", X_SIMP, T_BYTE, 0, 0, 0, 0, &(options.iso_mode)},
    {"iso0", X_SIMP, T_STRING, /*opts*/250, 0, 0, 0, &(iso_lists[0])},
    {"iso1", X_SIMP, T_STRING, /*opts*/250, 0, 0, 0, &(iso_lists[1])},
    {"iso2", X_SIMP, T_STRING, /*opts*/250, 0, 0, 0, &(iso_lists[2])},
    {"iso3", X_SIMP, T_STRING, /*opts*/250, 0, 0, 0, &(iso_lists[3])},
    {"wosenab", X_SIMP, T_BYTE, 0, 0, 0, 0, &(options.wos_enable)},
    {"wosfilt", X_SIMP, T_BYTE, 0, 0, 0, 0, &(options.wos_filter)},
    {"wos_thresh", X_SIMP, T_SINGLE, /*prec*/3, 0, 0, 0, &(options.wos_threshold)},
    {"wos_mindur", X_SIMP, T_SINGLE, /*prec*/1, 0, 0, 0, &(options.wos_mindur)},
    {"options", X_EREC, 0, 0, 0, 0, 0, 0}
} ;
const string15 FOPTISOS[OPT_ISO_CNT] = {"CONT", "DHCP"} ;

int iso_to_bit (pchar pc)
{
    int idx ;

    for (idx = 0 ; idx < OPT_ISO_CNT ; idx++)
        if (strcmp((pchar)&(FOPTISOS[idx]), pc) == 0)
            return idx ;

    return -1 ;
}

BOOLEAN read_options (void) /* TRUE if section not found */
{
    int idx, max, i ;
    U16 mask ;
    string src ;
    string63 s ;
    pchar psrc, pc ;

    error_counts[XS_OPTS] = 0 ;

    if (read_xml_start ((pchar)&(XOPTS[0].name))) {
        memclr (&(options), sizeof(toptions)) ;
        read_xml ((pxmldefarray)&(XOPTS), OPTCNT, XS_OPTS) ;

        /* Convert option lists to bitmap */
        if (options.iso_mode)
            max = 3 ;
        else
            max = 1 ;

        for (idx = 0 ; idx <= max ; idx++) {
            mask = 0 ;
            strcpy (src, iso_lists[idx]) ;
            q660_upper (src) ;
            psrc = (pchar)&(src) ;

            do {
                pc = strstr(psrc, ",") ;

                if (pc) {
                    *pc = 0 ;
                    strncpy (s, psrc, 15) ;
                    psrc = psrc + (strlen(s) + 1) ;
                } else
                    strncpy (s, psrc, 47) ;

                if (s[0]) {
                    /* Have something */
                    i = iso_to_bit (s) ;

                    if (i < 0)
                        (error_counts[XS_OPTS])++ ;
                    else
                        mask = mask | (1 << i) ;
                }
            } while (! (pc == NIL)) ;

            options.iso_opts[idx] = mask ;
        }
    } else
        return TRUE ;

    return FALSE ;
}
#endif

#ifdef SECT_NETW
tnetwork network ;
const txmldef XNETWORK[NETCNT] = {
    {"network", X_REC, 0, 0, 0, 0, 0, 0},
    {"cycled", X_SIMP, T_BYTE, 0, 0, 0, 0, &(network.cycled)},
    {"ipv6", X_SIMP, T_BYTE, 0, 0, 0, 0, &(network.ipv6)},
    {"baseport", X_SIMP, T_WORD, 0, 0, 0, 0, &(network.baseport)},
    {"prefix_length", X_SIMP, T_WORD, 0, 0, 0, 0, &(network.prefix_lth)},
    {"hours", X_SIMP, T_STRING, /*opts*/95, 0, 0, 0, &(network.hours_text)},
    {"manual", X_SIMP, T_BYTE, 0, 0, 0, 0, &(network.manual)},
    {"ip", X_SIMP, T_STRING, /*opts*/40, 0, 0, 0, &(network.ip_text)},
    {"router", X_SIMP, T_STRING, /*opts*/40, 0, 0, 0, &(network.router_text)},
    {"mask", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(network.mask_text)},
    {"usemtu", X_SIMP, T_BYTE, 0, 0, 0, 0, &(network.usemtu)},
    {"usedns", X_SIMP, T_BYTE, 0, 0, 0, 0, &(network.usedns)},
    {"mtu", X_SIMP, T_WORD, 0, 0, 0, 0, &(network.mtu)},
    {"dns1", X_SIMP, T_STRING, /*opts*/40, 0, 0, 0, &(network.dns1_text)},
    {"dns2", X_SIMP, T_STRING, /*opts*/40, 0, 0, 0, &(network.dns2_text)},
    {"network", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_network (void) /* TRUE if section not found */
{
    int good, value ;
    BOOLEAN nosect ;
    string95 src ;
    string23 s ;
    pchar pc, psrc ;
    U32 ip ;
    tip_v6 tmp6 ;

    error_counts[XS_NETW] = 0 ;
    nosect = FALSE ;

    if (read_xml_start ((pchar)&(XNETWORK[0].name))) {
        memclr (&(network), sizeof(tnetwork)) ;
        network.prefix_lth = 64 ; /* default is 64, not zero */
        network.baseport = 7330 ;
        network.mtu = 1500 ;
        read_xml ((pxmldefarray)&(XNETWORK), NETCNT, XS_NETW) ;
    } else
        nosect = TRUE ;

    if (network.mtu == 0)
        network.mtu = 1500 ;

    if (network.prefix_lth == 0)
        network.prefix_lth = 64 ;

    if (network.baseport == 0)
        network.baseport = 7330 ;

    if (nosect)
        return TRUE ;

    /* Convert hours list to binary if cycled and there is a list */
    if ((network.cycled) && (network.hours_text[0])) {
        strcpy (src, network.hours_text) ; /* make local copy */
        psrc = (pchar)&(src) ;

        do {
            pc = strstr(psrc, ",") ;

            if (pc) {
                *pc = 0 ;
                strncpy (s, psrc, 23) ;
                psrc = psrc + (strlen(s) + 1) ;
            } else
                strncpy (s, psrc, 23) ; /* last one in list */

            good = sscanf(s, "%d", &(value)) ;

            if (good != 1)
                (error_counts[XS_NETW])++ ;
            else {
                if ((value < 0) || (value > 23))
                    (error_counts[XS_NETW])++ ;
                else
                    network.hourmap = network.hourmap | (1 << value) ; /* Add to bitmap */
            }
        } while (! (pc == NIL)) ;
    }

    if (network.ipv6) {
        /* Process IPV6 fields */
        if (inet_pton6 (network.ip_text, &(tmp6)))
            memcpy (&(network.ip_v6), &(tmp6), IN6ADDRSZ) ;
        else
            (error_counts[XS_NETW])++ ;

        if (inet_pton6 (network.router_text, &(tmp6)))
            memcpy (&(network.router_v6), &(tmp6), IN6ADDRSZ) ;
        else
            (error_counts[XS_NETW])++ ;

        if (network.dns1_text[0]) {
            /* Have one */
            if (inet_pton6 (network.dns1_text, &(tmp6)))
                memcpy (&(network.dns1_v6), &(tmp6), IN6ADDRSZ) ;
            else if (network.usedns)
                (error_counts[XS_NETW])++ ; /* only care if enabled */
        }

        if (network.dns2_text[0]) {
            /* Have one */
            if (inet_pton6 (network.dns2_text, &(tmp6)))
                memcpy (&(network.dns2_v6), &(tmp6), IN6ADDRSZ) ;
            else if (network.usedns)
                (error_counts[XS_NETW])++ ; /* only care if enabled */
        }
    } else {
        /* Process IPV4 fields */
        ip = inet_addr(network.ip_text) ;

        if (ip == INADDR_NONE)
            (error_counts[XS_NETW])++ ;
        else
            network.ip_v4 = ip ;

        ip = inet_addr(network.router_text) ;

        if (ip == INADDR_NONE)
            (error_counts[XS_NETW])++ ;
        else
            network.router_v4 = ip ;

        ip = inet_addr(network.mask_text) ;

        if (ip == INADDR_NONE)
            (error_counts[XS_NETW])++ ;
        else
            network.mask = ip ;

        if (network.dns1_text[0]) {
            /* Have one */
            ip = inet_addr(network.dns1_text) ;

            if (ip != INADDR_NONE)
                network.dns1_v4 = ip ;
            else if (network.usedns)
                (error_counts[XS_NETW])++ ; /* only care if enabled */
        }

        if (network.dns2_text[0]) {
            /* Have one */
            ip = inet_addr(network.dns2_text) ;

            if (ip != INADDR_NONE)
                network.dns2_v4 = ip ;
            else if (network.usedns)
                (error_counts[XS_NETW])++ ; /* only care if enabled */
        }
    }

    return FALSE ;
}
#endif

#ifdef SECT_ANNC
tannc announce ;
const txmldef XANNC[ANNCCNT] = {
    {"announce", X_REC, 0, 0, 0, 0, 0, 0},
    {"ggate", X_SIMP, T_BYTE, 0, 0, 0, 0, &(announce.ggate)},
    {"gtimeout", X_SIMP, T_WORD, 0, 0, 0, 0, &(announce.gtimeout)},
    {"gresume", X_SIMP, T_WORD, 0, 0, 0, 0, &(announce.gresume)},
    {"poc", X_ARRAY, 0, /*start*/1, /*compact*/FALSE, /*count*/MAX_ANNC, /*basesize*/sizeof(tdest), &(announce.dests[0].ip_v6)},
    {"ip", X_SIMP, T_STRING, /*opts*/40, 0, 0, 0, &(announce.dests[0].ip_text)},
    {"port", X_SIMP, T_WORD, 0, 0, 0, 0, &(announce.dests[0].port)},
    {"timeout", X_SIMP, T_WORD, 0, 0, 0, 0, &(announce.dests[0].timeout)},
    {"resume", X_SIMP, T_WORD, 0, 0, 0, 0, &(announce.dests[0].resume)},
    {"interval", X_SIMP, T_WORD, 0, 0, 0, 0, &(announce.dests[0].interval)},
    {"valid", X_SIMP, T_BYTE, 0, 0, 0, 0, &(announce.dests[0].valid)},
    {"random", X_SIMP, T_BYTE, 0, 0, 0, 0, &(announce.dests[0].random)},
    {"stop", X_SIMP, T_BYTE, 0, 0, 0, 0, &(announce.dests[0].stop)},
    {"poc", X_EARRAY, 0, /*start*/1, /*compact*/FALSE, /*count*/MAX_ANNC, /*basesize*/sizeof(tdest), 0},
    {"announce", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_announce (void) /* TRUE if section not found */
{
    U32 ip ;
    int idx ;
    tip_v6 tmp6 ;
    pchar pc ;

    error_counts[XS_ANNC] = 0 ;

    if (read_xml_start ((pchar)&(XANNC[0].name))) {
        memclr (&(announce), sizeof(tannc)) ;
        read_xml ((pxmldefarray)&(XANNC), ANNCCNT, XS_ANNC) ;
    } else
        return TRUE ;

    /* Convert IP Address for each valid entry */
    for (idx = 0 ; idx < MAX_ANNC ; idx++)
        if (announce.dests[idx].valid) {
            if (network.ipv6) {
                /* Process IPV6 field */
                pc = strchr ((pchar)&(announce.dests[idx].ip_text), ':') ;

                if (pc) {
                    /* parse it if it is an address, else ignore it */
                    if (inet_pton6 ((pchar)&(announce.dests[idx].ip_text), &(tmp6)))
                        memcpy (&(announce.dests[idx].ip_v6), &(tmp6), IN6ADDRSZ) ;
                    else
                        (error_counts[XS_ANNC])++ ;
                }
            } else {
                /* Process IPV4 field */
                ip = inet_addr((pchar)&(announce.dests[idx].ip_text)) ;

                if (ip == INADDR_NONE)
                    (error_counts[XS_ANNC])++ ;
                else
                    announce.dests[idx].ip_v4 = ip ;
            }
        }

    return FALSE ;
}
#endif

#ifdef SECT_AUTO
tamass automass ;
const txmldef XAUTO[AUTOCNT] = {
    {"automass", X_REC, 0, 0, 0, 0, 0, 0},
    {"sensor", X_ARRAY, 0, /*start*/0x41, /*compact*/FALSE, /*count*/SENSOR_COUNT, /*basesize*/sizeof(taone), &(automass[0].tol[0])},
    {"tol_1", X_SIMP, T_WORD, 0, 0, 0, 0, &(automass[0].tol[0])},
    {"tol_2", X_SIMP, T_WORD, 0, 0, 0, 0, &(automass[0].tol[1])},
    {"tol_3", X_SIMP, T_WORD, 0, 0, 0, 0, &(automass[0].tol[2])},
    {"max_try", X_SIMP, T_WORD, 0, 0, 0, 0, &(automass[0].max_try)},
    {"norm_int", X_SIMP, T_WORD, 0, 0, 0, 0, &(automass[0].norm_int)},
    {"squelch_int", X_SIMP, T_WORD, 0, 0, 0, 0, &(automass[0].squelch_int)},
    {"duration", X_SIMP, T_SINGLE, /*prec*/2, 0, 0, 0, &(automass[0].duration)},
    {"sensor", X_EARRAY, 0, /*start*/0x41, /*compact*/FALSE, /*count*/SENSOR_COUNT, /*basesize*/sizeof(taone), 0},
    {"automass", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_automass (void) /* TRUE if section not found */
{

    error_counts[XS_AUTO] = 0 ;

    if (read_xml_start ((pchar)&(XAUTO[0].name))) {
        memclr (&(automass), sizeof(tamass)) ;
        read_xml ((pxmldefarray)&(XAUTO), AUTOCNT, XS_AUTO) ;
    } else
        return TRUE ;

    return FALSE ;
}
#endif

#ifdef SECT_DUST
tdust_devs dust_devs ;
const txmldef XDUST[DUSTCNT] = {
    {"dust", X_REC, 0, 0, 0, 0, 0, 0},
    {"network", X_SIMP, T_WORD, 0, 0, 0, 0, &(dust_devs.netid)},
    {"dust_on", X_SIMP, T_BYTE, 0, 0, 0, 0, &(dust_devs.flags)},
    {"device", X_ARRAY, 0, /*start*/0x31, /*compact*/FALSE, /*count*/DUST_COUNT, /*basesize*/sizeof(tone_dust), &(dust_devs.dusts[0])},
    {"data_on", X_SIMP, T_BYTE, 0, 0, 0, 0, &(dust_devs.dusts[0].data)},
    {"status_on", X_SIMP, T_BYTE, 0, 0, 0, 0, &(dust_devs.dusts[0].status)},
    {"tag_id", X_SIMP, T_LWORD, 0, 0, 0, 0, &(dust_devs.dusts[0].tag_id)},
    {"device", X_EARRAY, 0, /*start*/0x31, /*compact*/FALSE, /*count*/DUST_COUNT, /*basesize*/sizeof(tone_dust), 0},
    {"dust", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

BOOLEAN read_dust (void) /* TRUE if section not found */
{

    error_counts[XS_DUST] = 0 ;

    if (read_xml_start ((pchar)&(XDUST[0].name))) {
        memclr (&(dust_devs), sizeof(tdust_devs)) ;
        read_xml ((pxmldefarray)&(XDUST), DUSTCNT, XS_DUST) ;
    } else
        return TRUE ;

    return FALSE ;
}
#endif

