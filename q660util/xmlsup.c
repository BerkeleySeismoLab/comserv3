/*
     XML Read Support Routines
     Copyright 2015, 2016 Certified Software Corporation

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
    0 2015-12-15 rdr Adapted from WS330.
    1 2015-12-28 rdr Add support for letters as array index in tag.
    2 2016-01-08 rdr Replace calls to malloc with getbuf.
    3 2016-01-17 rdr Allow null string values.
    4 2016-01-18 rdr Change error handling to process as much as possible but
                     keep track of error count per section.
    5 2016-02-07 rdr Add buffer override parameter to load_cfg_xml.
    6 2016-02-19 rdr Allow mixed-case field lists in match_field.
    7 2017-06-14 rdr Fix processing of T_DOUBLE.
*/
#include "pascal.h"
#include "xmlsup.h"
#include <ctype.h>

typedef char tchararray[256] ;

enum txmlsect cur_sect ;
integer xmlnest ;
integer srccount ;
integer srcidx ;
string value ;
string tag ;
boolean startflag, endflag ;
string cur_xml_line ;
tsrcptrs srcptrs ;
pmeminst pxmlmem ;        /* Current memory manager pointer */

/* following returned from read_xml */
boolean crcvalid ;
longint sectcrc ;
longword crc_err_map ; /* Bitmap of structures with CRC errors */
longword load_map ; /* Bitmap of structures loaded */
/* end of caller variables */

static integer idx ;
static boolean inarray ;
static tarraystack arraystack[ARRAY_DEPTH] ;
static integer curdepth ; /* current array curdepth, 0 .. 2 */
static integer nesting ; /* current tag nesting */
static boolean insection, compactflag ;
static string sectname ;
static longint xmlcrc, last_xmlcrc ;

const crc_table_type CRC_TABLE = {
  0, 1443300200, -1408366896, -100072008,
  236654280, 1478233504, -1575679976, -200144016,
  473308560, 1244733176, -1338500288, -432072664,
  304943960, 1143607344, -1104996984, -400288032,
  946617120, 1852520520, -1805500944, -1033552232,
  913782248, 1617966720, -1703333064, -864145328,
  609887920, 1918707160, -2007752608, -564976888,
  708913272, 2084973328, -2041631064, -800576064,
  1893234240, 652178728, -589926256, -1965984776,
  2126739592, 683965408, -758284712, -2067104464,
  1827564496, 988375224, -1059033856, -1763217816,
  1660249368, 888301168, -822385720, -1728290656,
  1219775840, 515067400, -457552976, -1296217896,
  1185891240, 279462080, -358529672, -1129953776,
  1417826544, 42292120, -125020640, -1366600376,
  1520000568, 211705168, -157853464, -1601152128,
  -508498816, -1212682264, 1304357456, 465168696,
  -273948088, -1179852512, 1134945432, 362997744,
  -41488112, -1417547144, 1367930816, 126874792,
  -207749160, -1516569424, 1603533064, 160758368,
  -639838304, -1881418552, 1976750448, 601215512,
  -674768536, -2118067712, 2076827576, 768531664,
  -974468560, -1813132968, 1776602336, 1071894408,
  -873347848, -1644771440, 1744814632, 838385984,
  -1855415616, -948987480, 1030134800, 1801559928,
  -1619815416, -915105952, 863867608, 1702531504,
  -1923184816, -614890440, 558924160, 2002223848,
  -2092594792, -717059344, 793481032, 2035059744,
  -1459314208, -16539000, 84584240, 1393402968,
  -1491095768, -250041280, 185707000, 1561766544,
  -1254966160, -483016936, 423410336, 1329314248,
  -1154895176, -315706928, 388478056, 1092663040,
  -1788421528, -1016997632, 963173560, 1869602768,
  -1686252384, -847587384, 930337392, 1635045656,
  -1991194632, -547896176, 626966824, 1935262272,
  -2025076432, -783496616, 725995488, 2101529736,
  -1391795896, -82976224, 17098648, 1459873008,
  -1559105664, -183046936, 253749584, 1494805048,
  -1321403176, -415498320, 489880072, 1261828448,
  -1087901168, -383717000, 321516736, 1160705960,
  -440488920, -1279676608, 1236314872, 532130192,
  -341466400, -1113415288, 1202431024, 296527704,
  -108482120, -1349537072, 1434892136, 58831872,
  -141312144, -1584088040, 1537063328, 228244168,
  -573401336, -1948937120, 1910280664, 668701360,
  -741762624, -2050057560, 2143788816, 700488824,
  -1041986920, -1746695696, 1844087880, 1005424416,
  -805338032, -1711765704, 1676771968, 905347560,
  1960195816, 584136064, -656920520, -1897974960,
  2060269600, 751450952, -691847440, -2134622824,
  1759521656, 1055336464, -991023704, -1830211904,
  1727735216, 821831384, -889904288, -1661853688,
  1287261640, 448597664, -525071592, -1229780880,
  1117848320, 346423400, -290519600, -1196947784,
  1351356504, 109777712, -58583416, -1434118688,
  1586962064, 143662584, -224847808, -1533142232,
  68042920, 1376338880, -1476376968, -33078000,
  169168480, 1544703240, -1508161360, -266581032,
  406347064, 1312775760, -1271505944, -500082560,
  371414000, 1076121752, -1171434208, -332769720,
  1013087112, 1785034976, -1871938216, -966033872,
  846820672, 1686009384, -1636338800, -932155144,
  542402072, 1985176944, -1940234040, -631413856,
  776956112, 2018012088, -2109641216, -733581976} ;

const tescapes ESCAPES = {
    {'&', "&amp;"}, {'<', "&lt;"}, {'>', "&gt;"}, {'\'', "&apos;"}, {'\"', "&quot;"}} ;
const tsectnames SECTNAMES = {"System Info", "Writer", "Sensors", "Main Digitizer",
    "Accelerometer", "DUST Configuration", "Timing", "Options", "Network", "Announcements",
    "Auto Mass Recenter", "Station Information", "IIR Filters", "Main Digitizer & Accelerometer",
    "State of Health", "DUST Channels", "Engineering Data"} ;

/* XML Mutex Routines. To use the XML parser the calling program must call
   create_xml_mutex once. For each thread that needs to use the routines
   call xml_lock, do any initialization, do the parsing, copy the result
   into thread storage, and then call xml_unlock. These steps can be bypassed
   if the calling program only has one thread that will use the parser. */
#ifdef X86_WIN32
static HANDLE xml_mutex ;

void create_xml_mutex (void)
begin

  xml_mutex = CreateMutex(NIL, FALSE, NIL) ;
end

void xml_lock (void)
begin

  WaitForSingleObject (xml_mutex, INFINITE) ;
end

void xml_unlock (void)
begin

  ReleaseMutex (xml_mutex) ;
end

#else
static pthread_mutex_t xml_mutex ;

void create_xml_mutex (void)
begin

  pthread_mutex_init (addr(xml_mutex), NULL) ;
end

void xml_lock (void)
begin

  pthread_mutex_lock (addr(xml_mutex)) ;
end

void xml_unlock (void)
begin

  pthread_mutex_unlock (addr(xml_mutex)) ;
end
#endif

longint gcrccalc (pbyte p, longint len)
begin
  longint crc ;
  integer temp ;

  crc = 0 ;
  while (len > 0)
    begin
      temp = ((crc shr 24) xor *p++) and 255 ;
      crc = (crc shl 8) xor CRC_TABLE[temp] ;
      dec(len) ;
    end
  return crc ;
end

void gcrcupdate (pbyte p, longint len, longint *crc)
begin
  integer temp ;

  while (len-- > 0)
    begin
      temp = ((*crc shr 24) xor *p++) and 255 ;
      *crc = (*crc shl 8) xor CRC_TABLE[temp] ;
    end
  return ;
end

/* upshift a C string */
pchar q660_upper (pchar s)
begin
  size_t i ;

  for (i = 0 ; i < strlen(s) ; i++)
    s[i] = toupper (s[i]) ;
  return s ;
end

longint lib_round (double r)
begin
  longint result ;

  if (r >= 0.0)
    then
      result = r + 0.5 ;
    else
      result = r - 0.5 ;
  return result ;
end

/* Start of reading utilities */
pchar trim (string *s)
begin
  string s1 ;

  strcpy (s1, (pchar)s) ;
  while ((strlen(s1) > 0) land (s1[0] == ' '))
    memmove (s, addr(s1[1]), strlen(s1)) ;
  while ((strlen(s1) > 0) land (s1[strlen(s1) - 1] == ' '))
    s1[strlen(s1) - 1] = 0 ;
  strcpy ((pchar)s, s1) ;
  return (pchar)s ;
end

pchar xml_unescape (pchar s)
begin
  static string res ;
  integer i, lth ;
  boolean found ;
  pchar pc, pc2 ;

  strcpy (res, s) ;
  repeat
    found = FALSE ;
    for (i = 0 ; i < ESC_COUNT ; i++)
      begin
        pc = strstr(res, (pchar)addr(ESCAPES[i].escpd)) ;
        if (pc)
          then
            begin
              lth = (integer)strlen((pchar)addr(ESCAPES[i].escpd)) ;
              pc2 = pc ;
              incn(pc2, lth) ; /* skip past sequence */
              *pc++ = ESCAPES[i].unesc ;
              strcpy (pc, pc2) ; /* slide rest of string down */
              found = TRUE ;
            end
      end
  until (lnot found)) ;
  return res ;
end

boolean isclean (pchar f)
begin
  integer i, lth ;
  byte c ;

  lth = (integer)strlen(f) ;
  for (i = 0 ; i < lth ; i++)
    begin
      c = *f++ ;
      if ((c < 0x20) lor (c > 0x7e))
        then
          return FALSE ;
    end
  return TRUE ;
end

boolean read_next_tag (void)
begin
  integer i ;
  string s1, s2 ;
  pchar pc1, pc2 ;

  while (srcidx < srccount)
    begin
      strcpy (cur_xml_line, srcptrs[srcidx++]) ;
      last_xmlcrc = xmlcrc ;
      gcrcupdate ((pbyte)addr(cur_xml_line[0]), (integer)strlen(cur_xml_line), addr(xmlcrc)) ;
      startflag = FALSE ;
      endflag = FALSE ;
      compactflag = FALSE ;
      value[0] = 0 ;
      strcpy (s1, cur_xml_line) ;
      pc1 = strstr(s1, "<?") ;
      if (pc1)
        then
          continue ; /* Comment */
      pc1 = strchr(s1, '<') ;
      if (pc1 == NIL)
        then
          continue ;
      inc(pc1) ; /* skip to start of tag */
      pc2 = strchr(pc1, '>') ;
      if (pc2 == NIL)
        then
          continue ;
      s1[0] = 0 ;
      i = (integer)((pntrint)pc2 - (pntrint)pc1) ; /* number of characters in tag */
      if (i)
        then
          memcpy (addr(s2), pc1, i) ; /* first tag */
      s2[i] = 0 ;
      inc(pc2) ; /* pc2 now start of value and possible ending tag */
      if ((strlen(s2) > 1) land (s2[0] == '/'))
        then
          begin
            endflag = TRUE ; /* endflag only */
            strcpy (tag, addr(s2[1])) ; /* remove / */
          end
        else
          begin
            strcpy (tag, s2) ;
            startflag = TRUE ; /* have start flag */
            pc1 = strstr(pc2, "</") ;
            if (pc1)
              then
                begin /* start of ending tag */
                  i = (integer)((pntrint)pc1 - (pntrint)pc2) ;
                  if (i)
                    then
                      memcpy (addr(value), pc2, i) ;
                  value[i] = 0 ;
                  trim (addr(value)) ;
                  incn(pc1, 2) ; /* skip </ */
                  pc2 = strchr(pc1, '>') ;
                  if (pc2 == NIL)
                    then
                      begin
                        inc(error_counts[cur_sect]) ;
                        return TRUE ;
                      end
                  i = (integer)((pntrint)pc2 - (pntrint)pc1) ; /* number of characters in tag */
                  if (i)
                    then
                      memcpy (addr(s2), pc1, i) ;
                  s2[i] = 0 ;
                  if (strcmp(tag, s2))
                    then
                      begin
                        inc(error_counts[cur_sect]) ;
                        return TRUE ;
                      end
                  endflag = TRUE ; /* have both */
                end
          end
      if ((lnot insection) land (startflag))
        then
          begin
            if ((nesting == 1) land (strcmp(tag, sectname) == 0))
              then
                begin
                  insection = TRUE ;
                  xmlcrc = 0 ;
                  last_xmlcrc = xmlcrc ;
                  gcrcupdate ((pbyte)addr(cur_xml_line[0]), (integer)strlen(cur_xml_line), addr(xmlcrc)) ;
                  return FALSE ; /* found section */
                end
              else
                nesting++ ;
          end
      if (endflag)
        then
          begin
            if ((insection) land (strcmp(tag, sectname) == 0))
              then
                return TRUE ; /* end of section */
            else if (lnot insection)
              then
                nesting-- ; /* end of unused section */
          end
      if (insection)
        then
          return FALSE ; /* found entry */
    end
  return TRUE ;
end

integer match_tag (pxmldefarray pxe, integer cnt) /* Returns -1 for error */
begin
  boolean isarray ;
  integer j, good ;
  pxmldef xe ;
  pchar pc ;
  tarraystack *pas ;

  idx = 0 ;
  while (idx < cnt)
    begin
      xe = addr((*pxe)[idx]) ;
      if (((xe->x_type == X_ARRAY) land (startflag)) lor
          ((xe->x_type == X_EARRAY) land (endflag)))
        then
          begin
            isarray = TRUE ;
            if (strcmp(tag, "calmon") == 0)
              then
                strcpy (tag, "chan7") ;
          end
        else
          isarray = FALSE ;
      if ((lnot isarray) land (strcmp(tag, xe->name) == 0))
        then
          return idx ; /* found it */
      else if (isarray)
        then
          begin
            pc = strstr(tag, xe->name) ;
            if (pc)
              then
                begin /* starts the tag, get the part after the tag */
                  incn(pc, strlen(xe->name)) ;
                  if (xe->bopts > 9)
                    then
                      begin
                        j = (integer)*pc ; /* Is a letter */
                        good = 1 ;
                      end
                    else
                      good = sscanf(pc, "%d", addr(j)) ; /* Is a number */
                  if (good == 1)
                    then
                      begin /* is an array start or end */
                        if (startflag)
                          then
                            begin
                              j = j - xe->bopts ;
                              if ((j < 0) lor (j >= xe->wopts))
                                then
                                  return -1 ;
                              if (inarray)
                                then
                                  inc(curdepth) ;
                              pas = addr(arraystack[curdepth]) ;
                              pas->arrayidx = j ;
                              strcpy (pas->arrayname, xe->name) ;
                              pas->arraystart = xe->bopts ;
                              pas->arraycount = xe->wopts ;
                              pas->arraysize = xe->basesize ;
                              pas->arrayfirst = idx ;
                              if (curdepth == 1)
                                then
                                  pas->nestoffset = arraystack[0].arrayidx * arraystack[0].arraysize ;
                                else
                                  pas->nestoffset = 0 ;
                              inarray = TRUE ;
                              if ((endflag) land (xe->inv_com))
                                then
                                  begin /* all on one line, next entry has actual variable */
                                    compactflag = TRUE ;
                                    inc(idx) ;
                                  end
                              return idx ;
                            end
                        else if (endflag)
                          then
                            begin
                              pas = addr(arraystack[curdepth]) ;
                              j = j - pas->arraystart ;
                              if ((j < 0) lor (j >= pas->arraycount))
                                then
                                  return -1 ;
                              if (curdepth > 0)
                                then
                                  dec(curdepth) ; /* pop stack */
                                else
                                  inarray = FALSE ; /* nothing left to pop */
                              return idx ;
                            end
                      end
                end
          end
      inc(idx) ;
    end
  return -1 ;
end

boolean read_xml_start (pchar section)
begin

  crcvalid = TRUE ;
  sectcrc = 0 ;
  inarray = FALSE ;
  insection = FALSE ;
  curdepth = 0 ;
  nesting = 0 ;
  strcpy (sectname, section) ;
  srcidx = 0 ;
  if (lnot read_next_tag ())
    then
      return TRUE ; /* found section */
    else
      return FALSE ;
end

void proc_tag (pxmldef xe)
begin
  pointer thisfield ;
  pbyte pb ;
  shortint *psi ;
  pword pw ;
  int16 *pi ;
  longword *pl ;
  longint *pli ;
  string *ps ;
  single *pr ;
  double *pd ;
  integer i, good ;
  unsigned int l ;
  single r ;
  double d ;
  longint xmlcrc_comp ;
  tarraystack *pas ;

  if (xe->x_type != X_SIMP)
    then
      return ; /* at this point, must be simple type */
  if (lnot ((startflag) land (endflag)))
    then
      return ;
  if (value[0] == 0)
    then
      begin /* null value, usually an error */
        if (xe->stype == T_STRING)
          then
            return ; /* except for strings */
          else
            begin
              inc(error_counts[cur_sect]) ;
              return ;
            end
      end
  if (inarray)
    then
      begin
        pas = addr(arraystack[curdepth]) ;
        thisfield = (pointer)((pntrint)xe->svar + pas->arrayidx * pas->arraysize + pas->nestoffset) ;
      end
    else
      thisfield = xe->svar ;
  switch (xe->stype) begin
    case T_BYTE :
      pb = thisfield ;
      good = sscanf(value, "%u", addr(i)) ;
      if (good == 1)
        then
          *pb = i ;
        else
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      break ;
   case T_SHORT :
      psi = thisfield ;
      good = sscanf(value, "%d", addr(i)) ;
      if ((i < -128) lor (i > 127))
        then
          good = 0 ;
      if (good == 1)
        then
          *psi = i ;
        else
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      break ;
    case T_WORD :
      pw = thisfield ;
      good = sscanf(value, "%d", addr(i)) ;
      if ((i < 0) lor (i > 65535))
        then
          good = 0 ;
      if (good == 1)
        then
          *pw = i ;
        else
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      break ;
    case T_INT :
      pi = thisfield ;
      good = sscanf(value, "%d", addr(i)) ;
      if ((i < -32768) lor (i > 32767))
        then
          good = 0 ;
      if (good == 1)
        then
          *pi = i ;
        else
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      break ;
    case T_LWORD :
      pl = thisfield ;
      good = sscanf(value, "%u", addr(l)) ;
      if (good == 1)
        then
          *pl = l ;
        else
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      break ;
    case T_LINT :
      pli = thisfield ;
      good = sscanf(value, "%d", addr(i)) ;
      if (good == 1)
        then
          *pli = i ;
        else
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      break ;
    case T_CRC :
      good = sscanf(value, "%x", addr(l)) ;
      if (good != 1)
        then
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      xmlcrc_comp = l ;
      sectcrc = last_xmlcrc ;
      crcvalid = (xmlcrc_comp == last_xmlcrc) ;
      break ;
    case T_SINGLE :
      pr = thisfield ;
      good = sscanf(value, "%f", addr(r)) ;
      if (good != 1)
        then
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      *pr = r ;
      break ;
    case T_DOUBLE :
      pd = thisfield ;
      good = sscanf(value, "%lf", addr(d)) ;
      if (good != 1)
        then
          begin
            inc(error_counts[cur_sect]) ;
            return ;
          end
      *pd = d ;
      break ;
    case T_STRING :
      ps = thisfield ;
      i = (integer)strlen(value) ;
      if ((i >= 2) land (value[0] == '"') land (value[i - 1] == '"'))
        then
          begin
            value[i - 1] = 0 ; /* remove trailing quotes */
            memmove (addr(value[0]), addr(value[1]), i) ; /* remove leading quote */
          end
      strcpy (value, xml_unescape (value)) ;
      if (lnot isclean (value))
        then
          value[0] = 0 ;
      if ((byte)strlen(value) > xe->bopts)
        then
          begin
            value[xe->bopts] = 0 ;
            inc(error_counts[cur_sect]) ;
          end
      strcpy((pchar)ps, value) ;
      break ;
  end
  if (compactflag)
    then
      begin /* already done with this array */
        if (curdepth > 0)
          then
            dec(curdepth) ; /* pop stack */
          else
            inarray = FALSE ; /* nothing left to pop */
      end
  return ;
end

void read_xml (pxmldefarray pxe, integer cnt, enum txmlsect sect)
begin
  integer i ;

  cur_sect = sect ;
  while (lnot read_next_tag ())
    begin
      i = match_tag (pxe, cnt) ;
      if (i >= 0)
        then
          proc_tag (addr((*pxe)[i])) ;
        else
          inc(error_counts[cur_sect]) ;
    end
  if (crcvalid)
    then
      crc_err_map = crc_err_map and not (1L shl cur_sect) ;
    else
      crc_err_map = crc_err_map or (1 shl cur_sect) ;
  load_map = load_map or (1 shl cur_sect) ;
  return ;
end

/* Returns index in table if match, -1 if no match */
integer match_field (pfieldarray pfld, integer cnt, pchar pc)
begin
  integer i ;
  string23 s, s1 ;

  strcpy (s, pc) ;
  q660_upper (s) ;
  for (i = 0 ; i < cnt ; i++)
    begin
      strcpy (s1, (pchar)addr((*pfld)[i])) ;
      q660_upper (s1) ;
      if (strcmp (s, s1) == 0)
        then
          return i ;
    end
  return -1 ;
end









