/*
 * Program: 
 *   ReadConfig.C
 *
 * Purpose:
 *   These is processing routine is used by the mserv main routine
 *   to read in configuration information. It uses the comserv configuration
 *   routines to read the config file 
 *
 * Author:
 *   Phil Maechling
 *
 * Created:
 *   4 April 2002
 *
 * Modifications:
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
 * 29 Sep 2020 DSN Updated for comserv3.
 */
#include <iostream>
#include <string.h>
#include "ReadConfig.h"
#include "Verbose.h"
#include "qservdefs.h"
#include "ConfigVO.h"
#include "mservcfg.h"
#include "global.h"

extern Verbose      g_verbosity;
extern ConfigVO     g_cvo;


bool readConfigFile(char* server_name)
{
    struct mserv_cfg mcfg;
    int res;

    res = getmservcfg(&mcfg, server_name);

    if (res != QSERV_SUCCESS)
    {
	g_log << "xxx Error initializing from config file for server : " << 
	    server_name << std::endl;
        return false;
    }

    // Call to check that all required config values are set
    res = validateMservConfig(mcfg);
    if(res == false) return false;

    // Initialize class with config info, and update global class.
    ConfigVO tcvo(mcfg);
    g_cvo = tcvo;

    return true;
}

bool validateMservConfig(const struct mserv_cfg& aCfg)
{
    int len;

    // Required fields are 
    // 	mcastif
    //  ipaddr
    //  udpaddr

    len = strlen(aCfg.mcastif);
    if(len < 1)
    {
	g_log << 
	    "xxx Configuration file is missing value for mcastif:" << std::endl;
	return false;
    }
    len = strlen(aCfg.udpaddr);
    if(len < 1)
    {
	g_log << 
	    "xxx Configuration file is missing value for udpaddr:" << std::endl;
	return false;
    }
    len = strlen(aCfg.ipport);
    if(len < 1)
    {
	g_log << 
	    "xxx Configuration file is missing value for ipport:" << std::endl;
	return false;
    }

    return true;
}
