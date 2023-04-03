/*
 * File     :
 *   q330servcfg.h
 *
 * Purpose  :
 *  This is a c language header file for use when qma reads the comserv
 *  configuration file.
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
 *  2020-09-29 DSN Updated for comserv3.
 *  2022-03-16 DSN Added support for TCP connection to Q330/baler.
 */

#ifndef Q330CFG_H
#define Q330CFG_H

#include "cslimits.h"

struct q330serv_cfg
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
    char contFileDir[CFGWIDTH];
    char statusinterval[CFGWIDTH];
    char udpaddr[CFGWIDTH];
    char tcpaddr[CFGWIDTH];
    char baseport[CFGWIDTH];
    char dataport[CFGWIDTH];
    char serialnumber[CFGWIDTH];
    char authcode[CFGWIDTH];
    char verbosity[CFGWIDTH];
    char diagnostic[CFGWIDTH]; 
    char lockfile[CFGWIDTH];
    char startmsg[CFGWIDTH];
    char datarateinterval[CFGWIDTH];
    char loglevel[CFGWIDTH];
    char failedRegistrationsBeforeSleep[CFGWIDTH];
    char minutesToSleepBeforeRetry[CFGWIDTH];
    char dutycycle_maxConnectTime[CFGWIDTH];
    char dutycycle_sleepTime[CFGWIDTH];
    char dutycycle_bufferLevel[CFGWIDTH];
    char multicastPort[CFGWIDTH];
    char multicastHost[CFGWIDTH];
    char multicastEnabled[CFGWIDTH];
    char multicastChannelList[CFGWIDTH];
    char waitForClients[CFGWIDTH];
    char packetQueueSize[CFGWIDTH];
};

#ifdef __cplusplus
extern "C" {
#endif
    void clearConfig(struct q330serv_cfg* out_cfg);
    int GetServerParamsFromStationsIni(struct q330serv_cfg *out_cfg, char *server_name);
    int GetGlobalParamsFromNetworkIni(struct q330serv_cfg* out_cfg);
    int GetServerParamsFromStationIni(struct q330serv_cfg* out_cfg, char *prog_section_name);
    int getq330servcfg(struct q330serv_cfg* out_cfg, char *server_name);
#ifdef __cplusplus
}
#endif

#endif
