/*   Lib660 time series handling definitions
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
    0 2017-06-09 rdr Created
*/
#ifndef libsampglob_h
/* Flag this file as included */
#define libsampglob_h
#define VER_LIBSAMPGLOB 1

#include "libtypes.h"
#include "libseed.h"
#include "libclient.h"

/* Send to Client destination bitmaps */
#define SCD_ARCH 1 /* send to archival output */
#define SCD_512 2 /* send to 512 byte miniseed output */
#define SCD_BOTH 3 /* send to both */

#define MAXSAMP 38
#define FIRMAXSIZE 400
#define MAXPOLES 8       /* Maximum number of poles in recursive filters */
#define FILTER_NAME_LENGTH 31 /* Maximum number of characters in an IIR filter name */
#define PEEKELEMS 16
#define PEEKMASK 15 /* TP7 doesn't optimize mod operation */
#define CFG_TIMEOUT 120 /* seconds since last config blockette added before flush */
#define LOG_TIMEOUT 120 /* seconds since last message line added before flush */
#define Q_OFF 80 /* Clock quality for GPS lock but no PLL lock */
#define Q_LOW 10 /* Minimum quality for data */
#define NO_LAST_DATA_QUAL 999 /* initial value */

enum tevent_detector {STALTA, THRESHOLD} ;
/*
  tiirdef is a definition of an IIR filter which may be used multiple places
*/
typedef double tvector[MAXPOLES + 1] ;
typedef struct {
  byte poles ;
  boolean highpass ;
  byte spare ;
  single ratio ; /* ratio * sampling_frequency = corner */
  tvector a ;
  tvector b ;
} tsect_base ;
/*
  tiirfilter is one implementation of a filter on a specific LCQ
*/
typedef struct {
  byte poles ;
  boolean highpass ;
  byte spare ;
  single ratio ; /* ratio * sampling_frequency = corner */
  tvector a ;
  tvector b ;
  tvector x ;
  tvector y ;
} tiirsection ;
typedef struct tiirfilter {
  struct tiirfilter *link ; /* next filter */
  piirdef def ; /* definition of this filter */
  integer sects ;
  word packet_size ; /* total size of this packet */
  tiirsection filt[MAXSECTIONS + 1] ;
  tfloat out ; /*may be an array*/
} tiirfilter ;
typedef tiirfilter *piirfilter ;
/*
  Tfir_packet is the actual implementation of one FIR filter on a particular LCQ
*/
typedef tfloat *pfloat ;
typedef struct {
  pfloat fbuf ; /* pointer to FIR filter buffer */
  pfloat f ; /* working ptr into FIR buffer */
  pfloat fcoef ; /* ptr to floating pnt FIR coefficients */
  longint flen ; /* number of coef in FIR filter */
  longint fdec ; /* number of FIR inp samps per output samp */
  longint fcount ; /* current number of samps in FIR buffer */
} tfir_packet ;
typedef tfir_packet *pfir_packet ;
/*
  tdetload defines operating constants for murdock-hutt and threshold detectors
*/
typedef struct {
  longint filhi, fillo ; /* threshold limits */
  longint iwin ; /* window length in samples & threshold hysterisis */
  longint n_hits ; /* #P-T >= th2 for detection & threshold min. dur. */
  longint xth1, xth2, xth3, xthx ; /* coded threshold factors */
  longint def_tc ; /* time correcton for onset (default) */
  longint wait_blk ; /* controls re-activation of detector
                        and recording time in event code & threshold too */
  integer val_avg ; /* the number of values in tsstak[] */
} tdetload ;

typedef tfloat tsinglearray[MAX_RATE] ;
typedef tsinglearray *psinglearray ;
typedef longint tinsamps[MAXSAMP] ;
typedef tfloat trealsamps[MAXSAMP] ;
typedef longint tdataarray[MAX_RATE] ;
typedef tdataarray *pdataarray ;
typedef word tidxarray[MAX_RATE + 1] ;
typedef tidxarray *pidxarray ;
typedef longword tmergedbuf[MAX_RATE] ;
typedef tmergedbuf *pmergedbuf ;

/*
  tdetect defines a type of detector that can be used multiple times
*/
typedef struct tdetect {
  struct tdetector *link ; /* next in list of detectors */
  piirdef detfilt ; /* detector pre-filter, if any */
  tdetload uconst ; /*detector parameters*/
  byte detector_num ; /* detector number */
  enum tevent_detector dtype ; /* detector type */
  char detname[DETECTOR_NAME_LENGTH] ; /* detector name */
} tdetect ;
typedef tdetect *pdetect ;

/*
  Compressed buffer rings are used as pre-event buffers
*/
typedef struct tcompressed_buffer_ring {
  struct tcompressed_buffer_ring *link ; /* list link */
  seed_header hdr_buf ; /* for building header */
  completed_record rec ; /* ready to write format */
  boolean full ; /* if this record full */
} tcompressed_buffer_ring ;
typedef tcompressed_buffer_ring *pcompressed_buffer_ring ;
/*
  Downstream packets are used to filter a data stream and produce a new LCQ
*/
typedef struct tdownstream_packet {
  struct tdownstream_packet *link ; /* list link, NIL if end or no derived q's */
  pointer derived_q ; /* pointer to the lcq who looks at this flag, NIL if none */
} tdownstream_packet ;
typedef tdownstream_packet *pdownstream_packet ;
/*
  A com_packet is used to build up a compressed record using input from either
  the Q660 or another LCQ
*/
typedef struct {
  longint last_sample ; /* most recent sample for compression */
  longint flag_word ; /* for construction the flag longword */
  longint records_written ; /* count of buffers written */
  pcompressed_buffer_ring ring ; /* current element of buffer ring */
  pcompressed_buffer_ring last_in_ring ; /* last record in ring if non-NIL */
  compressed_frame frame_buffer ; /* frame we are currently compressing */
  word frame ; /* current compression frame */
  word maxframes ; /* maximum number of frames in a com record */
  integer ctabx ; /* current compression table index */
  integer block ; /* current compression block */
  integer peek_total ; /* number of samps in peek buffer */
  integer next_in ; /* peek buffer next-in index */
  integer next_out ; /* peek buffer next-out index */
  integer time_mark_sample ; /* sample number of time mark */
  integer next_compressed_sample ; /* next-in samp num in rec buf */
  integer blockette_count ; /* number of extra blockettes */
  integer blockette_index ; /* byte offset in record for next blockette (CNP) */
  integer last_blockette ; /* byte offset of last blockette */
  boolean charging ; /* filter charging */
  longint diffs[MAXSAMPPERWORD] ;
  longint sc[MAXSAMPPERWORD + 2] ;
  longint peeks[PEEKELEMS] ; /* compression buffer */
} tcom_packet ;
typedef tcom_packet *pcom_packet ;
/*
  A precomp record holds all the values associated with pre-compressed data from the Q660
*/
typedef struct {
  longint prev_sample ; /* previous sample from Q660 for decompression */
  longint prev_value ; /* from last decompression */
  integer block_idx ; /* index into source blocks */
  pbyte pmap ; /* pointer into blockette map */
  pbyte pdata ; /* pointer into blockette data */
  word mapidx ; /* indexes two bits at a time into pmap^ */
  word curmap ; /* current map word if mapidx <> 0 */
  integer blocks ; /* number of blocks to be decompressed */
  /* These are used when combining blockettes into one second worth */
  integer map_size ; /* Size of the merged map buffer */
  word map_cntr ; /* indexes two bits at a time into map_ptr^ */
  integer data_size ; /* Size of the merged data buffer */
  pbyte map_buf ; /* Start of map buffer */
  pbyte map_ptr ; /* Current map pointer */
  pbyte data_buf ; /* Start of data buffer */
  pbyte data_ptr ; /* Current data pointer */
} tprecomp ;
/* to build archival miniseed */
typedef struct {
  boolean appended ; /* data has been added */
  boolean existing_record ; /* from preload or incremental update */
  boolean incremental ; /* update every 512 byte record */
  boolean leave_in_buffer ; /* set to not clear out buffer after sending */
  word amini_filter ; /* OMF_xxx bits */
  integer total_frames ; /* sequential record filling index */
  integer frames_outstanding ; /* frames updated but not written */
  longint records_written ; /* count of buffers written */
  longint records_written_session ; /* this session */
  longint records_overwritten_session ; /* count of records overwritten */
  longword last_updated ; /* seconds since 2016 */
  seed_header hdr_buf ; /* for building header */
  pmax_cfr pcfr ;
} tarc ;
/*
  tlcq define one "Logical Channel Queue", corresponding to one SEED channel.
*/
typedef struct tlcq {
  struct tlcq *link ; /* forward link */
  pchan def ; /* pointer to the XML definition */
  enum tgdsrc gen_src ;      /* General Source */
  byte sub_field ;      /* sub field, any source, channel for MD and AC */
  tlocation location ; /* Seed Location */
  tseed_name seedname ; /* Seed Channel Name */
  boolean event_only ; /* Event only if set */
  byte gain_bits ; /* for DEB flags */
  string2 slocation ; /* dynamic length versions */
  integer lcq_num ;
  string3 sseedname ;
  word caldly ; /* number of seconds after cal over to turn off detection */
  word calinc ; /* count up timer for turning off detect flag*/
  integer rate ; /* + => samp per sec; - => sec per samp */
  boolean timemark_occurred ; /* set at the first sample */
  boolean cal_on ; /* calibration on */
  boolean calstat ; /* unfiltered calibration status */
  boolean validated ; /* DP LCQ is still in tokens */
  enum tpacket_class pack_class ; /* for sending to client */
  longword dtsequence ; /* data record sequence number currently being processed */
  longword seg_seq ; /* sequence number for segment collection */
  pidxarray idxbuf ; /* for converting frames into samples */
  pdataarray databuf ; /* raw input data */
  word datasize ; /* size of above structure */
  double timetag ; /* seconds since 2016 */
  double backup_tag ; /* in case >1hz data gets flushed between seconds */
  double last_timetag ; /* if not zero, timetag of last second of data */
  double delay ; /* Filter delay */
  word timequal ; /* quality from 0 to 100% */
  word backup_qual ; /* in case >1hz data gets flushed between seconds */
  single gap_threshold ; /* number of samples that constitutes a gap */
  tfloat gap_secs ; /* number of seconds constituting a gap */
  tfloat gap_offset ; /* expected number of seconds between new incoming samples */
  tprecomp precomp ; /* precompressed data fields */
  boolean slipping ; /* is derived stream, waiting for sync */
  longint slip_modulus ;
  tfloat input_sample_rate ; /* sample rate of input to decimation filter */
  pdownstream_packet downstream_link ; /* "stream_avail"'s for derived lcq's */
  struct tlcq *prev_link ; /* back link for checking fir-derived queue order */
  single firfixing_gain ; /* normally 1.0, typically <1.0 for goes */
  pcom_packet com ; /* this stream's compression packet(s) */
  pointer det ; /* head of this channel's detector chain, pointer because tdetpacket hasn't been defined */
  pfilter source_fir ; /* pointer to where "fir" came from */
  pfir_packet fir ; /* this stream's fir filter */
  boolean gen_on ; /* general detector on */
  boolean gen_last_on ;
  boolean data_written ;
  byte scd_evt, scd_cont ; /* SCD_xxx flags for event and continuous */
  word pre_event_buffers ; /* number of pre-event buffers */
  tfloat processed_stream ; /* output of this stream's FIR filter */
  longint records_generated_session ; /* count of buffers generated this connection */
  longword last_record_generated ; /* seconds since 2016 */
  longint detections_session ; /* number of detections during session */
  longint calibrations_session ; /* number of calibrations during session */
  longword gen_next ; /* general next to send */
  piirfilter stream_iir ; /* head of this channel's IIR filter chain */
  word mini_filter ; /* OMF_xxx bits */
  tarc arc ; /* archival miniseed structure */
} tlcq ;
typedef tlcq *plcq ;
/*
  tdet_packet defines the implementation of one detector on a LCQ. First part is not saved
  for continuity.
*/
typedef struct tdet_packet {
  struct tdet_packet *link ;
  pdetect detector_def ; /* definition of the detector */
  plcq parent ; /* the LCQ that owns this packet */
  byte det_options ; /* detector options */
  byte det_num ; /* ID for my copy of the detector */
  boolean singleflag ; /* true if data points are actually floating point */
  boolean remaining ; /* true if more samples to process in current rec */
  integer datapts ; /* Number of data points processed at a time */
  integer grpsize ; /* Samples per group, submultiple of datapts */
  integer sam_ch ;
  integer sam_no ; /*the number of the current seismic sample*/
  word insamps_size ; /* size of the tinsamps buffer, if any */
  word cont_size ; /* size of the continuity structure */
  double samrte ; /* Sample rate for this detector */
  pdataarray indatar ; /* ptr to data array */
  tinsamps *insamps ; /* ptr to low freq input buffer */
  pointer cont ; /* pointer to continuity structure */
  tonset_mh onset ; /* returned onset parameters */
  tdetload ucon ; /*user defined constants*/
} tdet_packet ;
typedef tdet_packet *pdet_packet ;

typedef struct tmsgqueue {
  struct tmsgqueue *link ;
  string250 msg ;
} tmsgqueue ;
typedef tmsgqueue *pmsgqueue ;

typedef plcq ttimdispatch[TIMF_SIZE] ; /* Timing Channels */
typedef plcq tsohdispatch[SOHF_SIZE] ; /* State of Health Channels */
typedef plcq tengdispatch[ENGF_SIZE] ; /* Engineering Channels */
typedef plcq tgpsdispatch[GPSF_SIZE] ; /* GPS Data Channels */
typedef plcq tmdispatch[TOTAL_CHANNELS][FREQS] ; /* main channels */

#endif
