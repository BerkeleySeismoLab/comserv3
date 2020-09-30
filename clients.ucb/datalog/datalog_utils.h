#ifndef DATALOG_UTILS_H
#define DATAOG_UTILS_H

#include "qlib2.h"

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#include "datalog.h"

typedef struct _sel {		/* Structure for storing selectors.	*/
    int nselectors;		/* Number of selectors.			*/
    seltype *selectors;		/* Ptr to selector array.		*/
} SEL;

typedef struct _filter {
    int flag;			/* 0 = ignore all, 1 = use selector list*/
    SEL sel;			/* Ptr to selector list.		*/
} FILTER;

/************************************************************************
 *  datalog_utils function declarations.
 ************************************************************************/

void append_durlist (DURLIST *head, char *channel, char *duration);
void boolean_mask (short *ps, short bit_mask, char *str);
char *build_filename(FINFO *fip);
char *build_name(FINFO *fip, char *fmt);
char *channel_duration (DURLIST *head, char *channel);
int close_file(FINFO *fip);
char *datadir_path (FINFO *fip);
char *datafile_path (FINFO *fip);
char *date_string(INT_TIME time, char *dflt_str);
int decode_val (char *str, int len);
void dump_durlist (DURLIST *head);
void dump_data_hdr(DATA_HDR *hdr);
void dump_seed(unsigned char *str);
EXT_TIME file_time_limit(EXT_TIME file_time, char *duration);
FINFO *find_finfo (DATA_HDR *hdr);
int make_dir(char *dirname, int mode);
int open_file(FINFO *fip);
void parse_cfg (char *str1, char *str2);
int read_vol(FINFO *fip, DATA_HDR *hdr);
int sel_match (char *location, char *channel, SEL *psel);
int selector_match (char *str1, char *str2);
void set_selectors (pclient_struc me);
int start_new_file (FINFO *fip, EXT_TIME ext_begtime);
int store_bad_block (char* data);
int store_seed (seed_record_header *pseed);
void store_selectors(int type, SEL *psel, char *str);
void update_durlist (char *str, int nhead);
void verify_cfg();
int write_vol(FINFO *fip, int blksize);
int reduce_reclen (FINFO *fip, DATA_HDR *hdr, seed_record_header *pseed);
void gap_check (FINFO *fip, DATA_HDR *hdr);
int terminate_program (int error);
char *channelstring(char *station, char *network, char *channel, char *location);
int check_hdr_wordorder (DATA_HDR *hdr, SDR_HDR *pseed);
int fix_sncl (DATA_HDR *hdr, SDR_HDR *pseed);

#endif
