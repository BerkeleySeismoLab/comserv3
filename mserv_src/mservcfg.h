/*
 * File     :
 *   mservcfg.h
 *
 * Purpose  :
 *  This is a c language header file for use when mserv reads the comserv
 *  configuration file.
 *
 * Author   :
 *  Phil Maechling
 *
 * Mod Date :
 *  28 March 2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *
 * 2020-09-29 DSN Updated for comserv3.
 */

#ifndef MSERVCFG_H
#define MSERVCFG_H

#include "cslimits.h"

struct mserv_cfg
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
    /* Specific station/program settings */
    char mcastif[CFGWIDTH];
    char udpaddr[CFGWIDTH];
    char ipport[CFGWIDTH];
    char lockfile[CFGWIDTH];
    char startmsg[CFGWIDTH];
    char verbosity[CFGWIDTH];
    char diagnostic[CFGWIDTH]; 
    char loglevel[CFGWIDTH];
    char waitForClients[CFGWIDTH];
    char packetQueueSize[CFGWIDTH];
};

#ifdef __cplusplus
extern "C" {
#endif
    void clearConfig(struct mserv_cfg* out_cfg);
    int GetServerParamsFromStationsIni(struct mserv_cfg *out_cfg, char *server_name);
    int GetGlobalParamsFromNetworkIni(struct mserv_cfg* out_cfg);
    int GetServerParamsFromStationIni(struct mserv_cfg* out_cfg, char *prog_section_name);
    int getmservcfg(struct mserv_cfg *out_cfg, char *server_name);
#ifdef __cplusplus
}
#endif

#endif
