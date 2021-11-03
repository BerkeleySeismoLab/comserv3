/*   Server Config file parser
     Copyright 1994-1998 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 23 Mar 94 WHO Pulled out of server.c
    1 27 Mar 94 WHO Merged blockette ring changed to separate rings.
    2 16 Apr 94 WHO Setup fake ultra structure for Shear stations.
    3  6 Jun 94 WHO Add setting user privilege structure.
    4  9 Jun 94 WHO Cleanup to avoid warnings.
    5 11 Aug 94 WHO Add setting of ipport.
    6 20 Jun 95 WHO allow setting of netto, netdly, grpsize, and
                    grptime parameters.
    7  2 Oct 95 WHO Allow setting of sync parameter.
    8  8 Oct 95 WHO Allow setting of flow parameter.
    9 17 Nov 95 WHO sync is now called notify in config file.
   10 28 Jan 96 DSN Add LOG_SEED and TIMING_SEED directives.
   11 29 May 96 WHO Start of conversion to run on OS9.
   12 13 Jun 96 WHO Add setting of "SEEDIN" flag.
   13  3 Aug 96 WHO Add setting of "ANYSTATION" and "NOULTRA" flags.
   14  3 Dec 96 WHO Add setting of "BLKBUFS".  
   15  7 Dec 96 WHO Add setting of "UDPADDR".
   16 16 Oct 97 WHO Add "MSHEAR" as duplicate of "SEEDIN" flag. Add
                    VER_CSCFG
   17 22 Oct 97 WHO Add setting of verbosity flags based on vopttable.
   18 29 Nov 97 DSN Add optional LOCKFILE directive again.
   19  8 Jan 98 WHO Lockfile not for OS9 version.
   20 23 Dec 98 WHO Add setting of "LINKRETRY".
   21 22 Feb 99 PJM Modified this to support the multicast comserv
   22 01 Dec 05 PAF Added in LOGDIR directive for logging directory
   23 29 Sep 2020 DSN Updated for comserv3.

   This is based on the original cscfg.c.
   Changes include:
   a.  Deprecating config parameters used only by original comserv.
   b.  Setting variables in a structure rather than global variables.
       It will be the responsibity of the caller to set any external
       variables used by the historic comserv libraries from this structure.

*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#include "service.h"
#include "server.h"
#include "cfgutil.h"
#include "csconfig.h"

short VER_CSCFG = 23 ;

#define STATIONS_INI	"/etc/stations.ini"
#define STATION_INI	"station.ini"
#define NETWORK_INI 	"/etc/network.ini"
#ifdef COMSERV2
#define GLOBAL_DEFAULTS	"NETM"
#else
#define GLOBAL_DEFAULTS	"global_defaults"
#endif

/***********************************************************************
 * initialize_csconfig:
 *	Initialize a csconfig structure
 **********************************************************************/  

void initialize_csconfig (csconfig *cs_cfg)
{
    memset (cs_cfg, 0, sizeof(csconfig));
    return;
}

/***********************************************************************
 * verify_server_name:
 *	Return 0 if station_name found, non-zero otherwise.
 **********************************************************************/  

int verify_server_name (char *server_name)
{
    config_struc cfg ;
    int rc;
    memset (&cfg, 0, sizeof(cfg));
    rc = open_cfg(&cfg, STATIONS_INI, server_name);
    close_cfg(&cfg);
    return rc;
}

#ifdef COMSERV2
/***********************************************************************
 * getLogParamsFromNetwork
 *	Retrieve and return logging config parameters from NETWORK_INI.
 *	logdir:		LOGDIR config parameter.
 *	logfile:	LOGTYPE config parameter (LOGFILE or STDOUT)
 *   Return 0 on success, non-zero otherwise.    
 **********************************************************************/  

int getLogParamsFromNetwork(csconfig *cs_cfg)
{
    config_struc network_cfg;           /* structure for config file.   */
    char str1[SECWIDTH], str2[SECWIDTH];

    /* Scan network initialization file.                                */
    memset (&network_cfg, 0, sizeof(network_cfg));
    if (open_cfg(&network_cfg,NETWORK_INI,GLOBAL_DEFAULTS))
    {
        fprintf (stderr, "Warning: Could not find [%s] section in network file %s\n", 
		 GLOBAL_DEFAULTS, NETWORK_INI);
	return -1;
    }
    while (1)
    {
	read_cfg(&network_cfg,&str1[0],&str2[0]);
	if (str1[0] == '\0') 
	{
	    break;
	}
	/* Logging parameters.               */
	if (strcmp(str1,"LOGDIR")==0)
	{
	    strcpy(cs_cfg->logdir,str2);
	    continue;
	}
	if (strcmp(str1,"LOGTYPE")==0)
	{
	    strcpy(cs_cfg->logtype,str2);
	    continue;
	}
    }
    close_cfg(&network_cfg);
    return 0;
}
#endif

/***********************************************************************
 * getCSServerInfo
 *	Retrieve and return staion config parameters from STATIONS_INI.
 *	server_dir	Directory containing the STATION_INI file.
 *	server_desc	Stations description string.
 *	server_source	Station comserv section in STATION_INI file.
 *	seed_station	SEED station code.
 *	seed_network	SEED network code.
 *   Return 0 on success, non-zero otherwise.
 **********************************************************************/  

int getCSServerInfo(csconfig *cs_cfg, char *server_name)
{
    config_struc stations_cfg;           /* structure for config file.   */
    char str1[SECWIDTH], str2[SECWIDTH];

    /* Get selected station info from stations file. */
    memset (&stations_cfg, 0, sizeof(stations_cfg));
    if (open_cfg(&stations_cfg,STATIONS_INI,server_name))
    {
        fprintf (stderr, "Could not find [%s] section in file %s\n", 
		 server_name, STATIONS_INI);
        exit(1);
    }
    while (1)
    {
	read_cfg(&stations_cfg,&str1[0],&str2[0]);
	if (str1[0] == '\0') 
	{
	    break;
	}
	if (strcmp(str1,"DIR")==0)
	{
	    strcpy(cs_cfg->server_dir,str2);
	    continue;
	}
	if (strcmp(str1,"DESC")==0)
	{
	    strcpy(cs_cfg->server_desc,str2);
	    continue;
	}
	if (strcmp(str1,"SOURCE")==0)
	{
	    strcpy(cs_cfg->server_source,str2);
	    continue;
	}
	if (strcmp(str1,"STATION")==0 || strcmp(str1,"SITE")==0)
	{
	    strcpy(cs_cfg->seed_station,str2);
	    continue;
	}
	if (strcmp(str1,"NETWORK")==0 || strcmp(str1,"NET")==0)
	{
	    strcpy(cs_cfg->seed_network,str2);
	    continue;
	}
    }
    close_cfg(&stations_cfg);
    return 0;
}

/***********************************************************************
 * getCSServerParams
 *	Retrieve and return station comserv config parameters from STATION_INI.
 *    Return 0 on success, non-zero otherwise.
 **********************************************************************/  

int getCSServerParams (csconfig *cs_cfg, char *server_dir, char *section)
{
    config_struc station_cfg;           /* structure for config file.   */
    char str1[CFGWIDTH] ;
    char str2[CFGWIDTH] ;
    char stemp[CFGWIDTH] ;
    char filename[CFGWIDTH];
    pchar tmp ;
    short i ;

    sprintf (filename, "%s/%s", server_dir, STATION_INI);
    /* Get selected station info from station file. */
    memset (&station_cfg, 0, sizeof(station_cfg));
    if (open_cfg(&station_cfg,filename,section))
    {
        fprintf (stderr, "Could not find [%s] section in file %s\n", 
		 section, filename);
        exit(1);
    }
    do
    {
	read_cfg(&station_cfg, str1, str2) ;
	if (str1[0] == '\0') 
	{
	    break ;
	}
#ifdef COMSERV2
	if (strcmp(str1, "LOCKFILE") == 0)
	{
	    strcpy(cs_cfg->lockfile, str2) ;
	    continue;
	}	   
	if (strcmp(str1, "IPPORT") == 0)
	{
	    strcpy(cs_cfg->ipport, str2) ;
	    continue;
	}
	if (strcmp(str1,"LOGDIR")==0)
	{
	    strcpy(cs_cfg->logdir,str2);
	    continue;
	}
	if (strcmp(str1,"LOGTYPE")==0)
	{
	    strcpy(cs_cfg->logtype,str2);
	    continue;
	}
	if (strcmp(str1, "UDPADDR") == 0)
	{
	    strcpy(cs_cfg->udpaddr, str2) ;
	    continue;
	}
	if (strcmp(str1, "MCASTIF") == 0)
	{
	    strcpy(cs_cfg->mcastif, str2) ;
	    continue;
	}
#endif
	if (strcmp(str1, "VERBOSITY") == 0)
	{
	    cs_cfg->verbose = (atoi(str2)) ;
	    continue;
	}
	if (strcmp(str1, "OVERRIDE") == 0)
	{
	    cs_cfg->override = ((str2[0] == 'y') || (str2[0] == 'Y')) ;
	    continue;
	}
	if (strcmp(str1, "STATION") == 0)
	{
	    str2[SERVER_NAME_SIZE-1] = '\0';
	    strcpy(cs_cfg->station,str2);
	    continue;
	}
	if (strcmp(str1, "LOG_SEED") == 0)
	{
	    upshift(str2) ;
	    tmp = strchr(str2, '-') ;
	    if (tmp)
	    {
		*tmp++ = '\0' ;
		strncpy(cs_cfg->log_location_id, str2, 2);
	    }
	    else
		tmp = str2;
	    tmp[3] = '\0';
	    strncpy(cs_cfg->log_channel_id, tmp, 3);
	    continue;
	}
	if (strcmp(str1, "TIMING_SEED") == 0)
	{
	    upshift(str2) ;
	    tmp = strchr(str2, '-') ;
	    if (tmp)
	    {
		*tmp++ = '\0' ;
		strncpy(cs_cfg->timing_location_id, str2, 2);
	    }
	    else
		tmp = str2;
	    tmp[3] = '\0';
	    strncpy(cs_cfg->timing_channel_id, tmp, 3);
	    continue;
	}
	if (strcmp(str1, "SEGID") == 0)
	{
	    cs_cfg->segid = atoi((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "POLLUSECS") == 0 || strcmp(str1, "POLLUSEC") == 0)
	{
	    cs_cfg->pollusecs = atol((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "DATABUFS") == 0)
	{
	    cs_cfg->databufs = atoi((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "DETBUFS") == 0)
	{
	    cs_cfg->detbufs = atoi((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "CALBUFS") == 0)
	{
	    cs_cfg->calbufs = atoi((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "TIMBUFS") == 0)
	{
	    cs_cfg->timbufs = atoi((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "MSGBUFS") == 0)
	{
	    cs_cfg->msgbufs = atoi((pchar)&str2) ;
	    continue;
	}
	if (strcmp(str1, "BLKBUFS") == 0)
	{
	    cs_cfg->blkbufs = atoi((pchar)&str2) ;
	    continue;
	}
	/* Look for compound keyword directives. */
	strcpy(stemp, str1) ;
	/* look for client[xx]=name[,timeout] */
	if (strncmp(stemp, "CLIENT", 6) == 0)
	{
	    i = cs_cfg->n_clients++ ;
	    /* get client name */
	    upshift(str2) ;
	    comserv_split (str2, stemp, ',') ;
	    strcpy (cs_cfg->clients[i].client_name, str2);
	    /* get timeout, if any */
	    if (stemp[0] != '\0')
	    {
		cs_cfg->clients[i].timeout = atol((pchar)&stemp) ;
 	    }
	    continue;
	}
	/* look for client[xx]=name[,timeout] */
	if (strncmp(stemp, "UID",3) == 0)
	{
	    i = cs_cfg->n_uids++ ;
	    /* get user id */
	    str_right (stemp, &str1[2]) ;
	    cs_cfg->user_privilege[i].user_id = atoi((pchar)&stemp) ;
	    /* get privilege mask */
	    cs_cfg->user_privilege[i].user_mask = atoi((pchar)&str2) ;
	    continue;
	}
    } while (1) ;
    close_cfg(&station_cfg) ;
    return 0;
}

/***********************************************************************
 * getAllCSServerParams
 *	Retrieve and return staion config parameters from STATION_INI.
 *   Return 0 on success, non-zero otherwise.
 **********************************************************************/  

int getAllCSServerParams (csconfig *cs_cfg, char *server_name)
{
    int rc;
    rc = getCSServerInfo (cs_cfg, server_name);
    if (rc != 0) return rc;
    rc = getCSServerParams (cs_cfg, cs_cfg->server_dir, cs_cfg->server_source);
    return rc;
}
