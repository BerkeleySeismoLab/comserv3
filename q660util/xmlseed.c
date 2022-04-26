



/*
     XML Read Configuration Routines
     Copyright 2016-2021 by
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
    0 2016-01-07 rdr Created.
    1 2016-02-16 rdr Add conversion of channel evt_list to evt_map.
    2 2016-03-01 rdr Add xdispatch array, most programs will need it.
    3 2016-03-02 rdr Add Run option for detectors.
    4 2016-03-04 rdr Add setting std_num for threshold detectors. At this point
                     there are no STA/LTA standard detectors defined.
    5 2016-10-18 rdr Use an incrementing number for decimated channels rather
                     relying on the sortlink in GDS_DEC dispatch.
    6 2017-02-13 rdr If there are errors in a SEED channel entry, don't add to list.
                     It used to add channels where there was no data source.
    7 2017-03-27 rdr Sensor temps removed from SOH fields.
    8 2017-06-11 rdr Add describe channel.
    9 2017-06-14 rdr Move setting pxmlmem to initialize_xml_reader.
   10 2018-02-25 jms remove omission of load_cfg_xml when PROG_DL is defined
   11 2018-04-06 rdr Remove describe_channel and associated consts.
   12 2018-05-06 rdr Change string BOOM_WARNING to 'BOOM WARNING'.
   13 2018-05-08 rdr Add reading of exclusion list.
   14 2018-07-24 rdr Fix CR/LF versus LF only detection in XML file.
   15 2018-10-09 rdr Add reading FIR chains.
   16 2019-06-19 rdr Add GPS data fields.
   17 2021-03-25 rdr Add DUST data fields.
   18 2021-07-27 rdr Fix reading double digit Dust channel numbers.
   19 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "platform.h"
#include "xmlsup.h"
#include "xmlcfg.h"
#include "xmlseed.h"

#ifdef SECT_SEED
piirdef xiirchain ;
pchan chanchain ;
pchan dlchain ;

tseed seedbuf ;
const txmldef XSEED[SEEDCNT] = {
    {"cfgname", X_SIMP, T_STRING, /*opts*/250, 0, 0, 0, &(seedbuf.cfgname)},
    {"network", X_SIMP, T_STRING, /*opts*/2, 0, 0, 0, &(seedbuf.network)},
    {"station", X_SIMP, T_STRING, /*opts*/5, 0, 0, 0, &(seedbuf.station)}
} ;

tiirdef curiir ;
const txmldef XIIR[IIRCNT] = {
    {"iir", X_REC, 0, 0, 0, 0, 0, 0},
    {"name", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curiir.fname)},
    {"sections", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curiir.sects)},
    {"gain", X_SIMP, T_SINGLE, /*opts*/7, 0, 0, 0, &(curiir.gain)},
    {"reffreq", X_SIMP, T_SINGLE, /*opts*/7, 0, 0, 0, &(curiir.rate)},
    {"sect", X_ARRAY, 0, /*start*/0, /*compact*/FALSE, /*count*/MAXSECTIONS, /*basesize*/sizeof(tsection_base), &(curiir.filt[0].poles)},
    {"cutratio", X_SIMP, T_SINGLE, /*opts*/7, 0, 0, 0, &(curiir.filt[0].ratio)},
    {"poles", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curiir.filt[0].poles)},
    {"high", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curiir.filt[0].highpass)},
    {"sect", X_EARRAY, 0, /*start*/0, /*compact*/FALSE, /*count*/MAXSECTIONS, /*basesize*/sizeof(tsection_base), 0},
    {"iir", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

tchan curchan ;
#define CHANCNT 21
const txmldef XCHAN[CHANCNT] = {
    {"chan", X_REC, 0, 0, 0, 0, 0, 0},
    {"name", X_SIMP, T_STRING, /*opts*/6, 0, 0, 0, &(curchan.seedname)},
    {"source", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(curchan.source)},
    {"desc", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curchan.desc)},
    {"decsource", X_SIMP, T_STRING, /*opts*/6, 0, 0, 0, &(curchan.decsrc)},
    {"decfilt", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curchan.decfilt)},
    {"units", X_SIMP, T_STRING, /*opts*/23, 0, 0, 0, &(curchan.units)},
    {"evt_only", X_SIMP, T_STRING, /*opts*/7, 0, 0, 0, &(curchan.evt_list)},
    {"exclude", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curchan.excl_list)},
    {"fir", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, &(curchan.fir_chain)},
    {"latency", X_SIMP, T_WORD, 0, 0, 0, 0, &(curchan.latency)},
    {"cal_trig", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curchan.cal_trig)},
    {"no_output", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curchan.no_output)},
    {"disable", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curchan.disable)},
    {"mask", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curchan.mask)},
    {"delay", X_SIMP, T_DOUBLE, /*prec*/6, 0, 0, 0, &(curchan.delay)},
    {"sta_lta", X_REC, 0, 0, 0, 0, 0, 0},
    {"sta_lta", X_EREC, 0, 0, 0, 0, 0, 0},
    {"threshold", X_REC, 0, 0, 0, 0, 0, 0},
    {"threshold", X_EREC, 0, 0, 0, 0, 0, 0},
    {"chan", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

tdetector curdet ;
tstaltacfg slcfg ;
tthreshcfg thrcfg ;
static int det_count ;
#define STALTAIDX 16
#define THRIDX 18
const txmldef XSTALTA[SLCNT] = {
    {"sta_lta", X_REC, 0, 0, 0, 0, 0, 0},
    {"detname", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curdet.dname)},
    {"filter", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curdet.fname)},
    {"run", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curdet.run)},
    {"ratio", X_SIMP, T_SINGLE, /*prec*/2, 0, 0, 0, &(slcfg.ratio)},
    {"quiet", X_SIMP, T_SINGLE, /*prec*/2, 0, 0, 0, &(slcfg.quiet)},
    {"sta_win", X_SIMP, T_WORD, 0, 0, 0, 0, &(slcfg.sta_win)},
    {"lta_mult", X_SIMP, T_WORD, 0, 0, 0, 0, &(slcfg.lta_mult)},
    {"pre", X_SIMP, T_WORD, 0, 0, 0, 0, &(slcfg.pre_event)},
    {"post", X_SIMP, T_WORD, 0, 0, 0, 0, &(slcfg.post_event)},
    {"sta_lta", X_EREC, 0, 0, 0, 0, 0, 0}
} ;
const txmldef XTHRESH[THRCNT] = {
    {"threshold", X_REC, 0, 0, 0, 0, 0, 0},
    {"detname", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curdet.dname)},
    {"filter", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curdet.fname)},
    {"run", X_SIMP, T_BYTE, 0, 0, 0, 0, &(curdet.run)},
    {"high", X_SIMP, T_LINT, 0, 0, 0, 0, &(thrcfg.hi_thresh)},
    {"low", X_SIMP, T_LINT, 0, 0, 0, 0, &(thrcfg.lo_thresh)},
    {"hysteresis", X_SIMP, T_LINT, 0, 0, 0, 0, &(thrcfg.hysteresis)},
    {"window", X_SIMP, T_LINT, 0, 0, 0, 0, &(thrcfg.window)},
    {"pre", X_SIMP, T_WORD, 0, 0, 0, 0, &(thrcfg.pre_event)},
    {"post", X_SIMP, T_WORD, 0, 0, 0, 0, &(thrcfg.post_event)},
    {"threshold", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

const txmldef XDLCHAN[DLCHCNT] = {
    {"dlchan", X_REC, 0, 0, 0, 0, 0, 0},
    {"name", X_SIMP, T_STRING, /*opts*/6, 0, 0, 0, &(curchan.seedname)},
    {"source", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, &(curchan.source)},
    {"desc", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, &(curchan.desc)},
    {"dlchan", X_EREC, 0, 0, 0, 0, 0, 0}
} ;

const ttimfields TIMFIELDS = {"PHASE", "CLKQUAL", "CLKLOSS"} ;
const tsohfields SOHFIELDS = {"ANTCUR", "SENSACUR", "SENSBCUR",
                              "BOOM1", "BOOM2", "BOOM3", "BOOM4", "BOOM5", "BOOM6", "SYSTEMP", "HUMIDITY",
                              "INPVOLT", "VCOCTRL", "NEGAN", "ISODC", "GPIN1", "GPIN2", "SYSCUR", "UPSVOLT",
                              "ANTVOLT", "GPOUT", "PKTBUF"
                             } ;
const tengfields ENGFIELDS = {"GPSSENS", "GPSCTRL", "SIOCTRLA", "SIOCTRLB",
                              "SENSIO", "MISCIO", "DUSTIO", "GPSHAL", "GPSSTATE"
                             } ;
const tgpsfields GPSFIELDS = {"SATUSED", "FIXTYPE", "LATITUDE", "LONGITUDE", "ELEVATION"} ;
const tlogfields LOGFIELDS = {"MESSAGES", "CONFIG", "TIMING", "DATAGAPS", "REBOOTS",
                              "RECVBPS", "SENTBPS", "COMATTEMPTS", "COMSUCCESS", "PKTSRECV", "COMEFF", "POCRECV",
                              "IPCHANGES", "COMDUTY", "THROUGHPUT", "MISSDATA", "SEQERR", "CHKERR",
                              "DATALAT", "LLDATALAT", "STATLAT"
                             } ;

txdispatch xdispatch ;
txmdispatch xmdispatch ;
int chan_count ;
U8 dec_count ;
int xml_file_size ;

void add_dispatch (pchan pl)
{
    pchan p ;
    int i, j ;

    i = (int)pl->gen_src ;
    j = pl->sub_field ;

    if (xdispatch[i][j] == NIL)
        xdispatch[i][j] = pl ;
    else {
        p = xdispatch[i][j] ;

        while (p->sortlink)
            p = p->sortlink ;

        p->sortlink = pl ;
    }
}

void add_mdispatch (pchan pl, int offset)
{
    pchan p ;
    int i, j ;

    i = (pl->sub_field >> 4) + offset ; /* get channel */
    j = pl->freqnum ; /* frequency bit */

    if (xmdispatch[i][j] == NIL)
        xmdispatch[i][j] = pl ;
    else {
        p = xmdispatch[i][j] ;

        while (p->sortlink)
            p = p->sortlink ;

        p->sortlink = pl ;
    }
}

void set_dispatch (pchan pl)
{

    switch (pl->gen_src) {
    case GDS_MD :
        add_mdispatch (pl, 0) ;
        break ;

    case GDS_CM :
        add_mdispatch (pl, CAL_CHANNEL) ;
        break ;

    case GDS_AC :
        add_mdispatch (pl, MAIN_CHANNELS) ;
        break ;

    default :
        add_dispatch (pl) ;
        break ;
    }
}

static void read_xml_iir (void)
{
    piirdef piir ;
    int i ;

    memclr (&(curiir), sizeof(tiirdef)) ;
    cur_sect = XS_IIR ;

    while (! read_next_tag ())
        if ((! startflag) && (endflag) &&
                (strcmp(tag, "iir") == 0))

        {
            load_map = load_map | (1L << XS_IIR) ;
            getbuf (pxmlmem, (pvoid)&(piir), sizeof(tiirdef)) ;
            memcpy (piir, &(curiir), sizeof(tiirdef)) ;

            if (xiirchain == NIL)
                xiirchain = piir ;
            else
                xiirchain = extend_link (xiirchain, piir) ;

            return ;
        } else {
            i = match_tag ((pxmldefarray)&(XIIR), IIRCNT) ;

            if (i >= 0)
                proc_tag ((pxmldef)&(XIIR[i])) ;
            else
                (error_counts[XS_IIR])++ ;
        }
}

static void lookup_det_filter (void)
{
    piirdef piir ;
    string31 s1, s2 ;

    curdet.fptr = NIL ;
    strcpy (s1, curdet.fname) ;

    if (strlen(s1) == 0)
        return ; /* nothing to look up */

    q660_upper (s1) ;
    piir = xiirchain ;

    while (piir) {
        strcpy (s2, piir->fname) ;
        q660_upper (s2) ;

        if (strcmp (s1, s2) == 0) {
            curdet.fptr = piir ;
            return ; /* found it */
        }

        piir = piir->link ;
    }

    (error_counts[cur_sect])++ ; /* not found */
}

static void read_stalta (void)
{
    int i ;
    pdetector pd ;

    memclr (&(curdet), sizeof(tdetector)) ;
    memclr (&(slcfg), sizeof(tstaltacfg)) ;
    curdet.dtype = DT_STA_LTA ;

    while (! read_next_tag ())
        if ((! startflag) && (endflag) && (strcmp(tag, "sta_lta") == 0)) {
            memcpy (&(curdet.cfg), &(slcfg), sizeof(tstaltacfg)) ; /* fill in parameters */
            lookup_det_filter () ;
            getbuf (pxmlmem, (pvoid)&(pd), sizeof(tdetector)) ; /* get a buffer for the detector */
            memcpy (pd, &(curdet), sizeof(tdetector)) ; /* and copy into */
            curchan.detptrs[det_count] = pd ;
            det_count++ ;
            break ;
        } else {
            i = match_tag ((pxmldefarray)&(XSTALTA), SLCNT) ;

            if (i >= 0)
                proc_tag ((pxmldef)&(XSTALTA[i])) ;
            else
                (error_counts[cur_sect])++ ;
        }
}

static void read_thresh (void)
{
    int i ;
    pdetector pd ;

    memclr (&(curdet), sizeof(tdetector)) ;
    memclr (&(thrcfg), sizeof(tthreshcfg)) ;
    curdet.dtype = DT_THRESHOLD ;

    while (! read_next_tag ())
        if ((! startflag) && (endflag) && (strcmp(tag, "threshold") == 0)) {
            memcpy (&(curdet.cfg), &(thrcfg), sizeof(tthreshcfg)) ; /* fill in parameters */
            lookup_det_filter () ;

            if (strcmp(curdet.dname, "BOOM WARNING") == 0)
                curdet.std_num = 3 ;
            else if (strcmp(curdet.dname, "OVERTEMP") == 0)
                curdet.std_num = 1 ;
            else if (strcmp(curdet.dname, "ANTCUR") == 0)
                curdet.std_num = 2 ;

            getbuf (pxmlmem, (pvoid)&(pd), sizeof(tdetector)) ; /* get a buffer for the detector */
            memcpy (pd, &(curdet), sizeof(tdetector)) ; /* and copy into */
            curchan.detptrs[det_count] = pd ;
            det_count++ ;
            break ;
        } else {
            i = match_tag ((pxmldefarray)&(XTHRESH), THRCNT) ;

            if (i >= 0)
                proc_tag ((pxmldef)&(XTHRESH[i])) ;
            else
                (error_counts[cur_sect])++ ;
        }

    return ;
}

static int match_source (pchar list, int count) /* returns -1 if no match, else index */
{
    int i ;

    for (i = 0 ; i < count ; i++)
        if (strcmp (curchan.source, list) == 0)
            return i ; /* index in list */
        else
            list = list + (sizeof(string31)) ;

    return -1 ;
}

static void read_xml_chan (void)
{
    pchan pch, pdec ;
    pchar pc, psrc ;
    int i, j, good ;
    string31 s, src ;
    float f ;
    terr_counts beg_errs ;

    memclr (&(curchan), sizeof(tchan)) ;
    det_count = 0 ;
    cur_sect = XS_MNAC ;
    memcpy (&(beg_errs), &(error_counts), sizeof(terr_counts)) ;

    while (! read_next_tag ())
        if ((! startflag) && (endflag) &&
                (strcmp(tag, "chan") == 0))

        {
            /* Process source string */
            q660_upper (curchan.source) ; /* These should all be q660_upper, make sure */

            while (TRUE) {
                /* this is a bit of a cheat to avoid deeply nested elses */
                i = match_source ((pchar)TIMFIELDS, TIMF_SIZE) ;

                if (i >= 0) {
                    curchan.gen_src = GDS_TIM ;
                    curchan.sub_field = i ;
                    curchan.rate = 1.0 ;
                    cur_sect = XS_SOH ;
                    load_map = load_map | (1L << XS_SOH) ;
                    break ;
                }

                i = match_source ((pchar)SOHFIELDS, SOHF_SIZE) ;

                if (i >= 0) {
                    curchan.gen_src = GDS_SOH ;
                    curchan.sub_field = i ;
                    curchan.rate = 0.1 ;
                    cur_sect = XS_SOH ;
                    load_map = load_map | (1L << XS_SOH) ;
                    break ;
                }

                i = match_source ((pchar)ENGFIELDS, ENGF_SIZE) ;

                if (i >= 0) {
                    curchan.gen_src = GDS_ENG ;
                    curchan.sub_field = i ;
                    curchan.rate = 1.0 ;
                    cur_sect = XS_ENG ;
                    load_map = load_map | (1L << XS_ENG) ;
                    break ;
                }

                i = match_source ((pchar)GPSFIELDS, GPSF_SIZE) ;

                if (i >= 0) {
                    curchan.gen_src = GDS_GPS ;
                    curchan.sub_field = i ;
                    curchan.rate = 1.0 ;
                    cur_sect = XS_SOH ;
                    load_map = load_map | (1L << XS_SOH) ;
                    break ;
                }

                if (strcmp (curchan.source, "DECIMATE") == 0) {
                    curchan.gen_src = GDS_DEC ;
                    curchan.sub_field = dec_count++ ;
                    cur_sect = XS_SOH ;
                    load_map = load_map | (1L << XS_SOH) ;
                    curchan.decptr = NIL ;
                    pdec = chanchain ;
                    q660_upper (curchan.decsrc) ;
                    q660_upper (curchan.decfilt) ;

                    while (pdec)
                        if (strcmp(curchan.decsrc, pdec->seedname) == 0) {
                            curchan.decptr = pdec ;
                            curchan.rate = pdec->rate / 10.0 ;
                            break ;
                        } else
                            pdec = pdec->link ;

                    if (curchan.decptr == NIL)
                        (error_counts[cur_sect])++ ;

                    break ;
                }

                if (strncmp (curchan.source, "MD", 2) == 0) {
                    /* Main digitizer */
                    curchan.gen_src = GDS_MD ;
                    cur_sect = XS_MNAC ;
                    load_map = load_map | (1L << XS_MNAC) ;
                    s[0] = curchan.source[2] ;
                    s[1] = 0 ;
                    good = sscanf (s, "%d", &(i)) ;

                    if ((good == 1) && (i >= 1) && (i <= 6)) {
                        /* valid channel */
                        curchan.sub_field = (i - 1) << 4 ;
                        strncpy (s, &(curchan.source[4]), 7) ;
                        good = sscanf (s, "%d", &(i)) ;

                        if (good == 1) {
                            j = freq_to_bit (i) ;

                            if (j >= 0) {
                                if (digis[curchan.sub_field >> 4].freqmap & (1 << j)) {
                                    /* We have a source */
                                    curchan.freqnum = j ;
                                    curchan.sub_field = curchan.sub_field | j ;
                                    curchan.rate = FREQTAB[j] ;
                                } else
                                    (error_counts[cur_sect])++ ;

                                break ;
                            } else
                                (error_counts[cur_sect])++ ;
                        } else
                            (error_counts[cur_sect])++ ;
                    } else
                        (error_counts[cur_sect])++ ;

                    break ;
                }

                if (strncmp (curchan.source, "CM", 2) == 0) {
                    /* Calibration monitor */
                    curchan.gen_src = GDS_CM ;
                    cur_sect = XS_MNAC ;
                    load_map = load_map | (1L << XS_MNAC) ;
                    strncpy (s, &(curchan.source[2]), 7) ;
                    good = sscanf (s, "%d", &(i)) ;

                    if (good == 1) {
                        j = freq_to_bit (i) ;

                        if (j >= 0) {
                            if (digis[CAL_CHANNEL].freqmap & (1 << j)) {
                                curchan.freqnum = j ;
                                curchan.sub_field = j ;
                                curchan.rate = FREQTAB[j] ;
                            } else
                                (error_counts[cur_sect])++ ;

                            break ;
                        } else
                            (error_counts[cur_sect])++ ;
                    } else
                        (error_counts[cur_sect])++ ;

                    break ;
                }

                if (strncmp (curchan.source, "AC", 2) == 0) {
                    /* Accelerometer */
                    curchan.gen_src = GDS_AC ;
                    cur_sect = XS_MNAC ;
                    load_map = load_map | (1L << XS_MNAC) ;
                    s[0] = curchan.source[2] ;
                    s[1] = 0 ;
                    good = sscanf (s, "%d", &(i)) ;

                    if ((good == 1) && (i >= 1) && (i <= 3)) {
                        /* valid channel */
                        curchan.sub_field = (i - 1) << 4 ;
                        strncpy (s, &(curchan.source[4]), 7) ;
                        good = sscanf (s, "%d", &(i)) ;

                        if ((good == 1) && (i <= 200)) {
                            j = freq_to_bit (i) ;

                            if (j >= 0) {
                                if (digis[(curchan.sub_field >> 4) + MAIN_CHANNELS].freqmap & (1 << j)) {
                                    curchan.freqnum = j ;
                                    curchan.sub_field = curchan.sub_field | j ;
                                    curchan.rate = FREQTAB[j] ;
                                } else
                                    (error_counts[cur_sect])++ ;

                                break ;
                            } else
                                (error_counts[cur_sect])++ ;
                        } else
                            (error_counts[cur_sect])++ ;
                    } else
                        (error_counts[cur_sect])++ ;

                    break ;
                }

                if (strncmp (curchan.source, "DD", 2) == 0) {
                    /* DUST Device */
                    curchan.gen_src = GDS_DUST ;
                    cur_sect = XS_DUST ;
                    load_map = load_map | (1L << XS_DUST) ;
                    s[0] = curchan.source[2] ;
                    s[1] = 0 ;
                    good = sscanf (s, "%d", &(i)) ;

                    if ((good == 1) && (i >= 1) && (i <= DUST_COUNT)) {
                        /* valid device */
                        curchan.sub_field = (i - 1) << 4 ; /* Put device in MS 4 bits */
                        strncpy (s, &(curchan.source[4]), 3) ; /* 2 can fail for some reason */

                        if (s[1] == ',') {
                            /* Single digit channel */
                            s[1] = 0 ;
                            j = 6 ;
                        } else {
                            /* Double digit channel */
                            s[2] = 0 ;
                            j = 7 ;
                        }

                        good = sscanf (s, "%d", &(i)) ;

                        if ((good == 1) && (i >= 1) && (i <= DUST_CHANNELS)) {
                            curchan.sub_field = curchan.sub_field | (i - 1) ; /* LS 4 bits are channel # */
                            strncpy (s, &(curchan.source[j]), 10) ;
                            good = sscanf (s, "%f", &(f)) ;

                            if ((good == 1) && (f >= 0.000099) && (f < 200.1))
                                curchan.rate = f ;
                            else
                                (error_counts[cur_sect])++ ;
                        } else
                            (error_counts[cur_sect])++ ;
                    } else
                        (error_counts[cur_sect])++ ;

                    break ;
                }

                (error_counts[cur_sect])++ ;
                return ;
            }

            /* Convert event list to bitmap */
            strcpy (src, curchan.evt_list) ; /* make local copy */

            if (src[0]) {
                /* Has one */
                psrc = (pchar)&(src) ;

                do {
                    pc = strstr(psrc, ",") ;

                    if (pc) {
                        *pc = 0 ;
                        strncpy (s, psrc, 7) ;
                        psrc = psrc + (strlen(s) + 1) ;
                    } else
                        strncpy (s, psrc, 7) ; /* last one in list */

                    good = sscanf(s, "%d", &(i)) ;

                    if ((good != 1) || (i < 1) || (i > 3))
                        (error_counts[cur_sect])++ ;
                    else
                        curchan.evt_map = curchan.evt_map | (1 << i) ;
                } while (! (pc == NIL)) ;
            }

            /* Convert exclude list to bitmap */
            strcpy (src, curchan.excl_list) ; /* make local copy */

            if (src[0]) {
                /* Has one */
                psrc = (pchar)&(src) ;

                do {
                    pc = strstr(psrc, ",") ;

                    if (pc) {
                        *pc = 0 ;
                        strncpy (s, psrc, 3) ;
                        psrc = psrc + (strlen(s) + 1) ;
                    } else
                        strncpy (s, psrc, 3) ; /* last one in list */

                    if (s[0] == 'X')
                        j = 4 ; /* external data logger */
                    else if (s[0] == 'I')
                        j = 0 ; /* external data logger */
                    else
                        j = -1 ; /* invalid */

                    if (j >= 0) {
                        good = sscanf((pchar)&(s[1]), "%d", &(i)) ;

                        if ((good != 1) || (i < 1) || (i > 4))
                            (error_counts[cur_sect])++ ;
                        else
                            curchan.excl_map = curchan.excl_map | (1 << (i + j - 1)) ;
                    } else
                        (error_counts[cur_sect])++ ;
                } while (! (pc == NIL)) ;
            }

            /* Now put into chain if no errors */
            if (error_counts[cur_sect] != beg_errs[cur_sect])
                return ;

            getbuf (pxmlmem, (pvoid)&(pch), sizeof(tchan)) ;
            memcpy (pch, &(curchan), sizeof(tchan)) ;

            if (chanchain == NIL)
                chanchain = pch ;
            else
                chanchain = extend_link (chanchain, pch) ;

            /* Now add to xdispatch */
            set_dispatch (pch) ;
            chan_count++ ;
            return ;
        } else {
            i = match_tag ((pxmldefarray)&(XCHAN), CHANCNT) ;

            if (i >= 0)
                switch (i) {
                case STALTAIDX :
                    if (det_count >= MAX_DET)
                        (error_counts[cur_sect])++ ;
                    else
                        read_stalta () ;

                    break ;

                case THRIDX :
                    if (det_count >= MAX_DET)
                        (error_counts[cur_sect])++ ;
                    else
                        read_thresh () ;

                    break ;

                default :
                    proc_tag ((pxmldef)&(XCHAN[i])) ;
                } else
                (error_counts[cur_sect])++ ;
        }
}

static void read_xml_dlchan (void)
{
    pchan pch ;
    int i ;

    memclr (&(curchan), sizeof(tchan)) ;

    while (! read_next_tag ())
        if ((! startflag) && (endflag) &&
                (strcmp(tag, "dlchan") == 0))

        {
            curchan.gen_src = GDS_LOG ; /* Data Logger channel */
            i = match_source ((pchar)LOGFIELDS, LOGF_SIZE) ;

            if (i >= 0)
                curchan.sub_field = i ;
            else
                (error_counts[cur_sect])++ ;

            getbuf (pxmlmem, (pvoid)&(pch), sizeof(tchan)) ;
            memcpy (pch, &(curchan), sizeof(tchan)) ;

            if (dlchain == NIL)
                dlchain = pch ;
            else
                dlchain = extend_link (dlchain, pch) ;

            /* Now add to xdispatch */
            set_dispatch (pch) ;
            return ;
        } else {
            i = match_tag ((pxmldefarray)&(XDLCHAN), DLCHCNT) ;

            if (i >= 0)
                proc_tag ((pxmldef)&(XDLCHAN[i])) ;
            else
                (error_counts[cur_sect])++ ;
        }
}

BOOLEAN read_seed (void)
{
    int i ;

    error_counts[XS_STN] = 0 ;
    chan_count = 0 ;
    dec_count = 0 ;

    if (read_xml_start ("seed")) {
        while (! read_next_tag ())
            if ((startflag) && (! endflag)) {
                if (strcmp(tag, "iir") == 0)
                    read_xml_iir () ;
                else if (strcmp(tag, "chan") == 0)
                    read_xml_chan () ;
                else if (strcmp(tag, "dlchan") == 0)
                    read_xml_dlchan () ;
            } else if ((startflag) && (endflag)) {
                cur_sect = XS_STN ;
                load_map = load_map | (1L << XS_STN) ;
                i = match_tag ((pxmldefarray)&(XSEED), SEEDCNT) ;

                if (i >= 0)
                    proc_tag ((pxmldef)&(XSEED[i])) ;
                else
                    (error_counts[cur_sect])++ ;
            }
    } else
        return TRUE ;

    return FALSE ;
}
#endif

void initialize_xml_reader (pmeminst pmem, BOOLEAN initmem)
{

    if (initmem)
        initialize_memory_utils (pmem) ;

    pxmlmem = pmem ;
#ifdef SECT_SEED
    chanchain = NIL ;
    dlchain = NIL ;
    xiirchain = NIL ;
    memclr (&(xdispatch), sizeof(txdispatch)) ;
    memclr (&(xmdispatch), sizeof(txmdispatch)) ;
#endif
    crc_err_map = 0 ;
    load_map = 0 ;
    srccount = 0 ;
    srcidx = 0 ;
}

/* Read XML file and setup line pointers, returns TRUE if error.
   Set buffer to NIL to use internally generated buffer. */
BOOLEAN load_cfg_xml (pmeminst pmem, pchar name, pchar buffer)
{
#ifdef X86_WIN32
    U32 numread, size ;
    HANDLE cf ;
#else
    tfile_handle cf ;
    struct stat sb ;
    int size ;
    ssize_t numread ;
#endif
    pchar buf, pc, limit ;
    char nl ;

    initialize_xml_reader (pmem, TRUE) ;
#ifdef X86_WIN32
    cf = CreateFile (name, GENERIC_READ, 0, NIL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0) ;

    if (cf == INVALID_HANDLE_VALUE)
        return TRUE ;

    size = GetFileSize (cf, NIL) ;

    if (buffer == NIL) {
        getxmlbuf (pxmlmem, (pvoid)&(buf), size) ;

        if (buf == NIL) {
            CloseHandle (cf) ;
            return TRUE ;
        }
    } else
        buf = buffer ;

    ReadFile (cf, (pvoid)buf, size, (pvoid)&(numread), NIL) ;
    GetLastError () ;
    CloseHandle (cf) ;

    if (numread != size)
        return TRUE ;

#else
    cf = open (name, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) ;

    if (cf < 0)
        return TRUE ;

    fstat(cf, &(sb)) ;
    size = (int)sb.st_size ;

    if (buffer == NIL) {
        getxmlbuf (pxmlmem, (pvoid)&(buf), size) ;

        if (buf == NIL) {
            close (cf) ;
            return TRUE ;
        }
    } else
        buf = buffer ;

    numread = read (cf, buf, size) ;
    close (cf) ;

    if (numread != size)
        return TRUE ;

#endif
    xml_file_size = size ;
    limit = buf + (PNTRINT)size ;
    pc = buf ;

    do {
        if ((*pc == 0xD) || (*pc == 0xA)) {
            /* end of line */
            nl = *pc ;
            *pc++ = 0 ; /* terminate */

            if (srccount < (MAXLINES - 1))
                srcptrs[srccount++] = buf ;

            if ((nl == 0xD) && (*pc == 0xA))
                pc++ ; /* skip LF after CR */

            buf = pc ; /* start of next line */
        } else
            pc++ ;
    } while (! ((pc == NIL) || (pc >= limit))) ;

    return FALSE ; /* OK */
}

/* Pointers to thread's version of the structures need to be setup
   before calling */
void make_xml_copy (txmlcopy *pcpy)
{

#ifdef SECT_SENS

    if (pcpy->psensors)
        memcpy (pcpy->psensors, &(sensors), sizeof(tsensors)) ;

#endif
#if defined(SECT_MAIN) || defined(SECT_ACCL)

    if (pcpy->pdigis)
        memcpy (pcpy->pdigis, &(digis), sizeof(tdigis)) ;

#endif
#ifdef SECT_SINF

    if (pcpy->psysinfo)
        memcpy (pcpy->psysinfo, &(sysinfo), sizeof(tsysinfo)) ;

#endif
#ifdef SECT_WRIT

    if (pcpy->pwritercfg)
        memcpy (pcpy->pwritercfg, &(writercfg), sizeof(twritercfg)) ;

#endif
#ifdef SECT_TIME

    if (pcpy->ptiming)
        memcpy (pcpy->ptiming, &(timing), sizeof(ttiming)) ;

#endif
#ifdef SECT_OPTS

    if (pcpy->poptions)
        memcpy (pcpy->poptions, &(options), sizeof(toptions)) ;

#endif
#ifdef SECT_NETW

    if (pcpy->pnetwork)
        memcpy (pcpy->pnetwork, &(network), sizeof(tnetwork)) ;

#endif
#ifdef SECT_ANNC

    if (pcpy->pannounce)
        memcpy (pcpy->pannounce, &(announce), sizeof(tannc)) ;

#endif
#ifdef SECT_AUTO

    if (pcpy->pautomass)
        memcpy (pcpy->pautomass, &(automass), sizeof(tamass)) ;

#endif
#ifdef SECT_SEED

    if (pcpy->pdispatch)
        memcpy (pcpy->pdispatch, &(xdispatch), sizeof(txdispatch)) ;

    if (pcpy->pmdispatch)
        memcpy (pcpy->pmdispatch, &(xmdispatch), sizeof(txmdispatch)) ;

    if (pcpy->pseed)
        memcpy (pcpy->pseed, &(seedbuf), sizeof(tseed)) ;

    pcpy->pchanchain = chanchain ;
    pcpy->pdlchain = dlchain ;
    pcpy->piirchain = xiirchain ;
#endif
}




