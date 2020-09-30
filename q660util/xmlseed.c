/*
     XML Read Configuration Routines
     Copyright 2016-2019 Certified Software Corporation

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
*/
#include "pascal.h"
#include "xmlsup.h"
#include "xmlcfg.h"
#include "xmlseed.h"

#ifdef SECT_SEED
piirdef xiirchain ;
pchan chanchain ;
pchan dlchain ;

tseed seedbuf ;
const txmldef XSEED[SEEDCNT] = {
    {"cfgname", X_SIMP, T_STRING, /*opts*/250, 0, 0, 0, addr(seedbuf.cfgname)},
    {"network", X_SIMP, T_STRING, /*opts*/2, 0, 0, 0, addr(seedbuf.network)},
    {"station", X_SIMP, T_STRING, /*opts*/5, 0, 0, 0, addr(seedbuf.station)}} ;

tiirdef curiir ;
const txmldef XIIR[IIRCNT] = {
    {"iir", X_REC, 0, 0, 0, 0, 0, 0},
    {"name", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curiir.fname)},
    {"sections", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curiir.sects)},
    {"gain", X_SIMP, T_SINGLE, /*opts*/7, 0, 0, 0, addr(curiir.gain)},
    {"reffreq", X_SIMP, T_SINGLE, /*opts*/7, 0, 0, 0, addr(curiir.rate)},
    {"sect", X_ARRAY, 0, /*start*/0, /*compact*/FALSE, /*count*/MAXSECTIONS, /*basesize*/sizeof(tsection_base), addr(curiir.filt[0].poles)},
     {"cutratio", X_SIMP, T_SINGLE, /*opts*/7, 0, 0, 0, addr(curiir.filt[0].ratio)},
     {"poles", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curiir.filt[0].poles)},
     {"high", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curiir.filt[0].highpass)},
    {"sect", X_EARRAY, 0, /*start*/0, /*compact*/FALSE, /*count*/MAXSECTIONS, /*basesize*/sizeof(tsection_base), 0},
    {"iir", X_EREC, 0, 0, 0, 0, 0, 0}} ;

tchan curchan ;
#define CHANCNT 21
const txmldef XCHAN[CHANCNT] = {
    {"chan", X_REC, 0, 0, 0, 0, 0, 0},
    {"name", X_SIMP, T_STRING, /*opts*/6, 0, 0, 0, addr(curchan.seedname)},
    {"source", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, addr(curchan.source)},
    {"desc", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curchan.desc)},
    {"decsource", X_SIMP, T_STRING, /*opts*/6, 0, 0, 0, addr(curchan.decsrc)},
    {"decfilt", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curchan.decfilt)},
    {"units", X_SIMP, T_STRING, /*opts*/23, 0, 0, 0, addr(curchan.units)},
    {"evt_only", X_SIMP, T_STRING, /*opts*/7, 0, 0, 0, addr(curchan.evt_list)},
    {"exclude", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curchan.excl_list)},
    {"fir", X_SIMP, T_STRING, /*opts*/47, 0, 0, 0, addr(curchan.fir_chain)},
    {"latency", X_SIMP, T_WORD, 0, 0, 0, 0, addr(curchan.latency)},
    {"cal_trig", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curchan.cal_trig)},
    {"no_output", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curchan.no_output)},
    {"disable", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curchan.disable)},
    {"mask", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curchan.mask)},
    {"delay", X_SIMP, T_DOUBLE, /*prec*/6, 0, 0, 0, addr(curchan.delay)},
    {"sta_lta", X_REC, 0, 0, 0, 0, 0, 0},
    {"sta_lta", X_EREC, 0, 0, 0, 0, 0, 0},
    {"threshold", X_REC, 0, 0, 0, 0, 0, 0},
    {"threshold", X_EREC, 0, 0, 0, 0, 0, 0},
    {"chan", X_EREC, 0, 0, 0, 0, 0, 0}} ;

tdetector curdet ;
tstaltacfg slcfg ;
tthreshcfg thrcfg ;
static integer det_count ;
#define STALTAIDX 16
#define THRIDX 18
const txmldef XSTALTA[SLCNT] = {
    {"sta_lta", X_REC, 0, 0, 0, 0, 0, 0},
    {"detname", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curdet.dname)},
    {"filter", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curdet.fname)},
    {"run", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curdet.run)},
    {"ratio", X_SIMP, T_SINGLE, /*prec*/2, 0, 0, 0, addr(slcfg.ratio)},
    {"quiet", X_SIMP, T_SINGLE, /*prec*/2, 0, 0, 0, addr(slcfg.quiet)},
    {"sta_win", X_SIMP, T_WORD, 0, 0, 0, 0, addr(slcfg.sta_win)},
    {"lta_mult", X_SIMP, T_WORD, 0, 0, 0, 0, addr(slcfg.lta_mult)},
    {"pre", X_SIMP, T_WORD, 0, 0, 0, 0, addr(slcfg.pre_event)},
    {"post", X_SIMP, T_WORD, 0, 0, 0, 0, addr(slcfg.post_event)},
    {"sta_lta", X_EREC, 0, 0, 0, 0, 0, 0}} ;
const txmldef XTHRESH[THRCNT] = {
    {"threshold", X_REC, 0, 0, 0, 0, 0, 0},
    {"detname", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curdet.dname)},
    {"filter", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curdet.fname)},
    {"run", X_SIMP, T_BYTE, 0, 0, 0, 0, addr(curdet.run)},
    {"high", X_SIMP, T_LINT, 0, 0, 0, 0, addr(thrcfg.hi_thresh)},
    {"low", X_SIMP, T_LINT, 0, 0, 0, 0, addr(thrcfg.lo_thresh)},
    {"hysteresis", X_SIMP, T_LINT, 0, 0, 0, 0, addr(thrcfg.hysteresis)},
    {"window", X_SIMP, T_LINT, 0, 0, 0, 0, addr(thrcfg.window)},
    {"pre", X_SIMP, T_WORD, 0, 0, 0, 0, addr(thrcfg.pre_event)},
    {"post", X_SIMP, T_WORD, 0, 0, 0, 0, addr(thrcfg.post_event)},
    {"threshold", X_EREC, 0, 0, 0, 0, 0, 0}} ;

const txmldef XDLCHAN[DLCHCNT] = {
    {"dlchan", X_REC, 0, 0, 0, 0, 0, 0},
    {"name", X_SIMP, T_STRING, /*opts*/6, 0, 0, 0, addr(curchan.seedname)},
    {"source", X_SIMP, T_STRING, /*opts*/15, 0, 0, 0, addr(curchan.source)},
    {"desc", X_SIMP, T_STRING, /*opts*/31, 0, 0, 0, addr(curchan.desc)},
    {"dlchan", X_EREC, 0, 0, 0, 0, 0, 0}} ;

const ttimfields TIMFIELDS = {"PHASE", "CLKQUAL", "CLKLOSS"} ;
const tsohfields SOHFIELDS = {"ANTCUR", "SENSACUR", "SENSBCUR",
    "BOOM1", "BOOM2", "BOOM3", "BOOM4", "BOOM5", "BOOM6", "SYSTEMP", "HUMIDITY",
    "INPVOLT", "VCOCTRL", "NEGAN", "ISODC", "GPIN1", "GPIN2", "SYSCUR", "UPSVOLT",
    "ANTVOLT", "GPOUT", "PKTBUF"} ;
const tengfields ENGFIELDS = {"GPSSENS", "GPSCTRL", "SIOCTRLA", "SIOCTRLB",
    "SENSIO", "MISCIO", "DUSTIO"} ;
const tgpsfields GPSFIELDS = {"SATUSED", "FIXTYPE", "LATITUDE", "LONGITUDE", "ELEVATION"} ;
const tlogfields LOGFIELDS = {"MESSAGES", "CONFIG", "TIMING", "DATAGAPS", "REBOOTS",
    "RECVBPS", "SENTBPS", "COMATTEMPTS", "COMSUCCESS", "PKTSRECV", "COMEFF", "POCRECV",
    "IPCHANGES", "COMDUTY", "THROUGHPUT", "MISSDATA", "SEQERR", "CHKERR",
    "DATALAT", "LLDATALAT", "STATLAT"} ;

txdispatch xdispatch ;
txmdispatch xmdispatch ;
integer chan_count ;
byte dec_count ;
integer xml_file_size ;

void add_dispatch (pchan pl)
begin
  pchan p ;
  integer i, j ;

  i = (integer)pl->gen_src ;
  j = pl->sub_field ;
  if (xdispatch[i][j] == NIL)
    then
      xdispatch[i][j] = pl ;
    else
      begin
        p = xdispatch[i][j] ;
        while (p->sortlink)
          p = p->sortlink ;
        p->sortlink = pl ;
      end
end

void add_mdispatch (pchan pl, integer offset)
begin
  pchan p ;
  integer i, j ;

  i = (pl->sub_field shr 4) + offset ; /* get channel */
  j = pl->freqnum ; /* frequency bit */
  if (xmdispatch[i][j] == NIL)
    then
      xmdispatch[i][j] = pl ;
    else
      begin
        p = xmdispatch[i][j] ;
        while (p->sortlink)
          p = p->sortlink ;
        p->sortlink = pl ;
      end
end

void set_dispatch (pchan pl)
begin

  switch (pl->gen_src) begin
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
  end
end

static void read_xml_iir (void)
begin
  piirdef piir ;
  integer i ;

  memclr (addr(curiir), sizeof(tiirdef)) ;
  cur_sect = XS_IIR ;
  while (lnot read_next_tag ())
    if ((lnot startflag) land (endflag) land
        (strcmp(tag, "iir") == 0))
      then
        begin
          load_map = load_map or (1L shl XS_IIR) ;
          getbuf (pxmlmem, (pvoid)addr(piir), sizeof(tiirdef)) ;
          memcpy (piir, addr(curiir), sizeof(tiirdef)) ;
          if (xiirchain == NIL)
            then
              xiirchain = piir ;
            else
              xiirchain = extend_link (xiirchain, piir) ;
          return ;
        end
      else
        begin
          i = match_tag ((pxmldefarray)addr(XIIR), IIRCNT) ;
          if (i >= 0)
            then
              proc_tag ((pxmldef)addr(XIIR[i])) ;
            else
              inc(error_counts[XS_IIR]) ;
        end
end

static void lookup_det_filter (void)
begin
  piirdef piir ;
  string31 s1, s2 ;

  curdet.fptr = NIL ;
  strcpy (s1, curdet.fname) ;
  if (strlen(s1) == 0)
    then
      return ; /* nothing to look up */
  q660_upper (s1) ;
  piir = xiirchain ;
  while (piir)
    begin
      strcpy (s2, piir->fname) ;
      q660_upper (s2) ;
      if (strcmp (s1, s2) == 0)
        then
          begin
            curdet.fptr = piir ;
            return ; /* found it */
          end
      piir = piir->link ;
    end
  inc(error_counts[cur_sect]) ; /* not found */
end

static void read_stalta (void)
begin
  integer i ;
  pdetector pd ;

  memclr (addr(curdet), sizeof(tdetector)) ;
  memclr (addr(slcfg), sizeof(tstaltacfg)) ;
  curdet.dtype = DT_STA_LTA ;
  while (lnot read_next_tag ())
    if ((lnot startflag) land (endflag) land (strcmp(tag, "sta_lta") == 0))
      then
        begin
          memcpy (addr(curdet.cfg), addr(slcfg), sizeof(tstaltacfg)) ; /* fill in parameters */
          lookup_det_filter () ;
          getbuf (pxmlmem, (pvoid)addr(pd), sizeof(tdetector)) ; /* get a buffer for the detector */
          memcpy (pd, addr(curdet), sizeof(tdetector)) ; /* and copy into */
          curchan.detptrs[det_count] = pd ;
          det_count++ ;
          break ;
        end
      else
        begin
          i = match_tag ((pxmldefarray)addr(XSTALTA), SLCNT) ;
          if (i >= 0)
            then
              proc_tag ((pxmldef)addr(XSTALTA[i])) ;
            else
              inc(error_counts[cur_sect]) ;
        end
end

static void read_thresh (void)
begin
  integer i ;
  pdetector pd ;

  memclr (addr(curdet), sizeof(tdetector)) ;
  memclr (addr(thrcfg), sizeof(tthreshcfg)) ;
  curdet.dtype = DT_THRESHOLD ;
  while (lnot read_next_tag ())
    if ((lnot startflag) land (endflag) land (strcmp(tag, "threshold") == 0))
      then
        begin
          memcpy (addr(curdet.cfg), addr(thrcfg), sizeof(tthreshcfg)) ; /* fill in parameters */
          lookup_det_filter () ;
          if (strcmp(curdet.dname, "BOOM WARNING") == 0)
            then
              curdet.std_num = 3 ;
          else if (strcmp(curdet.dname, "OVERTEMP") == 0)
            then
              curdet.std_num = 1 ;
          else if (strcmp(curdet.dname, "ANTCUR") == 0)
            then
              curdet.std_num = 2 ;
          getbuf (pxmlmem, (pvoid)addr(pd), sizeof(tdetector)) ; /* get a buffer for the detector */
          memcpy (pd, addr(curdet), sizeof(tdetector)) ; /* and copy into */
          curchan.detptrs[det_count] = pd ;
          det_count++ ;
          break ;
        end
      else
        begin
          i = match_tag ((pxmldefarray)addr(XTHRESH), THRCNT) ;
          if (i >= 0)
            then
              proc_tag ((pxmldef)addr(XTHRESH[i])) ;
            else
              inc(error_counts[cur_sect]) ;
        end
  return ;
end

static integer match_source (pchar list, integer count) /* returns -1 if no match, else index */
begin
  integer i ;

  for (i = 0 ; i < count ; i++)
    if (strcmp (curchan.source, list) == 0)
      then
        return i ; /* index in list */
      else
        incn (list, sizeof(string31)) ;
  return -1 ;
end

static void read_xml_chan (void)
begin
  pchan pch, pdec ;
  pchar pc, psrc ;
  integer i, j, good ;
  string31 s, src ;
  terr_counts beg_errs ;

  memclr (addr(curchan), sizeof(tchan)) ;
  det_count = 0 ;
  cur_sect = XS_MNAC ;
  memcpy (addr(beg_errs), addr(error_counts), sizeof(terr_counts)) ;
  while (lnot read_next_tag ())
    if ((lnot startflag) land (endflag) land
        (strcmp(tag, "chan") == 0))
      then
        begin
          /* Process source string */
          q660_upper (curchan.source) ; /* These should all be q660_upper, make sure */
          while (TRUE)
            begin /* this is a bit of a cheat to avoid deeply nested elses */
              i = match_source ((pchar)TIMFIELDS, TIMF_SIZE) ;
              if (i >= 0)
                then
                  begin
                    curchan.gen_src = GDS_TIM ;
                    curchan.sub_field = i ;
                    curchan.rate = 1.0 ;
                    cur_sect = XS_SOH ;
                    load_map = load_map or (1L shl XS_SOH) ;
                    break ;
                  end
              i = match_source ((pchar)SOHFIELDS, SOHF_SIZE) ;
              if (i >= 0)
                then
                  begin
                    curchan.gen_src = GDS_SOH ;
                    curchan.sub_field = i ;
                    curchan.rate = 0.1 ;
                    cur_sect = XS_SOH ;
                    load_map = load_map or (1L shl XS_SOH) ;
                    break ;
                  end
              i = match_source ((pchar)ENGFIELDS, ENGF_SIZE) ;
              if (i >= 0)
                then
                  begin
                    curchan.gen_src = GDS_ENG ;
                    curchan.sub_field = i ;
                    curchan.rate = 1.0 ;
                    cur_sect = XS_ENG ;
                    load_map = load_map or (1L shl XS_ENG) ;
                    break ;
                  end
              i = match_source ((pchar)GPSFIELDS, GPSF_SIZE) ;
              if (i >= 0)
                then
                  begin
                    curchan.gen_src = GDS_GPS ;
                    curchan.sub_field = i ;
                    curchan.rate = 1.0 ;
                    cur_sect = XS_SOH ;
                    load_map = load_map or (1L shl XS_SOH) ;
                    break ;
                  end
              if (strcmp (curchan.source, "DECIMATE") == 0)
                then
                  begin
                    curchan.gen_src = GDS_DEC ;
                    curchan.sub_field = dec_count++ ;
                    cur_sect = XS_SOH ;
                    load_map = load_map or (1L shl XS_SOH) ;
                    curchan.decptr = NIL ;
                    pdec = chanchain ;
                    q660_upper (curchan.decsrc) ;
                    q660_upper (curchan.decfilt) ;
                    while (pdec)
                      if (strcmp(curchan.decsrc, pdec->seedname) == 0)
                        begin
                          curchan.decptr = pdec ;
                          curchan.rate = pdec->rate / 10.0 ;
                          break ;
                        end
                      else
                        pdec = pdec->link ;
                    if (curchan.decptr == NIL)
                      then
                        inc(error_counts[cur_sect]) ;
                    break ;
                  end
              if (strncmp (curchan.source, "MD", 2) == 0)
                then
                  begin /* Main digitizer */
                    curchan.gen_src = GDS_MD ;
                    cur_sect = XS_MNAC ;
                    load_map = load_map or (1L shl XS_MNAC) ;
                    s[0] = curchan.source[2] ;
                    s[1] = 0 ;
                    good = sscanf (s, "%d", addr(i)) ;
                    if ((good == 1) land (i >= 1) land (i <= 6))
                      then
                        begin /* valid channel */
                          curchan.sub_field = (i - 1) shl 4 ;
                          strncpy (s, addr(curchan.source[4]), 7) ;
                          good = sscanf (s, "%d", addr(i)) ;
                          if (good == 1)
                            then
                              begin
                                j = freq_to_bit (i) ;
                                if (j >= 0)
                                  then
                                    begin
                                      if (digis[curchan.sub_field shr 4].freqmap and (1 shl j))
                                        then
                                          begin /* We have a source */
                                            curchan.freqnum = j ;
                                            curchan.sub_field = curchan.sub_field or j ;
                                            curchan.rate = FREQTAB[j] ;
                                          end
                                        else
                                          inc(error_counts[cur_sect]) ;
                                      break ;
                                    end
                                  else
                                    inc(error_counts[cur_sect]) ;
                              end
                            else
                              inc(error_counts[cur_sect]) ;
                        end
                      else
                        inc(error_counts[cur_sect]) ;
                  end
              if (strncmp (curchan.source, "CM", 2) == 0)
                then
                  begin /* Calibration monitor */
                    curchan.gen_src = GDS_CM ;
                    cur_sect = XS_MNAC ;
                    load_map = load_map or (1L shl XS_MNAC) ;
                    strncpy (s, addr(curchan.source[2]), 7) ;
                    good = sscanf (s, "%d", addr(i)) ;
                    if (good == 1)
                      then
                        begin
                          j = freq_to_bit (i) ;
                          if (j >= 0)
                            then
                              begin
                                if (digis[CAL_CHANNEL].freqmap and (1 shl j))
                                  then
                                    begin
                                      curchan.freqnum = j ;
                                      curchan.sub_field = j ;
                                      curchan.rate = FREQTAB[j] ;
                                    end
                                  else
                                    inc(error_counts[cur_sect]) ;
                                break ;
                              end
                            else
                              inc(error_counts[cur_sect]) ;
                        end
                      else
                        inc(error_counts[cur_sect]) ;
                  end
              if (strncmp (curchan.source, "AC", 2) == 0)
                then
                  begin /* Accelerometer */
                    curchan.gen_src = GDS_AC ;
                    cur_sect = XS_MNAC ;
                    load_map = load_map or (1L shl XS_MNAC) ;
                    s[0] = curchan.source[2] ;
                    s[1] = 0 ;
                    good = sscanf (s, "%d", addr(i)) ;
                    if ((good == 1) land (i >= 1) land (i <= 3))
                      then
                        begin /* valid channel */
                          curchan.sub_field = (i - 1) shl 4 ;
                          strncpy (s, addr(curchan.source[4]), 7) ;
                          good = sscanf (s, "%d", addr(i)) ;
                          if ((good == 1) land (i <= 200))
                            then
                              begin
                                j = freq_to_bit (i) ;
                                if (j >= 0)
                                  then
                                    begin
                                      if (digis[(curchan.sub_field shr 4) + MAIN_CHANNELS].freqmap and (1 shl j))
                                        then
                                          begin
                                            curchan.freqnum = j ;
                                            curchan.sub_field = curchan.sub_field or j ;
                                            curchan.rate = FREQTAB[j] ;
                                          end
                                        else
                                          inc(error_counts[cur_sect]) ;
                                      break ;
                                    end
                                  else
                                    inc(error_counts[cur_sect]) ;
                              end
                            else
                              inc(error_counts[cur_sect]) ;
                        end
                      else
                        inc(error_counts[cur_sect]) ;
                  end
              inc(error_counts[cur_sect]) ;
              return ;
            end
          /* Convert event list to bitmap */
          strcpy (src, curchan.evt_list) ; /* make local copy */
          if (src[0])
            then
              begin /* Has one */
                psrc = (pchar)addr(src) ;
                repeat
                  pc = strstr(psrc, ",") ;
                  if (pc)
                    then
                      begin
                        *pc = 0 ;
                        strncpy (s, psrc, 7) ;
                        incn (psrc, strlen(s) + 1) ;
                      end
                    else
                      strncpy (s, psrc, 7) ; /* last one in list */
                  good = sscanf(s, "%d", addr(i)) ;
                  if ((good != 1) lor (i < 1) lor (i > 3))
                    then
                      inc(error_counts[cur_sect]) ;
                    else
                      curchan.evt_map = curchan.evt_map or (1 shl i) ;
                until (pc == NIL)) ;
              end
          /* Convert exclude list to bitmap */
          strcpy (src, curchan.excl_list) ; /* make local copy */
          if (src[0])
            then
              begin /* Has one */
                psrc = (pchar)addr(src) ;
                repeat
                  pc = strstr(psrc, ",") ;
                  if (pc)
                    then
                      begin
                        *pc = 0 ;
                        strncpy (s, psrc, 3) ;
                        incn (psrc, strlen(s) + 1) ;
                      end
                    else
                      strncpy (s, psrc, 3) ; /* last one in list */
                  if (s[0] == 'X')
                    then
                      j = 4 ; /* external data logger */
                  else if (s[0] == 'I')
                    then
                      j = 0 ; /* external data logger */
                    else
                      j = -1 ; /* invalid */
                  if (j >= 0)
                    then
                      begin
                        good = sscanf((pchar)addr(s[1]), "%d", addr(i)) ;
                        if ((good != 1) lor (i < 1) lor (i > 4))
                          then
                            inc(error_counts[cur_sect]) ;
                          else
                            curchan.excl_map = curchan.excl_map or (1 shl (i + j - 1)) ;
                      end
                    else
                      inc(error_counts[cur_sect]) ;
                until (pc == NIL)) ;
              end
          /* Now put into chain if no errors */
          if (error_counts[cur_sect] != beg_errs[cur_sect])
            then
              return ;
          getbuf (pxmlmem, (pvoid)addr(pch), sizeof(tchan)) ;
          memcpy (pch, addr(curchan), sizeof(tchan)) ;
          if (chanchain == NIL)
            then
              chanchain = pch ;
            else
              chanchain = extend_link (chanchain, pch) ;
          /* Now add to xdispatch */
          set_dispatch (pch) ;
          chan_count++ ;
          return ;
        end
      else
        begin
          i = match_tag ((pxmldefarray)addr(XCHAN), CHANCNT) ;
          if (i >= 0)
            then
              switch (i) begin
                case STALTAIDX :
                  if (det_count >= MAX_DET)
                    then
                      inc(error_counts[cur_sect]) ;
                    else
                      read_stalta () ;
                  break ;
                case THRIDX :
                  if (det_count >= MAX_DET)
                    then
                      inc(error_counts[cur_sect]) ;
                    else
                      read_thresh () ;
                  break ;
                default :
                  proc_tag ((pxmldef)addr(XCHAN[i])) ;
              end
            else
              inc(error_counts[cur_sect]) ;
        end
end

static void read_xml_dlchan (void)
begin
  pchan pch ;
  integer i ;

  memclr (addr(curchan), sizeof(tchan)) ;
  while (lnot read_next_tag ())
    if ((lnot startflag) land (endflag) land
        (strcmp(tag, "dlchan") == 0))
      then
        begin
          curchan.gen_src = GDS_LOG ; /* Data Logger channel */
          i = match_source ((pchar)LOGFIELDS, LOGF_SIZE) ;
          if (i >= 0)
            then
              curchan.sub_field = i ;
            else
              inc(error_counts[cur_sect]) ;
          getbuf (pxmlmem, (pvoid)addr(pch), sizeof(tchan)) ;
          memcpy (pch, addr(curchan), sizeof(tchan)) ;
          if (dlchain == NIL)
            then
              dlchain = pch ;
            else
              dlchain = extend_link (dlchain, pch) ;
          /* Now add to xdispatch */
          set_dispatch (pch) ;
          return ;
        end
      else
        begin
          i = match_tag ((pxmldefarray)addr(XDLCHAN), DLCHCNT) ;
          if (i >= 0)
            then
              proc_tag ((pxmldef)addr(XDLCHAN[i])) ;
            else
              inc(error_counts[cur_sect]) ;
        end
end

boolean read_seed (void)
begin
  integer i ;

  error_counts[XS_STN] = 0 ;
  chan_count = 0 ;
  dec_count = 0 ;
  if (read_xml_start ("seed"))
    then
      begin
        while (lnot read_next_tag ())
          if ((startflag) land (lnot endflag))
            then
              begin
                if (strcmp(tag, "iir") == 0)
                  then
                    read_xml_iir () ;
                else if (strcmp(tag, "chan") == 0)
                  then
                    read_xml_chan () ;
                else if (strcmp(tag, "dlchan") == 0)
                  then
                    read_xml_dlchan () ;
              end
          else if ((startflag) land (endflag))
            then
              begin
                cur_sect = XS_STN ;
                load_map = load_map or (1L shl XS_STN) ;
                i = match_tag ((pxmldefarray)addr(XSEED), SEEDCNT) ;
                if (i >= 0)
                  then
                    proc_tag ((pxmldef)addr(XSEED[i])) ;
                  else
                    inc(error_counts[cur_sect]) ;
              end
      end
    else
      return TRUE ;
  return FALSE ;
end
#endif

void initialize_xml_reader (pmeminst pmem, boolean initmem)
begin

  if (initmem)
    then
      initialize_memory_utils (pmem) ;
  pxmlmem = pmem ;
#ifdef SECT_SEED
  chanchain = NIL ;
  dlchain = NIL ;
  xiirchain = NIL ;
  memclr (addr(xdispatch), sizeof(txdispatch)) ;
  memclr (addr(xmdispatch), sizeof(txmdispatch)) ;
#endif
  crc_err_map = 0 ;
  load_map = 0 ;
  srccount = 0 ;
  srcidx = 0 ;
end

/* Read XML file and setup line pointers, returns TRUE if error.
   Set buffer to NIL to use internally generated buffer. */
boolean load_cfg_xml (pmeminst pmem, pchar name, pchar buffer)
begin
#ifdef X86_WIN32
  longword numread, size ;
  HANDLE cf ;
#else
  tfile_handle cf ;
  struct stat sb ;
  integer size ;
  ssize_t numread ;
#endif
  pchar buf, pc, limit ;
  char nl ;

  initialize_xml_reader (pmem, TRUE) ;
#ifdef X86_WIN32
  cf = CreateFile (name, GENERIC_READ, 0, NIL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0) ;
  if (cf == INVALID_HANDLE_VALUE)
    then
      return TRUE ;
  size = GetFileSize (cf, NIL) ;
  if (buffer == NIL)
    then
      begin
        getxmlbuf (pxmlmem, (pvoid)addr(buf), size) ;
        if (buf == NIL)
          then
            begin
              CloseHandle (cf) ;
              return TRUE ;
            end
      end
    else
      buf = buffer ;
  ReadFile (cf, (pvoid)buf, size, (pvoid)addr(numread), NIL) ;
  GetLastError () ;
  CloseHandle (cf) ;
  if (numread != size)
    then
      return TRUE ;
#else
  cf = open (name, O_RDONLY, S_IRUSR or S_IWUSR or S_IRGRP or S_IROTH) ;
  if (cf < 0)
    then
      return TRUE ;
  fstat(cf, addr(sb)) ;
  size = (integer)sb.st_size ;
  if (buffer == NIL)
    then
      begin
        getxmlbuf (pxmlmem, (pvoid)addr(buf), size) ;
        if (buf == NIL)
          then
            begin
              close (cf) ;
              return TRUE ;
            end
      end
    else
      buf = buffer ;
  numread = read (cf, buf, size) ;
  close (cf) ;
  if (numread != size)
    then
      return TRUE ;
#endif
  xml_file_size = size ;
  limit = buf + (pntrint)size ;
  pc = buf ;
  repeat
    if ((*pc == 0xD) lor (*pc == 0xA))
      then
        begin /* end of line */
          nl = *pc ;
          *pc++ = 0 ; /* terminate */
          if (srccount < (MAXLINES - 1))
            then
              srcptrs[srccount++] = buf ;
          if ((nl == 0xD) land (*pc == 0xA))
            then
              pc++ ; /* skip LF after CR */
          buf = pc ; /* start of next line */
        end
      else
        pc++ ;
  until ((pc == NIL) lor (pc >= limit))) ;
  return FALSE ; /* OK */
end

/* Pointers to thread's version of the structures need to be setup
   before calling */
void make_xml_copy (txmlcopy *pcpy)
begin

#ifdef SECT_SENS
  if (pcpy->psensors)
    then
      memcpy (pcpy->psensors, addr(sensors), sizeof(tsensors)) ;
#endif
#if defined(SECT_MAIN) || defined(SECT_ACCL)
  if (pcpy->pdigis)
    then
      memcpy (pcpy->pdigis, addr(digis), sizeof(tdigis)) ;
#endif
#ifdef SECT_SINF
  if (pcpy->psysinfo)
    then
      memcpy (pcpy->psysinfo, addr(sysinfo), sizeof(tsysinfo)) ;
#endif
#ifdef SECT_WRIT
  if (pcpy->pwritercfg)
    then
      memcpy (pcpy->pwritercfg, addr(writercfg), sizeof(twritercfg)) ;
#endif
#ifdef SECT_TIME
  if (pcpy->ptiming)
    then
      memcpy (pcpy->ptiming, addr(timing), sizeof(ttiming)) ;
#endif
#ifdef SECT_OPTS
  if (pcpy->poptions)
    then
      memcpy (pcpy->poptions, addr(options), sizeof(toptions)) ;
#endif
#ifdef SECT_NETW
  if (pcpy->pnetwork)
    then
      memcpy (pcpy->pnetwork, addr(network), sizeof(tnetwork)) ;
#endif
#ifdef SECT_ANNC
  if (pcpy->pannounce)
    then
      memcpy (pcpy->pannounce, addr(announce), sizeof(tannc)) ;
#endif
#ifdef SECT_AUTO
  if (pcpy->pautomass)
    then
      memcpy (pcpy->pautomass, addr(automass), sizeof(tamass)) ;
#endif
#ifdef SECT_SEED
  if (pcpy->pdispatch)
    then
      memcpy (pcpy->pdispatch, addr(xdispatch), sizeof(txdispatch)) ;
  if (pcpy->pmdispatch)
    then
      memcpy (pcpy->pmdispatch, addr(xmdispatch), sizeof(txmdispatch)) ;
  if (pcpy->pseed)
    then
      memcpy (pcpy->pseed, addr(seedbuf), sizeof(tseed)) ;
  pcpy->pchanchain = chanchain ;
  pcpy->pdlchain = dlchain ;
  pcpy->piirchain = xiirchain ;
#endif
end




