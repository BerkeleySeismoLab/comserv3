/* $Id: channel_info.h,v 1.1.1.1 2001/11/20 02:09:54 redi Exp $	*/
/*  Channel and selector info.	*/
typedef struct _channel_info {
	char station[8];
	char channel[4];
	char network[4];
	char location[4];
} CHANNEL_INFO;
