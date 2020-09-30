/************************************************************************
 *  read_socket_request.c - utility functions for datasock program.
 *
 *  Douglas Neuhauser, UC Berkeley Seismological Laboratory.
 *  Copyright (c) 1995-2020 The Regents of the University of California.
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

#include "statchan.h"
#include "bld_statchans.h"
#include "read_socket_request.h"
#include "datasock_codes.h"

#define	ITIMEOUT    15
#define	MAXLINELEN  256

/************************************************************************
 *  read_socket_request:
 *	Generate station request list from input read from socket.
 *	Original format is single line:
 *	    station_list channel_list		(comma-delimited list)
 *	New format is multi-line:
 *	    100 PASSWORD password		(optional)
 *	    110 station_list channel_list	(comma-delmited lists)
 *	    ...
 *	    199 END_OF_REQUEST
 *  Return 0 on success, -1 on error.
 ************************************************************************/
int read_socket_request (int sd,	/* Input socket file descriptor.*/
			 STATCHAN **pws,/* ptr to statchan structure.	*/
			 int *pnws,	/* ptr to # STATCHAN entries.	*/
			 int flag,	/* request flag.		*/
			 char *passwd)	/* optional password to match.	*/
{
    int n;
    int code;
    FILE *fp;
    char str1[256], str2[256];
    int end_of_transaction = 0;
    char line[MAXLINELEN];
    int status = 0;
    int passwd_match = 0;
	
    if ((fp = fdopen (sd, "rw")) == NULL) {
	fprintf (stderr, "Error associating stream with socket\n");
	exit(1);
    }

    alarm (ITIMEOUT);

    if (fgets (line, MAXLINELEN, fp) == NULL) {
	fprintf (stderr, "Error reading station and channel info\n");
	exit(1);
    }
    n = sscanf (line, "%d", &code);
    if (n == 0 ) {
	/* Assume single-line format.					*/
	n = sscanf (line, "%s %s", str1, str2);
	if (n != 2) {
	    fprintf (stderr, "Error parsing station and channel info: %s\n",line);
	    exit(1);
	}
	status = bld_statchans (str1, str2, pws, pnws);
	if (status) {
	    fprintf (stderr, "Error processing station channel request: %s",
		     line);
	}

    }
    else {
	/* Assume multi-line format.					*/
	while (! end_of_transaction) {
	    n = sscanf (line, "%d %s %s", &code, str1, str2);
	    if (n >= 1) {
		switch (code) {
		case DATASOCK_PASSWD_CODE:
		    if (n != 3) {
			fprintf (stderr, "Error parsing line: %s", line);
			status = -1;
			break;
		    }
		    if (passwd && (strcmp(passwd,str2)==0)) passwd_match = 1;
		    break;
		case DATASOCK_CHANNEL_CODE:
		    if (n != 3) {
			fprintf (stderr, "Error parsing line: %s", line);
			status = -1;
			break;
		    }
		    /* Process channel request only if allowed by flag.	*/
		    if ((flag & SOCKET_REQUEST_CHANNELS)) {
			status = bld_statchans (str1, str2, pws, pnws);
		    }
		    else status = -1;
		    if (status) {
			fprintf (stderr, "Error processing station channel request: %s",
				 line);
		    }
		    break;
		case DATASOCK_EOT_CODE:
		    /* End of transaction.				*/
		    end_of_transaction = 1;
		    continue;
		    break;
		default:		/* Unknown code.		*/
		    fprintf (stderr, "Unimplemented code %d: %s",
			     code, line);
		    exit(1);
		    break;
		}
		if (status != 0) break;
	    }
	    if (fgets (line, MAXLINELEN, fp) == NULL) {
		fprintf (stderr, "Error reading station and channel info\n");
		exit(1);
	    }
	}
    }
    /* If password is required, ensure that we have a match.		*/
    if (status == 0 && ((flag & SOCKET_REQUEST_PASSWD) && ! passwd_match)) {
	fprintf (stderr, "Missing PASSWD in remote request\n");
	status = -1;
    }
    alarm(0);
    return (status);
}
