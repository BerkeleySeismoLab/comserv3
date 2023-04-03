



/*   Lib660 Seed Definitions
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
    0 2017-06-11 rdr Created
    1 2021-12-24 rdr Copyright assignment to Kinemetrics.
------2022-02-24 jms remove pseudo-pascal macros------
*/
#ifndef libseed_h
/* Flag this file as included */
#define libseed_h
#define VER_LIBSEED 1

#include "libtypes.h"

/* NOTE: Don't use sizeof for these values, we care about the actual SEED spec, not host sizes */
#define MAXSAMPPERWORD 7 /* maximum samples per 32-bit word */

#define WORDS_PER_FRAME 16 /* flag word and 15 actual entries */
#define FRAME_SIZE (WORDS_PER_FRAME * 4) /* 64 bytes per frame */
#define FRAMES_PER_RECORD 8 /* 512 byte base record */
#define LIB_REC_SIZE (FRAME_SIZE * FRAMES_PER_RECORD) /* normal miniseed is 512 byte record */
#define MAX_FRAMES_PER_RECORD 256 /* 16384 record */
#define MAX_REC_SIZE (FRAME_SIZE * MAX_FRAMES_PER_RECORD) /* largest that archive can handle */
#define DATA_OVERHEAD 64 /* data starts at second frame */
#define DATA_AREA (LIB_REC_SIZE - DATA_OVERHEAD) /* data area in normal miniseed */
#define NONDATA_OVERHEAD 56 /* can't use these bytes in record */
#define NONDATA_AREA (LIB_REC_SIZE - NONDATA_OVERHEAD) /* blockette/message area in normal miniseed */
#define OPAQUE_HDR_SIZE 20 /* for all 5 characters */
#define TIMING_BLOCKETTE_SIZE 200
#define RECORD_EXP 9
#define DEB_LOWV 8 /* In low range */
#define DEB_EVENT_ONLY 0x80 /* event only channel */
/* Seed activity Flags */
#define SAF_BEGIN_EVENT 0x4
#define SAF_END_EVENT 0x8
#define SAF_LEAP_FORW 0x10
#define SAF_LEAP_REV 0x20
#define SAF_EVENT_IN_PROGRESS 0x40
/* Seed quality flags */
#define SQF_AMPERR 0x1
#define SQF_MISSING_DATA 0x10
#define SQF_CHARGING 0x40
/* Seed flags also used by one second data */
#define SAF_CAL_IN_PROGRESS 0x1
#define SQF_QUESTIONABLE_TIMETAG 0x80
#define SIF_LOCKED 0x20

/* NOTE: The original paxcal definitions were 1 .. n instead of 0 .. n-1 */
typedef char tseed_name[3] ;
typedef char tseed_net[2] ;
typedef char tseed_stn[5] ;
typedef char tlocation[2] ;

typedef U32 compressed_frame[WORDS_PER_FRAME] ; /* 1 frame = 64 bytes */
typedef compressed_frame compressed_frame_array[FRAMES_PER_RECORD] ;
typedef compressed_frame max_compressed_frame_array[MAX_FRAMES_PER_RECORD] ;
typedef max_compressed_frame_array *pmax_cfr ;
typedef U8 completed_record[LIB_REC_SIZE] ; /* ready for storage */
typedef completed_record *pcompleted_record ;
typedef struct   /* minimum required fields for all blockettes */
{
    U16 blockette_type ;
    U16 next_blockette ; /*offset to next blockette*/
} blk_min ;
typedef struct   /* Data Only Blockette */
{
    U16 blockette_type ;
    U16 next_blockette ; /*offset to next blockette*/
    U8 encoding_format ; /*11 = Steim2, 0 = none*/
    U8 word_order ; /*1 always*/
    U8 rec_length ; /*8=256, 9=512, 12=4096*/
    U8 dob_reserved ; /*0 always*/
} data_only_blockette ;
typedef struct   /* Data Extension Blockette */
{
    U16 blockette_type ;
    U16 next_blockette ; /*offset to next blockette*/
    U8 qual ; /*0 to 100 quality indicator*/
    U8 usec99 ; /*0 - 99 microseconds*/
    U8 deb_flags ; /* DEB_XXXX */
    U8 frame_count ; /* number of 64 byte data frames */
} data_extension_blockette ;
typedef struct
{
    union {
        struct
        {
            U16 yr ;
            U16 jday ;
            U8 hr ;
            U8 minute ;
            U8 seconds ;
            U8 unused ;
            U16 tenth_millisec ;
        } STGREG ;
        double fpt ;
    } STUNION ;
} tseed_time ;
#define seed_yr STUNION.STGREG.yr
#define seed_jday STUNION.STGREG.jday
#define seed_hr STUNION.STGREG.hr
#define seed_minute STUNION.STGREG.minute
#define seed_seconds STUNION.STGREG.seconds
#define seed_unused STUNION.STGREG.unused
#define seed_tenth_millisec STUNION.STGREG.tenth_millisec
#define seed_fpt STUNION.fpt

typedef struct
{
    union {
        char ch[6] ;
        U32 num ;
    } SSUNION ;
} tseed_sequence ;
#define seed_ch SSUNION.ch
#define seed_num SSUNION.num

typedef struct
{
    tseed_sequence sequence ;                   /* record number */
    char seed_record_type ;                     /* D for data record */
    char continuation_record ;                  /* space normally */
    tseed_stn station_id_call_letters ;         /* last char must be space */
    tlocation location_id ;                     /* non aligned! */
    tseed_name channel_id ;                     /* non aligned! */
    tseed_net seednet ;                         /* space filled */
    tseed_time starting_time ;
    U16 samples_in_record ;
    I16 sample_rate_factor ;
    I16 sample_rate_multiplier ;              /* always 1 */
    U8 activity_flags ;                       /* ?I?LEBTC */
    U8 io_flags ;                             /* ??L????? */
    U8 data_quality_flags ;                   /* ???G???? */
    U8 number_of_following_blockettes ;       /* normally 2 for data */
    I32 tenth_msec_correction ;             /* always 0 */
    U16 first_data_byte ;                      /* 0, 56, or 64 - data starts in frame 1 */
    U16 first_blockette_byte ;                 /* 48 */
    data_only_blockette dob ;
    data_extension_blockette deb ;              /* used for data only */
} seed_header ;
typedef struct
{
    float signal_amplitude ;
    float signal_period ;
    float background_estimate ;
    U8 event_detection_flags ;                /* 0/1 for MH */
    U8 reserved_byte ;                        /* 0 */
    tseed_time signal_onset_time ;
} tonset_base ;
typedef struct
{
    float signal_amplitude ;
    float signal_period ;
    float background_estimate ;
    U8 event_detection_flags ;               /* 0/1 for MH */
    U8 reserved_byte ;                       /* 0 */
    tseed_time signal_onset_time ;
    U8 snr[6] ;                              /* only first 5 used */
    U8 lookback_value ;                      /* 0, 1, or 2 */
    U8 pick_algorithm ;                      /* 0 or 1 */
} tonset_mh ;
typedef struct              /* 201 */
{
    U16 blockette_type ;
    U16 next_blockette ;     /*offset to next blockette*/
    /* Extensions */
    tonset_mh mh_onset ;
    char s_detname[24] ;
} murdock_detect ;
typedef struct              /* 200 */
{
    U16 blockette_type ;
    U16 next_blockette ;
    tonset_base thr_onset ;
    /* Extensions */
    char s_detname[24] ;
} threshold_detect ;
typedef struct                            /* 500 */
{
    U16 blockette_type ;
    U16 next_blockette ;
    float vco_correction ;                 /* 0 to 100% of control value */
    tseed_time time_of_exception ;
    U8 usec99 ;                           /* -50 to +99 usec correction */
    U8 reception_quality ;                /* 0 to 100% */
    I32 exception_count ;               /* exception specific count */
    char exception_type[16] ;               /* description of exception */
    char clock_model[32] ;                  /* type of clock */
    char clock_status[128] ;                /* condition of clock */
} timing ;
typedef struct
{
    float calibration_amplitude ;          /* volts or amps, depending on LCQ */
    tseed_name calibration_input_channel ;  /* monitor channel, if any */
    U8 cal2_res ;                         /* zero */
    float ref_amp ;                        /* reference amplitude */
    char coupling[12] ;                     /* coupling method to seismo */
    char rolloff[12] ;                      /* type of filtering used */
} cal2 ;
typedef cal2 *pcal2 ;
typedef struct
{
    U16 blockette_type ;
    U16 next_blockette ;
    tseed_time calibration_time ;              /* start or stop */
    U8 number_of_steps ;                     /* 1 */
    U8 calibration_flags ;                   /* bit 0 = +, bit 2 = automatic */
    U32 calibration_duration ;            /* 0.0001 seconds / count */
    U32 interval_duration ;               /* 0 */
    cal2 step2 ;
} step_calibration ;
typedef struct
{
    U16 blockette_type ;
    U16 next_blockette ;
    tseed_time calibration_time ;              /* start or stop */
    U8 res ;                                 /* 0 */
    U8 calibration_flags ;                   /* bit 2 = automatic, bit 4 set */
    U32 calibration_duration ;            /* 0.0001 seconds / count */
    float sine_period ;                       /* in seconds */
    cal2 sine2 ;
} sine_calibration ;
typedef struct
{
    U16 blockette_type ;
    U16 next_blockette ;
    tseed_time calibration_time ;              /* start or stop */
    U8 res ;                                 /* 0 */
    U8 calibration_flags ;                   /* bit 2 = automatic, bit 4 = ? */
    U32 calibration_duration ;            /* 0.0001 seconds / count*/
    cal2 random2 ;
    char noise_type[8] ;                       /* frequency distribution */
} random_calibration ;
typedef struct
{
    U16 blockette_type ;
    U16 next_blockette ;
    tseed_time calibration_time ;              /* start or stop */
    U16 res ;                                 /* 0 */
} abort_calibration ;
typedef struct                               /* first part of every opaque blockette */
{
    U16 blockette_type ;
    U16 next_blockette ;
    U16 blk_lth ;                             /* Total blockette length in bytes */
    U16 data_off ;                            /* Byte offset to start of actual data */
    U32 recnum ;                          /* Record number */
    U8 word_order ;                          /* 1 = big endian */
    U8 opaque_flags ;                        /* orientation, packaging, etc. */
    U8 hdr_fields ;                          /* number of header fields */
    char rec_type[5] ;                         /* 4 character plus ~ terminator (most use only two characters) */
} topaque_hdr ;

extern pchar seed2string(tlocation loc, tseed_name sn, pchar result) ;
extern void string2fixed(pointer p, pchar s) ;
extern void fix_seed_header (seed_header *hdr, tsystemtime *greg,
                             I32 usec, BOOLEAN setdeb) ;
extern void lib660_seed_time (tseed_time *st, tsystemtime *greg, I32 usec) ;
extern void storebyte (PU8 *p, U8 b) ;
extern void storeword (PU8 *p, U16 w) ;
extern void storeint16 (PU8 *p, I16 i) ;
extern void storelongword (PU8 *p, U32 lw) ;
extern void storelongint (PU8 *p, I32 li) ;
extern void storesingle (PU8 *p, float s) ;
extern void storeblock (PU8 *p, int size, pointer psrc) ;
extern void storeseedhdr (PU8 *pdest, seed_header *hdr, BOOLEAN hasdeb) ;
/*extern void storemurdock (pbyte *pdest, murdock_detect *mdet) ; */
extern void storethreshold (PU8 *pdest, threshold_detect *tdet) ;
extern void storetiming (PU8 *pdest, timing *tim) ;
extern void storestep (PU8 *pdest, step_calibration *stepcal) ;
extern void storesine (PU8 *pdest, sine_calibration *sinecal) ;
extern void storerandom (PU8 *pdest, random_calibration *randcal) ;
extern void storeabort (PU8 *pdest, abort_calibration *abortcal) ;
extern void storeopaque (PU8 *pdest, topaque_hdr *ophdr,
                         pointer pbuf, int psize) ;
extern void storeframe (PU8 *pdest, compressed_frame *cf) ;
extern void loadblkhdr (PU8 *p, blk_min *blk) ;
extern void loadtime (PU8 *p, tseed_time *seedtime) ;
extern void loadseedhdr (PU8 *psrc, seed_header *hdr, BOOLEAN hasdeb) ;
extern void convert_time (double fp, tsystemtime *greg, I32 *usec) ;
extern double extract_time (tseed_time *st, U8 usec) ;
extern void loadtiming (PU8 *psrc, timing *tim) ;
extern void loadmurdock (PU8 *psrc, murdock_detect *mdet) ;
extern void loadthreshold (PU8 *psrc, threshold_detect *tdet) ;
extern void loadstep (PU8 *psrc, step_calibration *stepcal) ;
extern void loadsine (PU8 *psrc, sine_calibration *sinecal) ;
extern void loadrandom (PU8 *psrc, random_calibration *randcal) ;
extern void loadabort (PU8 *psrc, abort_calibration *abortcal) ;
extern void loadopaquehdr (PU8 *psrc, topaque_hdr *ophdr) ;

#endif
