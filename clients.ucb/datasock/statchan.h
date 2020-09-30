/* $Id: statchan.h,v 1.1.1.1 2001/11/20 02:09:53 redi Exp $	*/
/* Structure to hold station name and list of channels.			*/

#ifndef	__statchan_h
#define __statchan_h

typedef struct _statchan {	/* Structure for station channel info.	*/
	char station[SERVER_NAME_SIZE];	/* Station name.		*/
	int nchannels;		/* Number of channels.			*/
	char **channel;		/* Ptr to list of channel names.	*/
} STATCHAN;

#endif
