



/*   Lib660 Detector Definitions
     Copyright 2017 by
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
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef libdetect_h
/* Flag this file as included */
#define libdetect_h
#define VER_LIBDETECT 1

#include "libtypes.h"
#include "libstrucs.h"
#include "libsampglob.h"

extern void initialize_detector (pq660 q660, pdet_packet pdp, piirfilter pi) ;
extern BOOLEAN Te_detect (tdet_packet *detector) ;
extern enum tliberr lib_detstat (pq660 q660, tdetstat *detstat) ;
extern void lib_changeenable (pq660 q660, tdetchange *detchange) ;

#define MAXSAMP 38
#define EV_OFF 2
#define NOR_OUT 4
#define CUR_MAX 50
#define MINPOINTS 20 /* minimum number of points to run detection on */
enum slope_type
{ST_POS, ST_NEG, ST_NEITHER} ;

/*------------------The continuity structures-------------------------------*/
typedef struct
{
    BOOLEAN detector_on ; /* if last record was detected on */
    BOOLEAN detection_declared ; /* detector_on filtered with first_detection */
    BOOLEAN first_detection ; /* if this is first detection after startup */
    BOOLEAN detector_enabled ; /* currently enabled */
    BOOLEAN new_onset ; /* TRUE if new onset */
    BOOLEAN default_enabled ; /* detector enabled by default */
    int sampcnt ; /* Number of samples in "insamps" so far */
    I32 total_detections ; /* used to keep track of detections over time */
    double startt ; /* Starting time of first data point */
    double etime ;
} con_common ;

/*-------------------------Te_detect continuity structure---------------*/
typedef struct
{
    BOOLEAN detector_on ; /* if last record was detected on */
    BOOLEAN detection_declared ; /* detector_on filtered with first_detection */
    BOOLEAN first_detection ; /* if this is first detection after startup */
    BOOLEAN detector_enabled ; /* currently enabled */
    BOOLEAN new_onset ; /* TRUE if new onset */
    BOOLEAN default_enabled ; /* detector enabled by default */
    int sampcnt ; /* Number of samples in "insamps" so far */
    I32 total_detections ; /* used to keep track of detections over time */
    double startt ; /* Starting time of first data point */
    double etime ;
    I32 peakhi ; /*highest value of high limit*/
    I32 peaklo ; /*lowest value of low limit*/
    I32 waitdly ; /*sample countdown*/
    int overhi ; /*number of points over the high limit*/
    int overlo ; /*number of points under the low limit*/
    BOOLEAN hevon ; /*high limit event on*/
    BOOLEAN levon ; /*low limit event on*/
    tfloat sample_rate ; /* sample rate in seconds */
    tonset_base *onsetdata ;
    pdet_packet parent ;
} threshold_control_struc ;
typedef threshold_control_struc *pthreshold_control_struc ;

#endif
