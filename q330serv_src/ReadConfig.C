/*
 * Program: ReadConfig.C
 *
 * Purpose:
 *   These is processing routine is used by the q330serv main routine
 *   to read in configuration information. It uses the comserv configuration
 *   routines to read the config file 
 *
 * Author:
 *   Phil Maechling
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
 *   4 April 2002
 *  2020-09-29 DSN Updated for comserv3.
 */

#include <iostream>
#include <string.h>
#include "ReadConfig.h"
#include "Verbose.h"
#include "qservdefs.h"
#include "q330Diag.h"
#include "ConfigVO.h"
#include "q330servcfg.h"
#include "global.h"

extern Verbose      g_verbosity;
extern ConfigVO     g_cvo;


bool readConfigFile(char* server_name)
{
    struct q330serv_cfg q330cfg;
    int res;

    res = getq330servcfg(&q330cfg, server_name);

    if (res != QSERV_SUCCESS)
    {
	g_log << "xxx Error initializing from config file for server : " << 
	    server_name << std::endl;
        return false;
    }

    // Call to check that all required config values are set
    res = validateQ330servConfig(q330cfg);
    if(res == false) return false;

    // Initialize class with config info, and update global class.
    ConfigVO tcvo(q330cfg);
    g_cvo = tcvo;

    return true;
}

bool validateQ330servConfig(const struct q330serv_cfg& aCfg)
{
    int len;
    // Required fields are:
    // udpaddr
    // baseport
    // dataport
    // serial number
    // authcode
 
    len = strlen(aCfg.udpaddr);
    if (len < 1)
    {
	g_log << 
	    "xxx Configuration file is missing value for udpaddr:" << std::endl;
	return false;
    }

    len = strlen (aCfg.baseport);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for baseport:" << std::endl;
	return false;
    }

    len = strlen (aCfg.dataport);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for dataport:" << std::endl;
	return false;
    }

    len = strlen (aCfg.serialnumber);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for serialnumber:" 
	      << std::endl;
	return false;
    }

    len = strlen (aCfg.authcode);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for authcode:" << std::endl;
	return false;
    }

    return true;
}
