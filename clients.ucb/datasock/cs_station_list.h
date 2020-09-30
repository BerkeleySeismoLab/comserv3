/* $Id: cs_station_list.h,v 1.1.1.1 2001/11/20 02:09:53 redi Exp $	*/
int cs_station_list (STATCHAN **pcs,	/* ptr to comserv STATCHAN struc*/
		     int *pncs,		/* ptr to # comserv entries	*/
		     STATCHAN **pwc,	/* ptr to wildcard STATCHAN	*/
		     int *pnwc);	/* ptr to # wildcard entries.	*/
