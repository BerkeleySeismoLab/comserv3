/*
 * File     :
 *   q8servcfg.h
 *
 * Purpose  :
 *  This is a c language header file for use when q8serv reads the comserv
 *  configuration file.
 *
 * Author   :
 *  Doug Neuhauser, based on code by Phil Maechling
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
 * Mod Date :
 *  28 March 2002
 *  2020-09-29 DSN Updated for comserv3.
 */

#ifndef Q8SERVCFG_H
#define Q8SERVCFG_H

#include "cslimits.h"

struct q8serv_cfg
{
    /* Info supplied by the program. */
    char server_name[CFGWIDTH];
    /* Info from STATIONS_INI file. */
    char server_desc[CFGWIDTH];
    char server_dir[CFGWIDTH];
    char server_source[CFGWIDTH];
    char seed_station[CFGWIDTH];
    char seed_network[CFGWIDTH];
    /* Info from either global NETWORK_INI or server's STATION_INI file. */
    char logdir[CFGWIDTH];
    char logtype[CFGWIDTH];
    char udpaddr[CFGWIDTH];
    char contFileDir[CFGWIDTH];
    char statusinterval[CFGWIDTH];
    char baseport[CFGWIDTH];
    char priority[CFGWIDTH];
    char serialnumber[CFGWIDTH];
    char password[CFGWIDTH];
    char lockfile[CFGWIDTH];
    char startmsg[CFGWIDTH];
    char verbosity[CFGWIDTH];
    char diagnostic[CFGWIDTH]; 
    char loglevel[CFGWIDTH];
    char failedRegistrationsBeforeSleep[CFGWIDTH];
    char minutesToSleepBeforeRetry[CFGWIDTH];
    char dutycycle_maxConnectTime[CFGWIDTH];
    char dutycycle_sleepTime[CFGWIDTH];
    char multicastEnabled[CFGWIDTH];
    char multicastPort[CFGWIDTH];
    char multicastHost[CFGWIDTH];
    char multicastChannelList[CFGWIDTH];
    char limitBackfill[CFGWIDTH];
    char waitForClients[CFGWIDTH];
};

#ifdef __cplusplus
extern "C" {
#endif
    int getq8servcfg(struct q8serv_cfg* q8cfg, char *server_name);
    void clearConfig(struct q8serv_cfg* q8cfg);
#ifdef __cplusplus
}
#endif

#endif
