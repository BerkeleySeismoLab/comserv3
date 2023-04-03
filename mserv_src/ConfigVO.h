/* 
 * File : ConfigVO.h
 *
 * Purpose : This class encapsulates the configuration information that
 * is read from a mserv configuration file. It has constructors for both
 * data and string types. There are get and set methods to access the
 * data once it is set.
 *
 *
 * Author   :
 *  Phil Maechling
 *
 * Mod Date :
 *  27 July 2002
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

#ifndef _ConfigVO_H
#define _ConfigVO_H

#include <stdint.h>

#include "qservdefs.h"
#include "mservcfg.h"

int read_config(char * configFileName);

class ConfigVO {

public:

    ConfigVO(mserv_cfg);

    ConfigVO();
    ~ConfigVO() {};

    char *   getServerName() const;
    char *   getServerDesc() const;
    char *   getServerDir() const;
    char *   getServerSource() const;
    char *   getSeedStation() const;
    char *   getSeedNetwork() const;
    char *   getLogDir() const;
    char *   getLogType() const;
    char *   getMcastIf() const;
    char *   getUdpAddr() const;
    uint32_t getIPPort() const;
    char *   getLockFile() const;
    char *   getStartMsg() const;
    uint32_t getStatusInterval() const;
    uint32_t getVerbosity() const;
    uint32_t getDiagnostic() const;
    uint32_t getLogLevel() const;
    char *   getContFileDir() const;
    uint32_t getWaitForClients() const;
    uint32_t getPacketQueueSize() const;

    void setServerName(char *input);
    void setServerDesc(char *input);
    void setServerDir(char *input);
    void setServerSource(char *input);
    void setSeedStation(char *input);
    void setSeedNetwork(char *input);
    void setLogDir(char *input);
    void setLogType(char *input);
    void setMcastIf(char *input);
    void setUdpAddr(char *input);
    void setIPPort(char *input);
    void setLockFile(char* input);
    void setStartMsg(char* input);
    void setStatusInterval(char *input);
    void setVerbosity(char* input);
    void setDiagnostic(char* input);
    void setLogLevel(char *input);
    void setContFileDir(char *input);
    void setWaitForClients(char *input);
    void setPacketQueueSize(char *input);

    void setIPPort(uint32_t);
    void setLogLevel(uint32_t);
    void setStatusInterval(uint32_t);
    void setVerbosity(uint32_t);
    void setDiagnostic(uint32_t);
    void setWaitForClients(uint32_t);
    void setPacketQueueSize(uint32_t);

private:
    char p_server_name[256];	// Not in config file, but set by config reader.
    char p_server_desc[256];
    char p_server_dir[256];
    char p_server_source[256];
    char p_seed_station[256];
    char p_seed_network[256];
    char p_logdir[256];
    char p_logtype[256];
    char p_mcastif[256];
    char p_udpaddr[256];
    uint32_t p_ipport;
    char p_lockfile[256];
    char p_startmsg[256];
    uint32_t p_statusinterval;
    uint32_t p_verbosity;
    uint32_t p_diagnostic;
    uint32_t p_logLevel;
    char p_contFileDir[256];
    uint32_t p_waitForClients;
    uint32_t p_packetQueueSize;

    bool       p_configured;
};

#endif
