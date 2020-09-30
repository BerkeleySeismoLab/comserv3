/*   Lib660 Detector Definitions
     Copyright 2017 Certified Software Corporation

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
*/
#ifndef libdetect_h
/* Flag this file as included */
#define libdetect_h
#define VER_LIBDETECT 0

#include "libtypes.h"
#include "libstrucs.h"
#include "libsampglob.h"

extern void initialize_detector (pq660 q660, pdet_packet pdp, piirfilter pi) ;
extern boolean Te_detect (tdet_packet *detector) ;
extern enum tliberr lib_detstat (pq660 q660, tdetstat *detstat) ;
extern void lib_changeenable (pq660 q660, tdetchange *detchange) ;

#define MAXSAMP 38
#define EV_OFF 2
#define NOR_OUT 4
#define CUR_MAX 50
#define MINPOINTS 20 /* minimum number of points to run detection on */
enum slope_type {ST_POS, ST_NEG, ST_NEITHER} ;

/*------------------The continuity structures-------------------------------*/
typedef struct {
  boolean detector_on ; /* if last record was detected on */
  boolean detection_declared ; /* detector_on filtered with first_detection */
  boolean first_detection ; /* if this is first detection after startup */
  boolean detector_enabled ; /* currently enabled */
  boolean new_onset ; /* TRUE if new onset */
  boolean default_enabled ; /* detector enabled by default */
  integer sampcnt ; /* Number of samples in "insamps" so far */
  longint total_detections ; /* used to keep track of detections over time */
  double startt ; /* Starting time of first data point */
  double etime ;
} con_common ;

/*-------------------------Te_detect continuity structure---------------*/
typedef struct {
  boolean detector_on ; /* if last record was detected on */
  boolean detection_declared ; /* detector_on filtered with first_detection */
  boolean first_detection ; /* if this is first detection after startup */
  boolean detector_enabled ; /* currently enabled */
  boolean new_onset ; /* TRUE if new onset */
  boolean default_enabled ; /* detector enabled by default */
  integer sampcnt ; /* Number of samples in "insamps" so far */
  longint total_detections ; /* used to keep track of detections over time */
  double startt ; /* Starting time of first data point */
  double etime ;
  longint peakhi ; /*highest value of high limit*/
  longint peaklo ; /*lowest value of low limit*/
  longint waitdly ; /*sample countdown*/
  integer overhi ; /*number of points over the high limit*/
  integer overlo ; /*number of points under the low limit*/
  boolean hevon ; /*high limit event on*/
  boolean levon ; /*low limit event on*/
  tfloat sample_rate ; /* sample rate in seconds */
  tonset_base *onsetdata ;
  pdet_packet parent ;
} threshold_control_struc ;
typedef threshold_control_struc *pthreshold_control_struc ;

#endif
