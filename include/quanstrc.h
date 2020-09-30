/*$Id: quanstrc.h,v 1.2 2005/06/16 20:17:26 isti Exp $*/
/* DA-DP Communications structures
   Copyright 1994-1996 Quanterra, Inc.
   Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 18 Mar 94 WHO Derived from dpstruc.pas
    1 30 May 94 WHO ultra_rev and comm_mask added to ultra_type. Typos
                    found by DSN corrected.
    2 25 Sep 94 WHO RECORD_HEADER_3 added.
    3 13 Dec 94 WHO cl_spec_type moved here from dpstruc.h.
    4 17 Jun 95 WHO link_adj_msg removed, link_adj_struc.la now is linkadj_com.
    5 13 Jun 96 WHO seedname and location added to clock_exception and
                    commo_comment.
    6  3 Dec 96 WHO Add BLOCKETTE.
    7 27 Jul 97 WHO Add FLOOD_PKT and FLOOD_CTRL.
    8 17 Oct 97 WHO Add VER_QUANSTRC.
    9 29 Sep 2020 DSN Updated for comserv3.
*/

#ifndef QUANSTRC_H
#define QUANSTRC_H

/* Make sure seedstrc.h is included, this forces including pascal.h 
   and dpstruc.h */
#ifndef cs_seedstrc_h
#include "seedstrc.h"
#endif

/* Flag this file as included */
#define cs_quanstrc_h
#define VER_QUANSTRC 9
 
/* Quanterra Record types */
#define EMPTY 0
#define RECORD_HEADER_1 1
#define RECORD_HEADER_2 2
#define CLOCK_CORRECTION 3
#define COMMENTS 4
#define DETECTION_RESULT 5
#define RECORD_HEADER_3 6
#define BLOCKETTE 7
#define FLOOD_PKT 8
#define END_OF_DETECTION 9
#define CALIBRATION 10
#define FLUSH 11
#define LINK_PKT 12
#define SEED_DATA 13
#define ULTRA_PKT 16
#define DET_AVAIL 17
#define CMD_ECHO 18
#define UPMAP 19
#define DOWNLOAD 20

/* Time structures */
typedef struct
{
    byte year ;
    byte month ;
    byte day ;
    byte hour ;
    byte min ;
    byte sec ;
} time_array ;

/* Quanterra data record header */
typedef struct
{
    int32_t header_flag ;                 /* flag word for header frame */
    byte frame_type ;                     /* standard frame offset for frame type */
    byte component ;                      /* not valid in QSL */
    byte stream ;                         /* not valid in QSL */
    byte soh ;                            /* state-of-health bits */
    complong station ;                    /* 4 byte station name */
    int16_t millisec ;                    /* time mark millisec binary value */
    int16_t time_mark ;                   /* sample number of time tag, usually 1 */
    int32_t samp_1 ;                      /* 32-bit first sample in record */
    int16_t clock_corr ;                  /* last clock correction offset in milliseconds */
    uint16_t number_of_samples ;          /* number of samples in record */
    signed char rate ;                    /* samp rate: + = samp/sec; - = sec/samp */
    byte blockette_count ;
    time_array time_of_sample ;           /* time of the tagged sample */
    int32_t packet_seq ;
    byte header_revision ;
    byte detection_day ;
    int16_t detection_seq ;
    /* Following fields are valid in QSL format only */
    int32_t clock_drift ;
    seed_name_type seedname ;
    signed char clkq ;
    seed_net_type seednet ;
    location_type location ;
    int16_t ht_sp3 ;
    int16_t microsec ;                    /* time mark microsec binary value */
    byte frame_count ;
    byte hdrsp1 ;
    byte z[5] ;
    byte ht_sp4 ;
} header_type ;

/* 512 byte Quanterra data frame */
typedef struct
{
    header_type h ;
    compressed_frame_array frames ;
} compressed_record ;

/* Event reporting structure */
typedef struct
{
    int32_t jdate ;                        /* seconds since Jan 1, 1984 00:00:00 */
    int16_t millisec ;                     /* 0..999 fractional part of time */
    byte component ;                       /* not valid in QSL */
    byte stream ;                          /* not valid in QSL */
    int32_t motion_pick_lookback_quality ; /* "string" of this gives, eg "CA200999" */
    int32_t peak_amplitude ;               /* & threshold sample value */
    int32_t period_x_100 ;                 /* 100 times detected period */
    int32_t background_amplitude ;         /* & threshold limit exceeded */
    /* Following fields are valid in QSL Format only */
    string23 detname ;                     /* detector name */
    seed_name_type seedname ;              /* seed name */
    byte sedr_sp1 ;                        /* zero */
    location_type location ;               /* seed location */
    int16_t ev_station ;                   /* station name NOT LONGWORD ALIGNED-COMPLONG */
    int16_t ev_station_low ;
    seed_net_type ev_network ;             /* seed network */
} squeezed_event_detection_report ;

/* time quality. The SUN ANSI C compiler incorrectly calculates the size of
   this structure as 8 bytes instead of 6. Where it is used it is substitued
   with an 6 byte array. */
typedef struct
{
    int32_t msec_correction ;          /* last timemark offset in milliseconds */
    byte reception_quality_indicator ; /* from Kinemetrics clock */
    byte time_base_VCO_correction ;    /* time base VCO control byte */
} time_quality_descriptor ;

/* calibration reporting types */
#define SINE_CAL 0
#define RANDOM_CAL 1
#define STEP_CAL 2
#define ABORT_CAL 3

/* Clock specific information */
typedef struct
{
    byte model ;        /* model type from above */
    byte cl_seq ;       /* 1-255 sequence number */
    byte cl_status[6] ; /* clock specific status */
} cl_spec_type ;

/* Due to flaws in the alignment rules on the SUN ANSI C compiler, the
   Pascal variant record definition cannot be directly translated into
   C structure/unions, they must be separated.
*/

/* Clock exception portion of eventlog_struc. Do to flaws in the SUN ANSI
   C compiler, time_quality_descriptor cannot be used (the compiler thinks
   it takes 8 bytes or requires long alignment) so a byte array is used
   in it's place. */
typedef struct
{
    byte frame_type ;
    byte clock_exception ;                 /* type of exception condition */
    time_array time_of_mark ;              /* if clock_exception = EXPECTED, VALID, DAILY, or UNEXPECTED */
    int32_t count_of ;                     /* seconds elapsed or consecutive marks */
    byte correction_quality[6] ;
    int16_t usec_correction ;              /* timemark offset, 0 - 999 microseconds*/
    int32_t clock_drift ;                  /* clock drift in microseconds */
    int16_t vco ;                          /* full VCO value */
    cl_spec_type cl_spec ;                 /* clock specific information */
    int16_t cl_station ;                   /* station name NOT LONG ALIGNED! */
    int16_t cl_station_low ;
    seed_net_type cl_net ;                 /* network */
    location_type cl_location ;            /* location */
    seed_name_type cl_seedname ;           /* seed name */
} clock_exception ;

/* Detection result portion of eventlog_struc */
typedef struct
{
    byte frame_type ;                      /* type of exception condition */
    byte detection_type ;
    time_array time_of_report ;            /* time that detection is reported */
    squeezed_event_detection_report pick ;
} detection_result ;

/* Calibration result portion of eventlog_struc. Do to flaws in the SUN ANSI
   C compiler, the "calibration_report_struc" had to be merged in here. Code
   will need to be adjusted accordingly. */
typedef struct
{
    byte frame_type ;
    byte word_align ;
    int16_t cr_duration ;              /* duration in seconds NOT LONG ALIGNED -LONG*/
    int16_t cr_duration_low ;
    int16_t cr_period ;                /* period in milliseconds NOT LONG ALIGNED-LONG */
    int16_t cr_period_low ;
    int16_t cr_amplitude ;             /* amplitude in DB */
    time_array cr_time ;               /* time of signal on or abort */
    byte cr_type ;
    byte cr_component ;                /* not valid in QSL */
    byte cr_stream ;                   /* not valid in QSL */
    byte cr_input_comp ;               /* not valid in QSL */
    byte cr_input_strm ;               /* not valid in QSL */
    byte cr_stepnum ;                  /* number of steps? */
    byte cr_flags ;                    /* bit 0 = plus, bit 2 = automatic, bit 4 = p-p */
/* Following are valid in QSL format only */
    byte cr_flags2 ;                   /* bit 0 = cap, bit 1 = white noise */
    int16_t cr_0dB ;                   /* 0 = dB, <> 0 = value for 0dB NOT LONG ALIGNED-FLOAT */
    int16_t cr_0dB_low ;
    seed_name_type cr_seedname ;       /* seed name */
    byte cr_sfrq ;                     /* calibration frequency if sine */
    location_type cr_location ;        /* seed location */
    seed_name_type cr_input_seedname ; /* seed name of cal input */
    byte cr_filt ;                     /* filter number */
    location_type cr_input_location ;  /* location of cal input */
    int16_t cr_station ;               /* station name NOT LONG ALIGNED-COMPLONG */
    int16_t cr_station_low ;
    seed_net_type cr_network ;         /* network */
} calibration_result ;

/* The full eventlog_struc as a union */
typedef union
{
    clock_exception clk_exc ;
    detection_result det_res ;
    calibration_result cal_res ;
} eventlog_struc ;
  
#define DEFAULT_WINDOW_SIZE 8  /* sliding comm window default size */
#define LEADIN_ACKNAK 'Z'      /* lead-in char for ascii ack/nak packet */
#define LEADIN_CMD 'X'         /* lead-in char for ascii cmd packet */

/* Command types from DP to DA */
#define ACK_MSG 48
#define SHELL_CMD 50
#define NO_CMD 51
#define START_CAL 54
#define STOP_CAL 55
#define MANUAL_CORRECT_DAC 60
#define AUTO_DAC_CORRECTION 61
#define ACCURATE_DAC_CORRECTION 68
#define MASS_RECENTERING 69
#define COMM_EVENT 70
#define LINK_REQ 71
#define LINK_ADJ 72
#define DOWN_ABT 73
#define FLOOD_CTRL 74
#define ULTRA_REQ 77
#define ULTRA_MASS 78
#define ULTRA_CAL 79
#define ULTRA_STOP 80
#define DET_REQ 81
#define DET_ENABLE 82
#define DET_CHANGE 83
#define REC_ENABLE 84
#define DOWN_REQ 85
#define UPLOAD 86

/* Sequence control types */
#define SEQUENCE_INCREMENT 0
#define SEQUENCE_RESET 1
#define SEQUENCE_SPECIAL 2

/* Misc. maximum lengths */
#define DP_TO_DA_MESSAGE_LENGTH 100  /* size on bytes of DP_to_DA message */
#define MAX_REPLY_BYTE_COUNT 444

/* there is only one kind of comment format */
#define DYNAMIC_LENGTH_STRING = 0

/* comments are pascal strings, not C strings */
typedef char comment_string_type[COMMENT_STRING_LENGTH+1] ;

/* Event records */
typedef struct
{
    int32_t header_flag ;
    eventlog_struc header_elog ;
} commo_event ;

/* Comment records */
typedef struct
{
    int32_t header_flag ;
    byte frame_type ;
    byte comment_type ;
    time_array time_of_transmission ;
    comment_string_type ct ;
    byte cc_pad ;
/*
  In QSL Format the following fields are moved up past the end
  of the comment string at transmission, and moved back to their
  proper locations at reception. This is to improve throughput.
  They are not valid in any other format.
*/
    int16_t cc_station ; /* NOT LONG ALIGNED-COMPLONG */
    int16_t cc_station_low ;
    seed_net_type cc_net ;
    location_type cc_location ;
    seed_name_type cc_seedname ;
} commo_comment ;

/* Reply to DP Record */
typedef struct
{
    int32_t header_flag ;
    byte frame_type ;
    boolean first_seg ;
    uint16_t total_bytes ;
    int16_t total_seg ;
    int16_t this_seg ;
    uint16_t byte_offset ;
    uint16_t byte_count ;
    byte bytes[MAX_REPLY_BYTE_COUNT] ;
} commo_reply ;

/* Link information Record */
typedef struct
{
    int32_t header_flag ;
    byte frame_type ;
    boolean rcecho ;
    int16_t seq_modulus ;
    int16_t window_size ;
    byte total_prio ;
    byte msg_prio ;
    byte det_prio ;
    byte time_prio ;
    byte cal_prio ;
    byte link_format ;
    int16_t resendtime ;  /* resend packets timeout in seconds */
    int16_t synctime ;    /* sync packet time in seconds */
    int16_t resendpkts ;  /* packets in resend blocks */
    int16_t netdelay ;    /* network restart delay in seconds */
    int16_t nettime ;     /* network connect timeout in seconds */
    int16_t netmax ;      /* unacked network packets before timeout */
    int16_t groupsize ;   /* group packet count */
    int16_t grouptime ;   /* group timeout in seconds */
} commo_link ;

typedef byte seg_map_type[128] ;
typedef char string63[64] ;

/* Upload map sent back to DP */
typedef struct
{
    int32_t header_flag ;
    byte frame_type ;
    boolean upload_ok ;
    string63 fname ;
    seg_map_type segmap ;
} commo_upmap ;

/* Record from DA to DP is a union of the above */
typedef union
{
    compressed_record cr ;
    commo_event ce ;
    commo_comment cc ;
    commo_reply cy ;
    commo_link cl ;
    commo_upmap cu ;
} commo_record ;

/* This isn't incorporated into the buffers, since the SUN ANSI C compiler
   calculates the size of this structure as 8 bytes instead of 6
*/
typedef struct
{
    int16_t chksum ;   /* chksum not calc over error pkt */
    int16_t crc ;      /* first gen crc, then make checksum NOT LONG ALIGNED-LONG */
    int16_t crc_low ;
} error_control_packet ;

/* The record is packaged with a sequence number, a control field, the actual data, and error control */
typedef struct
{
    int16_t skipped ;        /* START FILLING BUFFER AT "seq", THIS ASSURES THAT
                                THE DATA BUFFER IS LONG ALIGNED */
    byte seq ;
    byte ctrl ;              /* what to do with "Seq" in DP */
    commo_record data_buf ;  /* Compressed seismic data buffer */
    int16_t chksum ;         /* chksum not calc over error pkt */
    int16_t crc ;            /* first gen crc, then make checksum NOT LONG ALIGNED-LONG */
    int16_t crc_low ;
} DA_to_DP_buffer ;

/* Upload control values */
#define CREATE_UPLOAD 0
#define SEND_UPLOAD 1
#define ABORT_UPLOAD 2
#define UPLOAD_DONE 3
#define MAP_ONLY 4

/* Download control structure */
typedef struct
{
    boolean filefound ;
    boolean toobig ;
    uint16_t file_size ;
    string63 file_name ;
    byte contents[65000] ;
} download_struc ;

/* Messages from DP to DA. While it might be possible to duplicate the
   elegant structure definition in the Pascal version, the C equivalent
   would be a nightmare, therefor things are declared as individual
   structures with duplicated header fields and then merged into a
   group later.
*/
/* The infamous stokely structure */
typedef struct
{
    byte param0 ;
    byte dummy2 ;
    int16_t param1 ;
    int16_t param2 ;
    int16_t param3 ;
    int16_t param4 ;
    int16_t param5 ;
    int16_t param6 ;
    int16_t param7 ;
} stokely ;
  
typedef struct /* AUTO_DAC_CORRECTION, START_CAL, ACCURATE_DAC_CORRECTION */
{
    byte cmd_type ;
    byte dp_seq ;
    stokely cmd_parms ;
} mm_commands ;

typedef struct /* COMM_EVENT */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t rc_sp1 ;
    int32_t mask ;
} comm_event_struc ;

typedef struct /* ULTRA_MASS */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t mbrd ;
    int16_t mdur ;
} ultra_mass_struc ;

typedef struct /* ULTRA_CAL */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t rc_sp2 ;
    cal_start_com xc ;
} ultra_cal_struc ;
  
typedef struct /* ULTRA_STOP */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t sbrd ;
} ultra_stop_struc ;

typedef struct /* DET_REQ */
{
    byte cmd_type ;
    byte dp_seq ;
    seed_name_type dr_name ;
    byte rc_sp3 ;
    location_type dr_loc ;
} det_req_struc ;

typedef struct /* DET_ENABLE */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t rc_sp4 ;
    det_enable_com de ;
} det_enable_struc ;

typedef struct /* DET_CHANGE */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t rc_sp5 ;
    det_change_com dc ;
} det_change_struc ;

typedef struct /* REC_ENABLE */
{
    byte cmd_type ;
    byte dp_seq ;
    int16_t rc_sp6 ;
    rec_enable_com re ;
} rec_enable_struc ;

typedef struct /* SHELL_CMD */
{
    byte cmd_type ;
    byte dp_seq ;
    string79 sc ;
} shell_cmd_struc ;

typedef struct /* LINK_ADJ */
{
    byte cmd_type ;
    byte dp_seq ;
    linkadj_com la ;
} link_adj_struc ;

typedef struct /* DOWN_REQ */
{
    byte cmd_type ;
    byte dp_seq ;
    string63 fname ;
} down_req_struc ;

typedef struct /* UPLOAD */
{
    byte cmd_type ;
    byte dp_seq ;
    boolean return_map ;
    byte upload_control ;
    union
    {
        struct
	{ /* Send upload */
            uint16_t byte_offset ;
            uint16_t byte_count ;
            int16_t seg_num ;
            byte bytes[DP_TO_DA_MESSAGE_LENGTH - 10] ;
	} up_send ;
	struct
	{ /* Create upload */
	    uint16_t file_size ;
	    string63 file_name ;
	} up_create ;
    } up_union ;
} upload_struc ;
  
typedef struct /* FLOOD CONTROL */
{
    byte cmd_type ;
    byte dp_seq ;
    boolean flood_on_off ;
} flood_struc ;

/* All the possible messages as a union */
typedef union
{
    byte x[DP_TO_DA_MESSAGE_LENGTH] ;
    mm_commands mmc ;
    comm_event_struc ces ;
    ultra_mass_struc ums ;
    ultra_cal_struc ucs ;
    ultra_stop_struc uss ;
    det_req_struc drs ;
    det_enable_struc des ;
    det_change_struc dcs ;
    rec_enable_struc res ;
    shell_cmd_struc scs ;
    link_adj_struc las ;
    down_req_struc downs ;
    upload_struc us ;
    flood_struc fcs ;
} DP_to_DA_msg_type ;

/* DP to DA command header structure */
typedef struct
{
    int16_t chksum ;               /* chksum not calc over error pkt */
    int16_t crc ;                  /* first gen crc, then make checksum NOT LONG ALIGNED-LONG */
    int16_t crc_low ;
    byte cmd ;                     /* Ack, Nak, Cal, Shell indicator */
    byte ack ;                     /* Nr of last packet rcv'd by DP */
} DP_to_DA_command_header ;

/* DP to DA buffer structure */
typedef struct
{  /*for sending DP -> DA cmds*/
    DP_to_DA_command_header c ;     /* minimum required for ack/nak */
    DP_to_DA_msg_type msg ;         /* CAL, SHELL, etc. command params */
} DP_to_DA_buffer ;

#define DP_TO_DA_LENGTH_ACKNAK sizeof(DP_to_DA_command_header)
#define DP_TO_DA_LENGTH_CMD sizeof (DP_to_DA_buffer)

/* Ultra packet type (full version) */
typedef struct
{
    digi_record digi ;
    int16_t vcovalue ; /* current vco value */
    boolean pllon ; /* true if PLL controlling PLL */
    boolean umass_ok ; /* coyp of cal.mass_ok */
    int16_t comcount ; /* number of comm events */
    int16_t comoffset ; /* offset to start of comm names */
    int16_t usedcount ; /* number of used combos */
    int16_t usedoffset ; /* offset to start of used combinations */
    int16_t calcount ; /* copy of cal.number */
    int16_t caloffset ; /* offset to start of calibrator defs */
    byte cpr_levels ; /* comlink priority levels */
    byte ultra_rev ; /* revision level, current = 1 */
    int16_t ut_sp2 ;
    int32_t comm_mask ; /* current comm detector mask */
} ultra_type ;

#endif
