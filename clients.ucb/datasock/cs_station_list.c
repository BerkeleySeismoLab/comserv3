/************************************************************************
 *  cs_station_list.c - utility functions for datasock program.
 *
 *  Douglas Neuhauser, UC Berkeley Seismological Laboratory.
 *  Copyright (c) 1995-2020 The Regents of the University of California.
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <search.h>
#include <fnmatch.h>

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#include "statchan.h"
#include "cs_station_list.h"

#ifndef	MAXHASH
#define	MAXHASH	100
#endif

char hash_key[MAXHASH][8];
char hash_data[MAXHASH][80];
int nhash = 0;

#define	LINELEN	256
char line[LINELEN];

#define	STATIONS_LIST	"/etc/stations.ini"

int add_channels (STATCHAN *cs, STATCHAN *wc);

/************************************************************************
 *  cs_station_list:
 *	Generate a comserv STATCHAN list based on the wildcard list.
 *	List is generated by comparing the comserv station list to
 *	each (possibly wildcarded) station and channel request list.
 *  Return 0 on success, -1 on error.
 ************************************************************************/
int cs_station_list (STATCHAN **pcs,	/* ptr to comserv statchan struc*/
		     int *pncs,		/* ptr to # comserv entries	*/
		     STATCHAN **pwc,	/* ptr to wildcard statchan	*/
		     int *pnwc)		/* ptr to # wildcard entries.	*/
{
    char str1[CFGWIDTH];
    char str2[CFGWIDTH];
    char this_station[CFGWIDTH];
    char site_net[CFGWIDTH*2+1];
    char *site = NULL;
    char *net = NULL;
    config_struc cfg;
    int i, j;
    STATCHAN *cs = *pcs;
    STATCHAN *wc = *pwc;
    int ncs = *pncs;
    int nwc = *pnwc;
    int status = 0;
    memset (&cfg, 0, sizeof(cfg));
    if (open_cfg (&cfg, STATIONS_LIST, "*") != 0) {
	close_cfg (&cfg);
	return (-1);	/* error */
    }

    do {
	/* Get the next station in the network config file.		*/
	strcpy (this_station, &cfg.lastread[1]);
	this_station[strlen(this_station)-1] = '\0';	/* remove [ ]	*/
	upshift (this_station);
	/* Look for a site name for this station that may be different	*/
	/* than the comserv station name.				*/
	while (1) {
	    read_cfg(&cfg, str1, str2);	/* Skip past section header.	*/
	    if (str1[0] == '\0') break;
	    if (strcmp(str1, "STATION") == 0 || strcmp(str1, "SITE") == 0) {
		site = strdup(str2);
	    }
	    if (strcmp(str1, "NETWORK") == 0 || strcmp(str1, "NET") == 0) {
		net = strdup(str2);
	    }
	}

        /* 2020-07-09.  Each station entry in the network config file	*/
	/* MUST have a site and net entry, and these are used to match	*/
	/* againt the requested station in the wildcard list.		*/
	if (site == NULL || net == NULL) {
	    free(site);
	    free(net);
	    return (-1);
	}
	sprintf (site_net, "%s.%s", site, net);

	/* Check site_net for a match with each entry on the input	*/
	/* station wildcard list.					*/
	/* If a match is found, add this_station and channels to the	*/
	/* desired list of stations and channels.			*/
	/* The wildcard station entry must be in station.net format.	*/
	for (i=0; i<nwc; i++) {
	    if (fnmatch (wc[i].station, site_net, 0) == 0) {
		/* Find this_station entry in cs list.			*/
		for (j=0; j<ncs; j++) {
		    if (strcmp(this_station, cs[j].station) == 0) break;
		}
		/* Add this_station to the cs list if not already there.*/
		if (j >= ncs) {
		    cs = (ncs == 0) ? 
			(STATCHAN *)malloc((ncs+1)*sizeof(STATCHAN)) :
			(STATCHAN *)realloc(cs,(ncs+1)*sizeof(STATCHAN));
		    if (cs == NULL) {
			fprintf (stderr, "Error allocating STATCHAN\n");
			return (-1);
		    }
		    strcpy (cs[ncs].station, this_station);
		    cs[ncs].nchannels = 0;
		    cs[ncs].channel = NULL;
		    j = ncs++;
		}
		/* Add the channels for this request to the list of	*/
		/* channels for this station.				*/
		status = add_channels (&cs[j],&wc[i]);
	    }
	    if (status != 0) break;
	}
	free (site);
	free (net);
	site = NULL;
	net = NULL;
	if (status != 0) break;
    } while (skipto (&cfg, "*") == 0);
    close_cfg (&cfg);

    *pncs = ncs;
    *pnwc = nwc;
    *pcs = cs;
    *pwc = wc;

    return (status);
}

/************************************************************************
 *  add_channels:
 *	Add channels from request entry wc to the generated entry cs.
 *	Do no add channel to cs if it already appears on cs.
 *  Returns 0 on success, -1 on error.
 ************************************************************************/
int add_channels (STATCHAN *cs, STATCHAN *wc)
{
    int i, j;
    int is_duplicate;
    int status = 0;

    /* Compare each request channel from r to channels on cs.		*/
    /* If there is no match, add channel from r to cs.			*/
    for (i=0; i<wc->nchannels; i++) {
	is_duplicate = 0;
	for (j=0; j<cs->nchannels; j++) {
	    if (strcasecmp(wc->channel[i],cs->channel[j]) != 0) continue;
	    is_duplicate = 1;
	    break;
	}
	if (is_duplicate) continue;
	/* Copy channel from r to g.					*/
	cs->channel = (cs->nchannels == 0) ? 
	    (char **)malloc((cs->nchannels+1)*sizeof(char *)) :
	    (char **)realloc(cs->channel,(cs->nchannels+1)*sizeof(char *));
	if (cs->channel == NULL) {
	    fprintf (stderr, "Error allocing channel info\n");
	    return (-1);
	}
	cs->channel[cs->nchannels] = strdup(wc->channel[i]);
	++(cs->nchannels);
    }
    return (status);
}
