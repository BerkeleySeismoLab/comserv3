/* $Id: bld_statchans.h,v 1.1.1.1 2001/11/20 02:09:53 redi Exp $	*/
#include "statchan.h"

int bld_statchans (char *station_str,	/* station list string.		*/
		   char *channel_str,	/* channel list string.		*/
		   STATCHAN **pws,	/* ptr to STATCHAN structure.	*/
		   int *pnws);		/* ptr to # STATCHAN entries.	*/

int bld_statchan (char *station_str,	/* single station string.	*/
		  char *channel_str,	/* channel list string.		*/
		  STATCHAN **pws,	/* ptr to STATCHAN structure.	*/
		  int *pnws);		/* ptr to # STATCHAN entries.	*/
