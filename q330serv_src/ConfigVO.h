/*
 * Purpose : This class encapsulates the configuration information that
 * is read from a q330serv configuration file. It has constructors for both
 * data and string types. There are get and set methods to access the
 * data once it is set.
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
 * Modification History:
 *  27 July 2002
 *  2020-09-29 DSN Updated for comserv3.
 *  2022-03-16 DSN Added support for TCP connection to Q330/baler.
 */

#ifndef _ConfigVO_H
#define _ConfigVO_H

#include <stdint.h>

#include "qservdefs.h"
#include "q330servcfg.h"

int read_config(char * configFileName);

class ConfigVO {

public:

    ConfigVO(q330serv_cfg);

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
    char *   getQ330UdpAddr() const;
    char *   getQ330TcpAddr() const;
    uint32_t getQ330BasePort() const;
    uint32_t getQ330DataPortNumber() const; // 1-4
    uint64_t getQ330SerialNumber() const;
    uint64_t getQ330AuthCode() const;
    char *   getLockFile() const;
    char *   getStartMsg() const;
    uint32_t getStatusInterval() const;
    uint32_t getVerbosity() const;
    uint32_t getDiagnostic() const;
    uint32_t getLogLevel() const;
    uint16_t getFailedRegistrationsBeforeSleep() const;
    uint16_t getMinutesToSleepBeforeRetry() const;
    uint16_t getDutyCycle_MaxConnectTime() const;
    uint16_t getDutyCycle_SleepTime() const;
    uint16_t getDutyCycle_BufferLevel() const;
    uint16_t getMulticastEnabled() const;
    uint16_t getMulticastPort() const;
    char *   getMulticastHost() const;
    char *   getMulticastChannelList() const;
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
    void setQ330UdpAddr(char * input);
    void setQ330TcpAddr(char * input);
    void setQ330BasePort(char * input);
    void setQ330DataPortNumber(char * input);
    void setQ330SerialNumber(char * input);
    void setQ330AuthCode(char * input);
    void setLockFile(char * input);
    void setStartMsg(char * input);
    void setStatusInterval(char * input);
    void setVerbosity(char * input);
    void setDiagnostic(char * input);
    void setLogLevel(char * input);
    void setFailedRegistrationsBeforeSleep(char * input);
    void setMinutesToSleepBeforeRetry(char * input);
    void setDutyCycle_MaxConnectTime(char * input);
    void setDutyCycle_SleepTime(char * input);
    void setDutyCycle_BufferLevel(char * input);
    void setMulticastEnabled(char * input);
    void setMulticastPort(char * input);
    void setMulticastHost(char * input);
    void setMulticastChannelList(char * input);
    void setContFileDir(char * input);
    void setWaitForClients(char *input);
    void setPacketQueueSize(char *input);

    void setQ330BasePort(uint32_t);
    void setQ330DataPortNumber(uint32_t);
    void setQ330SerialNumber(uint64_t);
    void setQ330AuthCode( uint64_t);
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
    char     p_q330_udpaddr[255];
    char     p_q330_tcpaddr[255];
    uint16_t p_q330_base_port;
    uint16_t p_q330_data_port;
    uint64_t p_q330_serial_number;
    uint64_t p_q330_auth_code;
    char     p_lockfile[256];
    char     p_startmsg[256];
    uint32_t p_statusinterval;
    uint32_t p_verbosity;
    uint32_t p_diagnostic;
    uint32_t p_logLevel;
    uint16_t p_failedRegistrationsBeforeSleep;
    uint16_t p_minutesToSleepBeforeRetry;
    uint16_t p_dutycycle_maxConnectTime;
    uint16_t p_dutycycle_sleepTime;
    uint16_t p_dutycycle_bufferLevel;
    uint16_t p_multicast_enabled;
    uint16_t p_multicast_port;
    char     p_multicast_host[256];
    char     p_multicast_channellist[512];
    char     p_contFileDir[256];
    uint32_t p_waitForClients;
    uint32_t p_packetQueueSize;

    bool     p_configured;
};

#endif
