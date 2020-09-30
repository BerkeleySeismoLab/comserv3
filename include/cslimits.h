/* Comserv System Limits */

#ifndef CSLIMITS_H
#define CSLIMITS_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

/* Originally in service.h */
#ifdef COMSERV2
#define CLIENT_NAME_SIZE		5
#define SERVER_NAME_SIZE		5
#else
#define CLIENT_NAME_SIZE		32
#define SERVER_NAME_SIZE		32
#endif

/* Originally in service.h */
#define MAXSTATIONS 512          /* maximum number of stations */
#define MAXCLIENTS 64           /* maximum number of clients */

/* Originally in server.h */
#define MAXUSERIDS 10

/* Originally in cfgutil.h */
#define CFGWIDTH 512
#define SECWIDTH 256

#endif
