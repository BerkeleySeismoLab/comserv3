/************************************************************************
 *  set_selectors.c - utility functions for datasock program.
 *
 *  Douglas Neuhauser, UC Berkeley Seismological Laboratory.
 *  Copyright (c) 1995-2020 The Regents of the University of California.
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include "dpstruc.h"
#include "service.h"
#include "timeutil.h"
#include "stuff.h"

#include "set_selectors.h"

/************************************************************************/
/*  set_selectors:							*/
/*	Generate and set comsert selectors for the specified 		*/
/*	station and comserv packet type.				*/
/*  Return 0 on success, -1 on error.					*/
/************************************************************************/
int set_selectors (pclient_struc me,	/* ptr to comserv shared mem.	*/
		   int k,		/* station index in shared mem.	*/
		   int type,		/* type of selector being set.	*/
		   int first,		/* index of starting selector.	*/
		   char **channel,	/* list of channels.		*/
		   int nchannels)	/* # of channel (ie selectors)	*/
{
    int i;
    char selector[6];
    pclient_station this;
    pselarray psa;
    int nsel;

    if (k < 0) {
	fprintf (stderr, "Error - comserv station index %d\n", k);
	return (-1);
    }
    this = (pclient_station) ((long) me + me->offsets[k]);
    psa = (pselarray) ((long) me + this->seloffset);
    if (nchannels <= 0) {
	fprintf (stderr, "Error - no channels specified for station %s\n",
		 sname_str_cs(this->name));
	return (-1);
    }

    if (first+nchannels > this->maxsel) {
	fprintf (stderr, "Error: Require %d selectors, allocated %d\n",
		 first+nchannels, this->maxsel);
	return (-1);
    }

    /* Leave previously set channels selectors alone.			*/
    this->sels[type].first = first;

    /* Create and set the selectors for each channels in the list.	*/
    for (i=0,nsel=first; i<nchannels; i++,nsel++) {
	memset((void *)selector,0,sizeof(selector));
	strcpy(selector,lead(5,'?',channel[i]));
	memcpy ((void *)&((*psa)[nsel]), (void *)selector, sizeof(seltype));
    }
    this->sels[type].last = nsel-1;
    return (0);
}
