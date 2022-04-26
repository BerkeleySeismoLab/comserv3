



/*
     XML Read Support Routines
     Copyright 2015-2017 by
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
    0 2015-12-15 rdr Adapted from WS330.
    1 2015-12-28 rdr Add support for letters as array index in tag.
    2 2016-01-08 rdr Replace calls to malloc with getbuf.
    3 2016-01-17 rdr Allow null string values.
    4 2016-01-18 rdr Change error handling to process as much as possible but
                     keep track of error count per section.
    5 2016-02-07 rdr Add buffer override parameter to load_cfg_xml.
    6 2016-02-19 rdr Allow mixed-case field lists in match_field.
    7 2017-06-14 rdr Fix processing of T_DOUBLE.
    8 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#include "platform.h"
#include "xmlsup.h"
#include <ctype.h>

typedef char tchararray[256] ;

enum txmlsect cur_sect ;
int xmlnest ;
int srccount ;
int srcidx ;
string value ;
string tag ;
BOOLEAN startflag, endflag ;
string cur_xml_line ;
tsrcptrs srcptrs ;
pmeminst pxmlmem ;        /* Current memory manager pointer */

/* following returned from read_xml */
BOOLEAN crcvalid ;
I32 sectcrc ;
U32 crc_err_map ; /* Bitmap of structures with CRC errors */
U32 load_map ; /* Bitmap of structures loaded */
/* end of caller variables */

static int idx ;
static BOOLEAN inarray ;
static tarraystack arraystack[ARRAY_DEPTH] ;
static int curdepth ; /* current array curdepth, 0 .. 2 */
static int nesting ; /* current tag nesting */
static BOOLEAN insection, compactflag ;
static string sectname ;
static I32 xmlcrc, last_xmlcrc ;

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
    776956112, 2018012088, -2109641216, -733581976
} ;

const tescapes ESCAPES = {
    {'&', "&amp;"}, {'<', "&lt;"}, {'>', "&gt;"}, {'\'', "&apos;"}, {'\"', "&quot;"}
} ;
const tsectnames SECTNAMES = {"System Info", "Writer", "Sensors", "Main Digitizer",
                              "Accelerometer", "Mesh Configuration", "Timing", "Options", "Network", "Announcements",
                              "Auto Mass Recenter", "Station Information", "IIR Filters", "Main Digitizer & Accelerometer",
                              "State of Health", "Mesh Channels", "Engineering Data"
                             } ;

/* XML Mutex Routines. To use the XML parser the calling program must call
   create_xml_mutex once. For each thread that needs to use the routines
   call xml_lock, do any initialization, do the parsing, copy the result
   into thread storage, and then call xml_unlock. These steps can be bypassed
   if the calling program only has one thread that will use the parser. */
#ifdef X86_WIN32
static HANDLE xml_mutex ;

void create_xml_mutex (void)
{

    xml_mutex = CreateMutex(NIL, FALSE, NIL) ;
}

void xml_lock (void)
{

    WaitForSingleObject (xml_mutex, INFINITE) ;
}

void xml_unlock (void)
{

    ReleaseMutex (xml_mutex) ;
}

#else
static pthread_mutex_t xml_mutex ;

void create_xml_mutex (void)
{

    pthread_mutex_init (&(xml_mutex), NULL) ;
}

void xml_lock (void)
{

    pthread_mutex_lock (&(xml_mutex)) ;
}

void xml_unlock (void)
{

    pthread_mutex_unlock (&(xml_mutex)) ;
}
#endif

I32 gcrccalc (PU8 p, I32 len)
{
    I32 crc ;
    int temp ;

    crc = 0 ;

    while (len > 0) {
        temp = ((crc >> 24) ^ *p++) & 255 ;
        crc = (crc << 8) ^ CRC_TABLE[temp] ;
        (len)-- ;
    }

    return crc ;
}

void gcrcupdate (PU8 p, I32 len, I32 *crc)
{
    int temp ;

    while (len-- > 0) {
        temp = ((*crc >> 24) ^ *p++) & 255 ;
        *crc = (*crc << 8) ^ CRC_TABLE[temp] ;
    }

    return ;
}

/* upshift a C string */
pchar q660_upper (pchar s)
{
    size_t i ;

    for (i = 0 ; i < strlen(s) ; i++)
        s[i] = toupper (s[i]) ;

    return s ;
}

I32 lib_round (double r)
{
    I32 result ;

    if (r >= 0.0)
        result = r + 0.5 ;
    else
        result = r - 0.5 ;

    return result ;
}

/* Start of reading utilities */
pchar trim (string *s)
{
    string s1 ;

    strcpy (s1, (pchar)s) ;

    while ((strlen(s1) > 0) && (s1[0] == ' '))
        memmove (s, &(s1[1]), strlen(s1)) ;

    while ((strlen(s1) > 0) && (s1[strlen(s1) - 1] == ' '))
        s1[strlen(s1) - 1] = 0 ;

    strcpy ((pchar)s, s1) ;
    return (pchar)s ;
}

pchar xml_unescape (pchar s)
{
    static string res ;
    int i, lth ;
    BOOLEAN found ;
    pchar pc, pc2 ;

    strcpy (res, s) ;

    do {
        found = FALSE ;

        for (i = 0 ; i < ESC_COUNT ; i++) {
            pc = strstr(res, (pchar)&(ESCAPES[i].escpd)) ;

            if (pc) {
                lth = (int)strlen((pchar)&(ESCAPES[i].escpd)) ;
                pc2 = pc ;
                pc2 = pc2 + (lth) ; /* skip past sequence */
                *pc++ = ESCAPES[i].unesc ;
                strcpy (pc, pc2) ; /* slide rest of string down */
                found = TRUE ;
            }
        }
    } while (! (! found)) ;

    return res ;
}

BOOLEAN isclean (pchar f)
{
    int i, lth ;
    U8 c ;

    lth = (int)strlen(f) ;

    for (i = 0 ; i < lth ; i++) {
        c = *f++ ;

        if ((c < 0x20) || (c > 0x7e))
            return FALSE ;
    }

    return TRUE ;
}

BOOLEAN read_next_tag (void)
{
    int i ;
    string s1, s2 ;
    pchar pc1, pc2 ;

    while (srcidx < srccount) {
        strcpy (cur_xml_line, srcptrs[srcidx++]) ;
        last_xmlcrc = xmlcrc ;
        gcrcupdate ((PU8)&(cur_xml_line[0]), (int)strlen(cur_xml_line), &(xmlcrc)) ;
        startflag = FALSE ;
        endflag = FALSE ;
        compactflag = FALSE ;
        value[0] = 0 ;
        strcpy (s1, cur_xml_line) ;
        pc1 = strstr(s1, "<?") ;

        if (pc1)
            continue ; /* Comment */

        pc1 = strchr(s1, '<') ;

        if (pc1 == NIL)
            continue ;

        (pc1)++ ; /* skip to start of tag */
        pc2 = strchr(pc1, '>') ;

        if (pc2 == NIL)
            continue ;

        s1[0] = 0 ;
        i = (int)((PNTRINT)pc2 - (PNTRINT)pc1) ; /* number of characters in tag */

        if (i)
            memcpy (&(s2), pc1, i) ; /* first tag */

        s2[i] = 0 ;
        (pc2)++ ; /* pc2 now start of value and possible ending tag */

        if ((strlen(s2) > 1) && (s2[0] == '/')) {
            endflag = TRUE ; /* endflag only */
            strcpy (tag, &(s2[1])) ; /* remove / */
        } else {
            strcpy (tag, s2) ;
            startflag = TRUE ; /* have start flag */
            pc1 = strstr(pc2, "</") ;

            if (pc1) {
                /* start of ending tag */
                i = (int)((PNTRINT)pc1 - (PNTRINT)pc2) ;

                if (i)
                    memcpy (&(value), pc2, i) ;

                value[i] = 0 ;
                trim (&(value)) ;
                pc1 = pc1 + (2) ; /* skip </ */
                pc2 = strchr(pc1, '>') ;

                if (pc2 == NIL) {
                    (error_counts[cur_sect])++ ;
                    return TRUE ;
                }

                i = (int)((PNTRINT)pc2 - (PNTRINT)pc1) ; /* number of characters in tag */

                if (i)
                    memcpy (&(s2), pc1, i) ;

                s2[i] = 0 ;

                if (strcmp(tag, s2)) {
                    (error_counts[cur_sect])++ ;
                    return TRUE ;
                }

                endflag = TRUE ; /* have both */
            }
        }

        if ((! insection) && (startflag)) {
            if ((nesting == 1) && (strcmp(tag, sectname) == 0)) {
                insection = TRUE ;
                xmlcrc = 0 ;
                last_xmlcrc = xmlcrc ;
                gcrcupdate ((PU8)&(cur_xml_line[0]), (int)strlen(cur_xml_line), &(xmlcrc)) ;
                return FALSE ; /* found section */
            } else
                nesting++ ;
        }

        if (endflag) {
            if ((insection) && (strcmp(tag, sectname) == 0))
                return TRUE ; /* end of section */
            else if (! insection)
                nesting-- ; /* end of unused section */
        }

        if (insection)
            return FALSE ; /* found entry */
    }

    return TRUE ;
}

int match_tag (pxmldefarray pxe, int cnt) /* Returns -1 for error */
{
    BOOLEAN isarray ;
    int j, good ;
    pxmldef xe ;
    pchar pc ;
    tarraystack *pas ;

    idx = 0 ;

    while (idx < cnt) {
        xe = &((*pxe)[idx]) ;

        if (((xe->x_type == X_ARRAY) && (startflag)) ||
                ((xe->x_type == X_EARRAY) && (endflag)))

        {
            isarray = TRUE ;

            if (strcmp(tag, "calmon") == 0)
                strcpy (tag, "chan7") ;
        } else
            isarray = FALSE ;

        if ((! isarray) && (strcmp(tag, xe->name) == 0))
            return idx ; /* found it */
        else if (isarray) {
            pc = strstr(tag, xe->name) ;

            if (pc) {
                /* starts the tag, get the part after the tag */
                pc = pc + (strlen(xe->name)) ;

                if (xe->bopts > 9) {
                    j = (int)*pc ; /* Is a letter */
                    good = 1 ;
                } else
                    good = sscanf(pc, "%d", &(j)) ; /* Is a number */

                if (good == 1) {
                    /* is an array start or end */
                    if (startflag) {
                        j = j - xe->bopts ;

                        if ((j < 0) || (j >= xe->wopts))
                            return -1 ;

                        if (inarray)
                            (curdepth)++ ;

                        pas = &(arraystack[curdepth]) ;
                        pas->arrayidx = j ;
                        strcpy (pas->arrayname, xe->name) ;
                        pas->arraystart = xe->bopts ;
                        pas->arraycount = xe->wopts ;
                        pas->arraysize = xe->basesize ;
                        pas->arrayfirst = idx ;

                        if (curdepth == 1)
                            pas->nestoffset = arraystack[0].arrayidx * arraystack[0].arraysize ;
                        else
                            pas->nestoffset = 0 ;

                        inarray = TRUE ;

                        if ((endflag) && (xe->inv_com)) {
                            /* all on one line, next entry has actual variable */
                            compactflag = TRUE ;
                            (idx)++ ;
                        }

                        return idx ;
                    } else if (endflag) {
                        pas = &(arraystack[curdepth]) ;
                        j = j - pas->arraystart ;

                        if ((j < 0) || (j >= pas->arraycount))
                            return -1 ;

                        if (curdepth > 0)
                            (curdepth)-- ; /* pop stack */
                        else
                            inarray = FALSE ; /* nothing left to pop */

                        return idx ;
                    }
                }
            }
        }

        (idx)++ ;
    }

    return -1 ;
}

BOOLEAN read_xml_start (pchar section)
{

    crcvalid = TRUE ;
    sectcrc = 0 ;
    inarray = FALSE ;
    insection = FALSE ;
    curdepth = 0 ;
    nesting = 0 ;
    strcpy (sectname, section) ;
    srcidx = 0 ;

    if (! read_next_tag ())
        return TRUE ; /* found section */
    else
        return FALSE ;
}

void proc_tag (pxmldef xe)
{
    pointer thisfield ;
    PU8 pb ;
    I8 *psi ;
    PU16 pw ;
    I16 *pi ;
    U32 *pl ;
    I32 *pli ;
    string *ps ;
    float *pr ;
    double *pd ;
    int i, good ;
    unsigned int l ;
    float r ;
    double d ;
    I32 xmlcrc_comp ;
    tarraystack *pas ;

    if (xe->x_type != X_SIMP)
        return ; /* at this point, must be simple type */

    if (! ((startflag) && (endflag)))
        return ;

    if (value[0] == 0) {
        /* null value, usually an error */
        if (xe->stype == T_STRING)
            return ; /* except for strings */
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }
    }

    if (inarray) {
        pas = &(arraystack[curdepth]) ;
        thisfield = (pointer)((PNTRINT)xe->svar + pas->arrayidx * pas->arraysize + pas->nestoffset) ;
    } else
        thisfield = xe->svar ;

    switch (xe->stype) {
    case T_BYTE :
        pb = thisfield ;
        good = sscanf(value, "%u", &(i)) ;

        if (good == 1)
            *pb = i ;
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }

        break ;

    case T_SHORT :
        psi = thisfield ;
        good = sscanf(value, "%d", &(i)) ;

        if ((i < -128) || (i > 127))
            good = 0 ;

        if (good == 1)
            *psi = i ;
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }

        break ;

    case T_WORD :
        pw = thisfield ;
        good = sscanf(value, "%d", &(i)) ;

        if ((i < 0) || (i > 65535))
            good = 0 ;

        if (good == 1)
            *pw = i ;
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }

        break ;

    case T_INT :
        pi = thisfield ;
        good = sscanf(value, "%d", &(i)) ;

        if ((i < -32768) || (i > 32767))
            good = 0 ;

        if (good == 1)
            *pi = i ;
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }

        break ;

    case T_LWORD :
        pl = thisfield ;
        good = sscanf(value, "%u", &(l)) ;

        if (good == 1)
            *pl = l ;
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }

        break ;

    case T_LINT :
        pli = thisfield ;
        good = sscanf(value, "%d", &(i)) ;

        if (good == 1)
            *pli = i ;
        else {
            (error_counts[cur_sect])++ ;
            return ;
        }

        break ;

    case T_CRC :
        good = sscanf(value, "%x", &(l)) ;

        if (good != 1) {
            (error_counts[cur_sect])++ ;
            return ;
        }

        xmlcrc_comp = l ;
        sectcrc = last_xmlcrc ;
        crcvalid = (xmlcrc_comp == last_xmlcrc) ;
        break ;

    case T_SINGLE :
        pr = thisfield ;
        good = sscanf(value, "%f", &(r)) ;

        if (good != 1) {
            (error_counts[cur_sect])++ ;
            return ;
        }

        *pr = r ;
        break ;

    case T_DOUBLE :
        pd = thisfield ;
        good = sscanf(value, "%lf", &(d)) ;

        if (good != 1) {
            (error_counts[cur_sect])++ ;
            return ;
        }

        *pd = d ;
        break ;

    case T_STRING :
        ps = thisfield ;
        i = (int)strlen(value) ;

        if ((i >= 2) && (value[0] == '"') && (value[i - 1] == '"')) {
            value[i - 1] = 0 ; /* remove trailing quotes */
            memmove (&(value[0]), &(value[1]), i) ; /* remove leading quote */
        }

        strcpy (value, xml_unescape (value)) ;

        if (! isclean (value))
            value[0] = 0 ;

        if ((U8)strlen(value) > xe->bopts) {
            value[xe->bopts] = 0 ;
            (error_counts[cur_sect])++ ;
        }

        strcpy((pchar)ps, value) ;
        break ;
    }

    if (compactflag) {
        /* already done with this array */
        if (curdepth > 0)
            (curdepth)-- ; /* pop stack */
        else
            inarray = FALSE ; /* nothing left to pop */
    }

    return ;
}

void read_xml (pxmldefarray pxe, int cnt, enum txmlsect sect)
{
    int i ;

    cur_sect = sect ;

    while (! read_next_tag ()) {
        i = match_tag (pxe, cnt) ;

        if (i >= 0)
            proc_tag (&((*pxe)[i])) ;
        else
            (error_counts[cur_sect])++ ;
    }

    if (crcvalid)
        crc_err_map = crc_err_map & ~ (1L << cur_sect) ;
    else
        crc_err_map = crc_err_map | (1 << cur_sect) ;

    load_map = load_map | (1 << cur_sect) ;
    return ;
}

/* Returns index in table if match, -1 if no match */
int match_field (pfieldarray pfld, int cnt, pchar pc)
{
    int i ;
    string23 s, s1 ;

    strcpy (s, pc) ;
    q660_upper (s) ;

    for (i = 0 ; i < cnt ; i++) {
        strcpy (s1, (pchar)&((*pfld)[i])) ;
        q660_upper (s1) ;

        if (strcmp (s, s1) == 0)
            return i ;
    }

    return -1 ;
}









