/*
 * File     :
 *   mservcfg.c
 *
 * Purpose  :
 *  This is used by mserv to scan the appropriate mserv config section
 *  and copy the information needed to a mserv_cfg structure.
 *
 * Author   :
 *  Phil Maechling
 *
 * Mod Date :
 *  28 March 2002
 *  2015/09/25 - DSN - Added close_cfg() calls to close all config files.
 *	
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * 2020-09-29 DSN Updated for comserv3.
 */
#include "stuff.h"
#include "qservdefs.h"
#include "cfgutil.h"
#include "service.h"
#include "logging.h"
#include "global.h"
#include "mservcfg.h"

extern void terminate (char *);

#ifdef COMSERV2
static char *global_defaults_section_name = "netm";
#else
static char *prog_section_name = "mserv";
static char *global_defaults_section_name = "global_defaults";
#endif

/***********************************************************************
 * clearConfig
 *	Clear / Initialize the config structure.
 **********************************************************************/  
void clearConfig(struct mserv_cfg* out_cfg)
{
    memset(out_cfg,0,sizeof(struct mserv_cfg));
}

/***********************************************************************
 * GetServeraramsFromStationsIni
 *	Retrieve and return staion config parameters from STATIONS_INI.
 *	server_dir	Directory containing the STATION_INI file.
 *	server_desc	Stations description string.
 *	server_source	Station comserv config section in STATION_INI file.
 *	seed_station	SEED station code.
 *	seed_network	SEED network code.
 **********************************************************************/  

int GetServerParamsFromStationsIni(struct mserv_cfg *out_cfg, char *server_name)
{
    config_struc stations_cfg;           /* structure for config file.   */
    char str1[SECWIDTH], str2[SECWIDTH];
    char *stations_ini;

    /* Get selected station info from stations file. */
    memset (&stations_cfg, 0, sizeof(stations_cfg));
    stations_ini = get_stations_ini_pathname();
    if (open_cfg(&stations_cfg,stations_ini,server_name))
    {
        fprintf (stderr, "Could not find [%s] section in file %s\n", 
		 server_name, stations_ini);
	return (QSERV_FAILURE);
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
	    strcpy(out_cfg->server_dir,str2);
	    continue;
	}
	if (strcmp(str1,"DESC")==0)
	{
	    strcpy(out_cfg->server_desc,str2);
	    continue;
	}
	if (strcmp(str1,"SOURCE")==0)
	{
	    strcpy(out_cfg->server_source,str2);
	    continue;
	}
	if (strcmp(str1,"STATION")==0 || strcmp(str1,"SITE")==0)
	{
	    strcpy(out_cfg->seed_station,str2);
	    continue;
	}
	if (strcmp(str1,"NETWORK")==0 || strcmp(str1,"NET")==0)
	{
	    strcpy(out_cfg->seed_network,str2);
	    continue;
	}
    }
    close_cfg(&stations_cfg);
    return QSERV_SUCCESS;
}

/***********************************************************************
 * GetGlobalParamsFromNetworkIni
 *   Retrieve and return any pertinent global parameters from NETWORK_INI.
 *   Silently ignore any paremters that you are not interested in.
 **********************************************************************/  

int GetGlobalParamsFromNetworkIni(struct mserv_cfg* out_cfg)
{
    config_struc network_cfg;           /* structure for config file op */
    char str1[CFGWIDTH], str2[CFGWIDTH];
    char *network_ini;

    /* Scan network initialization file.                                */
    memset (&network_cfg, 0, sizeof(network_cfg));
    network_ini = get_network_ini_pathname();
    if (open_cfg(&network_cfg,network_ini,global_defaults_section_name))
    {
        fprintf (stderr, "Warning: Could not find [%s] section in network file %s\n", 
		 global_defaults_section_name, network_ini);
	return QSERV_FAILURE;
    }
    while (1)
    {
	read_cfg(&network_cfg,&str1[0],&str2[0]);
	if (str1[0] == '\0') 
	{
	    break;
	}
	if (strcmp(str1, "LOGDIR") == 0)
	{
	    strcpy(out_cfg->logdir, str2);
	    continue;
	}
	if (strcmp(str1, "LOGTYPE") == 0) 
	{
	    strcpy(out_cfg->logtype, str2);
	    continue;
	}
	if (strcmp(str1, "CONTFILEDIR") == 0)
	{
	    strcpy(out_cfg->contFileDir, str2);
	    continue;
	}
	if (strcmp(str1, "STATUSINTERVAL") == 0)
	{
	    strcpy(out_cfg->statusinterval, str2) ;
	    continue;
	}
	if (strcmp(str1, "VERBOSITY") == 0)
	{
	    strcpy(out_cfg->verbosity, str2) ;
	    continue;
	}
	if (strcmp(str1, "WAITFORCLIENTS") == 0)
	{
	    strcpy(out_cfg->waitForClients, str2) ;
	    continue;
	}
	if (strcmp(str1, "PACKETQUEUESIZE") == 0)
	{
	    strcpy(out_cfg->packetQueueSize, str2) ;
	    continue;
	}
    }
    close_cfg(&network_cfg);
    return QSERV_SUCCESS;
}

/***********************************************************************
 * GetServerParamsFromStationIni
 *	Retrieve server and program specific parameters from STATION_INI.
 *   Silently ignore any paremters that you are not interested in.
 **********************************************************************/  
int GetServerParamsFromStationIni(struct mserv_cfg* out_cfg, char *section_name)
{
    config_struc cfg ;
    char filename[CFGWIDTH] ;
    char str1[CFGWIDTH], str2[CFGWIDTH];

    /* Try to open the STATION_INI file in this station's directory */
    sprintf (filename, "%s/%s", out_cfg->server_dir, STATION_INI);
    memset (&cfg, 0, sizeof(cfg));
    if (open_cfg(&cfg, filename, section_name))
    {
	fprintf (stderr, "Warning: Could not find a [%s] section in %s\n", section_name, filename);
	return QSERV_FAILURE;
    }

    /* Now with file open, scan for program/server info */
    while (1)
    {
	read_cfg(&cfg, str1, str2) ;
	if (str1[0] == '\0')
	{
	    break ;
	}
	/* Possible global parameters. */
	if (strcmp(str1, "LOGDIR") == 0)
	{
	    strcpy(out_cfg->logdir, str2);
	    continue;
	}
	if (strcmp(str1, "LOGTYPE") == 0)
	{
	    strcpy(out_cfg->logtype, str2);
	    continue;
	}
	if (strcmp(str1, "CONTFILEDIR") == 0)
	{
	    strcpy(out_cfg->contFileDir, str2);
	    continue;
	}
	if (strcmp(str1, "STATUSINTERVAL") == 0)
	{
	    strcpy(out_cfg->statusinterval, str2) ;
	    continue;
	}
	if (strcmp(str1, "WAITFORCLIENTS") == 0)
	{
	    strcpy(out_cfg->waitForClients, str2) ;
	    continue;
	}
	if (strcmp(str1, "PACKETQUEUESIZE") == 0)
	{
	    strcpy(out_cfg->packetQueueSize, str2) ;
	    continue;
	}
	/* Server/program specific parameters. */
	if (strcmp(str1, "MCASTIF") == 0)
	{
	    strcpy(out_cfg->mcastif, str2) ;
	    continue;
	}
	if (strcmp(str1, "UDPADDR") == 0)
	{
	    strcpy(out_cfg->udpaddr, str2) ;
	    continue;
	}
	if (strcmp(str1, "IPPORT") == 0)
	{
	    strcpy(out_cfg->ipport, str2) ;
	    continue;
	}
	if (strcmp(str1, "LOCKFILE") == 0)
	{
	    strcpy(out_cfg->lockfile, str2) ;
	    continue;
	}
	if (strcmp(str1, "STARTMSG") == 0)
	{
	    strcpy(out_cfg->startmsg, str2) ;
	    continue;
	}
	if (strcmp(str1, "VERBOSITY") == 0)
	{
	    strcpy(out_cfg->verbosity, str2) ;
	    continue;
	}
	if (strcmp(str1, "DIAGNOSTIC") == 0)
	{
	    strcpy(out_cfg->diagnostic, str2) ;
	    continue;
	}
	if (strcmp(str1, "LOGLEVEL") == 0)
	{
	    strcpy(out_cfg->loglevel, str2);
	    continue;
	}
    }
    close_cfg(&cfg);
    return QSERV_SUCCESS;
}

/***********************************************************************
 * getmservcfg
 *	Retrieve and return any parmaters for mserv.
 *      Source of info:  
 *		STATIONS_INI file (for server info)
 *		NETWORK_INI file (for any global parameters)
 *		STATION_INI file (for server-specific parameters, which
 *			may override global parameters)
 *   Silently ignore any paremters that you are not interested in.
 **********************************************************************/  

int getmservcfg(struct mserv_cfg *out_cfg, char *server_name)
{
    int status;
    /*
     * Clear the config structure.
     */
    clearConfig(out_cfg);
    strcpy (out_cfg->server_name, server_name);

    /*
      Start by reading the server section from the STATIONS_INI file.
    */
    status = GetServerParamsFromStationsIni(out_cfg, server_name);
    if (status != QSERV_SUCCESS) return status;

    /*
      Next read global info from the NETWORK_INI file.
    */
    status = GetGlobalParamsFromNetworkIni(out_cfg);
    if (status != QSERV_SUCCESS) return status;

    /* 
     * Finally read program-specific server info in the STATION_INI file.
     * The "source=" general section will be read by comserv's config routines.
     */
#ifdef COMSERV2
    // In COMSERV2 there was no separate prog_section_name for mserv.
    // All info for mserv was in the comlink section.
    // Initialize ipport string to some non-zero value to avoid interim error condition.
    // The true value of ipport will be read and used from the comlink section.
    strcpy (out_cfg->ipport, "1");
#else
    GetServerParamsFromStationIni(out_cfg, prog_section_name);
    if (status != QSERV_SUCCESS) return status;
#endif

    return QSERV_SUCCESS;
}
