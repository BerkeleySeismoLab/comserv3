/*	$Id: datalog.h,v 1.1.1.1 2004/06/15 19:08:00 isti Exp $	*/

#ifndef DATALOG_H
#define DATALOG_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#define info stderr

#define DAT_INDEX	DATAQ
#define DET_INDEX	DETQ
#define	CAL_INDEX	CALQ
#define	CLK_INDEX	TIMQ
#define	LOG_INDEX	MSGQ
#define	BLK_INDEX	BLKQ

#define	MAXLEN_X	32

typedef struct _durlist {
    char    channel[4];
    char    duration[8];
    struct _durlist *next;
} DURLIST;

typedef struct _finfo {
    char    station[6];
    char    channel[4];
    char    network[3];
    char    location[3];
    char    ch_dir[40];
    char    filename[40];
    int	    itype;
    int	    blksize;
    char    duration[8];
    EXT_TIME limit;
    EXT_TIME begtime;
    EXT_TIME endtime;
    struct _finfo *next;
    int	    fd;
} FINFO;

#endif
