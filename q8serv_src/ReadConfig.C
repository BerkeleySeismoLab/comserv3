/*
 * Program: 
 *   ReadConfig.C
 *
 * Purpose:
 *   These is processing routine is used by the q8serv main routine
 *   to read in configuration information. It uses the comserv configuration
 *   routines to read the config file 
 *
 * Author:
 *   Doug Neuhauser, based on code from Phil Maechling
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
 * Modifications:
 *  2020-09-29 DSN Updated for comserv3.
 */

#include <iostream>
#include <string.h>
#include "ReadConfig.h"
#include "Verbose.h"
#include "qservdefs.h"
#include "ConfigVO.h"
#include "q8servcfg.h"
#include "global.h"

extern Verbose      g_verbosity;
extern ConfigVO     g_cvo;


bool readConfigFile(char* server_name)
{
    struct q8serv_cfg q8cfg;
    int res;

    res = getq8servcfg(&q8cfg, server_name);

    if (res != QSERV_SUCCESS)
    {
        g_log << "xxx Error initializing from config file for server : " <<
            server_name << std::endl;
        return false;
    }

    // Call to see that all required config values are set.
    res = validateq8servConfig(q8cfg);
    if (res == false) return false;

    // Initialize class with config info, and update global class.
    ConfigVO tcvo(q8cfg);
    g_cvo = tcvo;

    return true;
}

bool validateq8servConfig(const struct q8serv_cfg& aCfg)
{
    int len;
    //
    // Required fields are 
    // tcpaddr
    // baseport
    // priority
    // serial number
    // password

    len = strlen(aCfg.tcpaddr);
    if (len < 1)
    {
	g_log << 
	    "xxx Configuration file is missing value for tcpaddr:" << std::endl;
	return false;
    }

    len = strlen (aCfg.baseport);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for baseport:" << std::endl;
	return false;
    }

    len = strlen (aCfg.priority);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for priority:" << std::endl;
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

    len = strlen (aCfg.password);
    if (len < 1)
    {
	g_log << 
	    "xxx - Configuration file is missing value for password:" << std::endl;
	return false;
    }

    return true;
}
