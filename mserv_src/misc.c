/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

/***********************************************************************
 * GetLogParamsFromNetwork
 *	Retrieve and return logging config parameters from NETWORK_INI.
 *	logdir:		LOGDIR config parameter.
 *	logfile:	LOGTYPE config parameter (LOGFILE or STDOUT)
 **********************************************************************/  

void GetLogParamsFromNetwork(csconfig *cs_cfg)
{
    config_struc network_cfg;           /* structure for config file.   */
    char str1[SECWIDTH], str2[SECWIDTH];

    /* Scan network initialization file.                                */
    if (open_cfg(&network_cfg,NETWORK_INI,GLOBAL_DEFAULTS))
    {
        fprintf (stderr, "Warning: Could not find [%s] section in network file %s\n", 
		 GLOBAL_DEFAULTS, NETWORK_INI);
	return;
    }
    while (1)
    {
	read_cfg(&network_cfg,&str1[0],&str2[0]);
	if (str1[0] == '\0') 
	{
	    break;
	}
	/* Logging parameters.               */
	if (strcmp(str1,"LOGDIR")==0) {
	    strcpy(cs_cfg->logdir,str2);
	    continue;
	}
	if (strcmp(str1,"LOGTYPE")==0) {
	    strcpy(cs_cfg->logtype,str2);
	    continue;
	}
    }
    close_cfg(&network_cfg);
}

/***********************************************************************
 * GetStationParamsFromStations
 *	Retrieve and return staion config parameters from STATIONS_INI.
 *	server_dir	Directory containing the STATION_INI file.
 *	server_desc	Stations description string.
 *	server_source	Station config section in STATION_INIF file.
 *			Must be "comlink".
 *	seed_station	SEED station code.
 *	seed_network	SEED network code.
 **********************************************************************/  

void GetStationParamsFromStations(csconfig *cs_cfg, char *server_name)
{
    config_struc stations_cfg;           /* structure for config file.   */
    char str1[SECWIDTH], str2[SECWIDTH];

    /* Get selected station info from stations file. */
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
	/* Mandatory daemon configuration parameters.               */
	if (strcmp(str1,"DIR")==0) {
	    strcpy(cs_cfg->server_dir,str2);
	    continue;
	}
	if (strcmp(str1,"DESC")==0) {
	    strcpy(cs_cfg->server_desc,str2);
	    continue;
	}
	if (strcmp(str1,"SOURCE")==0) {
	    strcpy(cs_cfg->server_source,str2);
	    continue;
	}
	if (strcmp(str1,"STATION")==0) {
	    strcpy(cs_cfg->seed_station,str2);
	    continue;
	}
	if (strcmp(str1,"SITE")==0) {	/* Alias for STATION */
	    strcpy(cs_cfg->seed_station,str2);
	    continue;
	}
	if (strcmp(str1,"NET")==0) {
	    strcpy(cs_cfg->seed_network,str2);
	    continue;
	}
    }
    close_cfg(&stations_cfg);
}
