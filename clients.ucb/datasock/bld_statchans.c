/************************************************************************
 *  bld_statchans.c - utility functions for datasock program.
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
#include <stdlib.h>

#include <qlib2.h>

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#include "statchan.h"
#include "bld_statchans.h"

/************************************************************************
 *  bld_statchans:
 *	Add or augment a STATCHAN entry from a station_list and
 *	channel_list
 *	Inputs are:
 *	    station_list (comma-delimited station list).
 *	    channel_list (comma-delimited channel list).
 *	The station and channel may contain wildcard characters.
 *	Station may contain "*" or "?" wildcard characters.
 *	Channel may contain "?" wildcard characters.
 *	Generate a cross-product of stations and channels.
 *  Return 0 on success, -1 on error.
 ************************************************************************/
int bld_statchans (char *station_str,	/* station list string.		*/
		   char *channel_str,	/* channel list string.		*/
		   STATCHAN **pws,	/* ptr to statchan structure.	*/
		   int *pnws)		/* ptr to # statchan entries.	*/
{
    char *token, *p;
    char *str = NULL;
    char **stations;
    int nstations = 0;
    int status;
    int i;

    stations = (char **)malloc(sizeof(char *));
    p = str = strdup (station_str);
    if (str == NULL || stations == NULL) {
	free(stations);
	free(str);
	return (-1);
    }

    /* Break the station list into individual station tokens.		*/
    while ((token = strtok (p,","))) {
	stations = (char **)realloc(stations,(nstations+1)*sizeof(char *));
	stations[nstations++] = token;
	p = NULL;
    }

    /* Process the channel list for each station.			*/
    /* Add a network wildcard to the station if it does not contain a 	*/
    /* network component.						*/
    for (i=0; i<nstations; i++) {
	if (strchr(stations[i],'.') == NULL) {
	    /* Add a network wildcard component to the station.		*/
	    char *sta_net = malloc (strlen(stations[i]+3));
	    strcpy (sta_net, stations[i]);
	    strcat (sta_net, ".*");
	    status = bld_statchan (sta_net, channel_str, 
				   pws, pnws);
	    free(sta_net);
	}
	else {
	    status = bld_statchan (stations[i], channel_str, 
				   pws, pnws);
	}
	if (status != 0) break;
    }

    /* Free temp space and return status of channel parsing.		*/
    free(stations);
    free(str);
    return (status);
}

/************************************************************************
 *  bld_statchan
 *	Add or augment a STATCHAN entry from a station and channel_list.
 *	Inputs are:
 *	    station (or station.net)
 *	    channel_list (comma-delimited channel list).
 *	The station and channel may contain wildcard characters.
 *	Station may contain "*" or "?" wildcard characters.
 *      If station contains a ".", it is a station.net.
 *	A channel-location string will normally be "CCC.LL", that is a
 *	three-letter channel name followed by a 2-letter location code
 *	separated by a period.
 *	The single-letter wildcard '*' may be used in place of the
 *	three letter channel code or two-letter location code; this
 *	match all possible channel or location codes, respctively.
 *	The single-letter wildcard '?' may be used in any position of
 *	the channel or location strings.
 *	A channel-location string of "CCC" will be treated as "CCC.??"
 *	while the channel-location string of "CCC." will be treated as
 *	"CCC.  ", i.e. the empty location code.
 *  Returns 0 on success, -1 on error.
 ************************************************************************/
int bld_statchan (char *station_str,	/* single station string.	*/
		  char *channel_str,	/* channel list string.		*/
		  STATCHAN **pws,	/* ptr to statchan structure.	*/
		  int *pnws)		/* ptr to # statchan entries.	*/
{
    char *s;
    char *chan, *loc;
    const char *wildchan = "???";
    const char *wildloc = "??";
    const char *blankloc = "  ";
    const char *sep = ".,";
    int i, ic, len;
    STATCHAN *ws = *pws;
    int nws = *pnws;
    char *station = NULL;
    char *channel = NULL;

    /* Check to see if this is a duplicate station name.		*/
    station = strdup(station_str);
    channel = strdup(channel_str);
    if (station == NULL || channel == NULL) {
	fprintf (stderr, "Error mallocing station or channel strings\n");
	if (station) free(station);
	if (channel) free(channel);
	return (-1);
    }
    uppercase (station);
    uppercase (channel);
    for (i=0; i<nws; i++) {
	if (strcmp(station, ws[i].station) == 0) break;
    }
    /* If station was not found, allocate space for a new station.	*/
    if (i >= nws) {
	ws = (nws == 0) ? 
	    (STATCHAN *)malloc((nws+1)*sizeof(STATCHAN)) :
	    (STATCHAN *)realloc(ws,(nws+1)*sizeof(STATCHAN));
	if (ws == NULL) {
	    fprintf (stderr, "Error mallocing station info\n");
	    free(station);
	    free(channel);
	    return (-1);
	}
	memset((void *)&ws[i], 0, sizeof(STATCHAN));
	i = nws++;
	strcpy(ws[i].station, station);
    }
    /* Append the new channels to the station request.			*/
    s = channel;
    while ((s != NULL) && (ic = strcspn(s, sep)) != 0) {
	if ( *(s+ic) == ',' || *(s+ic) == '\0') {
	    /* channel without any location */
	    chan = s;
	    loc = (char*)wildloc;
	}
	else if ( *(s+ic) == '.' ) {
	    /* channel followed by location */
	    chan = s;
	    *(s+ic) = '\0';
	    s += ic + 1;
	    ic = strcspn(s,",");
	    if (ic == 0)
		loc = (char*)blankloc;
	    else {
		loc = s;
	    }
	}
	
	len = strlen(s);  /* how much is left */
	*(s+ic) = '\0';
	s += ic + ((ic < len) ? 1 : 0); /* don't advance s beyond the end */

	/* Validity checks: we don't check for outlandish characters */
	if (strcmp(chan, "*") == 0) {
	    chan = (char*)wildchan;
	}
	else if (strlen(chan) != 3) {
	    fprintf(stderr, "invalid channel name <%s>\n", chan);
	    return -1;
	}
	
	if (strcmp(loc, "*") == 0) {
	    loc = (char*)wildloc;
	}
	else if (strlen(loc) != 2) {
	    fprintf(stderr, "invalid location string <%s>\n", loc);
	    return -1;
	} else {
	    if (loc[0] == '-') loc[0] = ' ';
	    if (loc[1] == '-') loc[1] = ' ';
	}

	ws[i].channel = (ws[i].nchannels == 0) ? 
	    (char **)malloc((ws[i].nchannels+1)*sizeof(char *)) :
	    (char **)realloc(ws[i].channel,
			     (ws[i].nchannels+1)*sizeof(char *));
	ws[i].channel[ws[i].nchannels] = malloc(6);
	strcpy(ws[i].channel[ws[i].nchannels],loc);
	strcat(ws[i].channel[ws[i].nchannels],chan);
	++(ws[i].nchannels);
    }
    *pws = ws;
    *pnws = nws;
    free(station);
    free(channel);
    return (0);
}
