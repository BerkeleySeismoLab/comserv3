/* $Id: set_selectors.h,v 1.1.1.1 2001/11/20 02:09:53 redi Exp $	*/
int set_selectors (pclient_struc me,	/* ptr to comserv shared mem.	*/
		   int k,		/* station index in shared mem.	*/
		   int type,		/* type of selector being set.	*/
		   int first,		/* index of starting selector.	*/
		   char **channel,	/* list of channels.		*/
		   int nchannels);	/* # of channel (ie selectors)	*/
