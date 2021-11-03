/*
 * File : q330servcfg.c
 *
 * Purpose  :
 *  This is used by q330 to scan the appropriate comserv config file 
 *  and copy the information needed to a q330 config file.
 *
 * Author   :
 *  Phil Maechling
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
 * Modification History:
 *  28 March 2002
 *  2015/09/25 - DSN - Added close_cfg() calls to close all config files.
 *  2020-09-29 DSN Updated for comserv3.
 */

#include "q330servcfg.h"
#include "qservdefs.h"
#include "stuff.h"
#include "cfgutil.h"
#include "service.h"
#include "logging.h"
#include "global.h"

extern void terminate (char *);

static char *prog_section_name = "q330serv";
#ifdef COMSERV2
static char *global_defaults_section_name = "netm";
#else
static char *global_defaults_section_name = "global_defaults";
#endif

/***********************************************************************
 * clearConfig
 *      Clear / Initialize the config structure.
 **********************************************************************/

void clearConfig(struct q330serv_cfg* out_cfg)
{
    memset(out_cfg,0,sizeof(struct q330serv_cfg));
}

/***********************************************************************
 * GetServeraramsFromStationsIni
 *	Retrieve and return staion config parameters from STATIONS_INI.
 *	server_dir	Directory containing the STATION_INI file.
 *	server_desc	Stations description string.
 *	server_source	Station comserv config section in STATION_INIF file.
 *	seed_station	SEED station code.
 *	seed_network	SEED network code.
 **********************************************************************/  

int GetServerParamsFromStationsIni(struct q330serv_cfg *out_cfg, char *server_name)
{
    config_struc stations_cfg;           /* structure for config file.   */
    char str1[SECWIDTH], str2[SECWIDTH];

    /* Get selected station info from stations file. */
    memset (&stations_cfg, 0, sizeof(stations_cfg));
    if (open_cfg(&stations_cfg,STATIONS_INI,server_name))
    {
        fprintf (stderr, "Could not find [%s] section in file %s\n", 
		 server_name, STATIONS_INI);
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

int GetGlobalParamsFromNetworkIni(struct q330serv_cfg* out_cfg)
{
    config_struc network_cfg;           /* structure for config file op */
    char str1[CFGWIDTH], str2[CFGWIDTH];

    /* Scan network initialization file.                                */
    memset (&network_cfg, 0, sizeof(network_cfg));
    if (open_cfg(&network_cfg,NETWORK_INI,global_defaults_section_name))
    {
        fprintf (stderr, "Warning: Could not find [%s] section in network file %s\n", 
		 global_defaults_section_name, NETWORK_INI);
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
    }
    close_cfg(&network_cfg);
    return QSERV_SUCCESS;
}

/***********************************************************************
 * GetServerParamsFromStationIni
 *	Retrieve server and program specific parameters from STATION_INI.
 *   Silently ignore any paremters that you are not interested in.
 **********************************************************************/  
int GetServerParamsFromStationIni(struct q330serv_cfg* out_cfg, char *section_name)
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
	if (strcmp(str1, "MULTICASTENABLED") == 0)
	{
	    strcpy(out_cfg->multicastEnabled, str2);
	    continue;
	}
	if (strcmp(str1, "MULTICASTPORT") == 0)
	{
	    strcpy(out_cfg->multicastPort, str2);
	    continue;
	}
	if (strcmp(str1, "MULTICASTHOST") == 0)
	{
	    strcpy(out_cfg->multicastHost, str2);
	    continue;
	}
	if (strcmp(str1, "MULTICASTCHANNELLIST") == 0)
	{
	    strcpy(out_cfg->multicastChannelList, str2);
	    continue;
	}
	/* Server/program specific parameters. */
	if (strcmp(str1, "UDPADDR") == 0)
	{
	    strcpy(out_cfg->udpaddr, str2) ;
	    continue;
	}
	if (strcmp(str1, "BASEPORT") == 0)
	{
	    strcpy(out_cfg->baseport, str2) ;
	    continue;
	}
	if (strcmp(str1, "DATAPORT") == 0)
	{
	    strcpy(out_cfg->dataport, str2) ;
	    continue;
	}
	if (strcmp(str1, "SERIALNUMBER") == 0)
	{
	    strcpy(out_cfg->serialnumber, str2) ;
	    continue;
	}
	if (strcmp(str1, "AUTHCODE") == 0)
	{
	    strcpy(out_cfg->authcode, str2) ;
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
	if (strcmp(str1, "STATUSINTERVAL") == 0)
	{
	    strcpy(out_cfg->statusinterval, str2) ;
	    continue;
	}
	if (strcmp(str1, "DATARATEINTERVAL") == 0)
	{
	    strcpy(out_cfg->datarateinterval, str2) ;
	    continue;
	}
	if (strcmp(str1, "FAILEDREGISTRATIONSBEFORESLEEP") == 0)
	{
	    strcpy(out_cfg->failedRegistrationsBeforeSleep, str2);
	    continue;
	}
	if (strcmp(str1, "MINUTESTOSLEEPBEFORERETRY") == 0)
	{
	    strcpy(out_cfg->minutesToSleepBeforeRetry, str2);
	    continue;
	}
	if (strcmp(str1, "DUTYCYCLE_MAXCONNECTTIME") == 0)
	{
	    strcpy(out_cfg->dutycycle_maxConnectTime, str2);
	    continue;
	}
	if (strcmp(str1, "DUTYCYCLE_SLEEPTIME") == 0)
	{
	    strcpy(out_cfg->dutycycle_sleepTime, str2);
	    continue;
	}
	if (strcmp(str1, "DUTYCYCLE_BUFFERLEVEL") == 0)
	{
	    strcpy(out_cfg->dutycycle_bufferLevel, str2);
	    continue;
	}
	if (strcmp(str1, "WAITFORCLIENTS") == 0)
	{
	    strcpy(out_cfg->waitForClients, str2) ;
	    continue;
	}
    }
    close_cfg(&cfg);
    return QSERV_SUCCESS;
}

/***********************************************************************
 * getq330servcfg
 *	Retrieve and return any parmaters for mserv.
 *      Source of info:  
 *		STATIONS_INI file (for server info)
 *		NETWORK_INI file (for any global parameters)
 *		STATION_INI file (for server-specific parameters, which
 *			may override global parameters)
 *   Silently ignore any paremters that you are not interested in.
 **********************************************************************/  

int getq330servcfg(struct q330serv_cfg *out_cfg, char *server_name)
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
    GetServerParamsFromStationIni(out_cfg, prog_section_name);
    if (status != QSERV_SUCCESS) return status;

    return QSERV_SUCCESS;
}
