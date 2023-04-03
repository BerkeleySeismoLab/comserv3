/*
 * File: ConfigVO.h
 *
 * Purpose : This class encapsulates the configuration information that
 * is read from a Mountainair configuration file. It has constructors for both
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
 */

#ifndef _ConfigVO_H
#define _ConfigVO_H

#include <stdint.h>

#include "qservdefs.h"
#include "q8servcfg.h"

int read_config(char * configFileName);

class ConfigVO {

public:

    ConfigVO(q8serv_cfg);

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
    char *   getQ660TcpAddr() const;
    uint32_t getQ660BasePort() const;
    uint32_t getQ660Priority() const; // 1-4
    uint64_t getQ660SerialNumber() const;
    char *   getQ660Password() const;
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
    uint16_t getMulticastEnabled() const;
    uint16_t getMulticastPort() const;
    char *   getMulticastHost() const;
    char *   getMulticastChannelList() const;
    char *   getContFileDir() const;
    uint32_t getLimitBackfill() const;
    uint32_t getWaitForClients() const;
    uint32_t getPacketQueueSize() const;
    // Bandwith control options
    uint32_t getOptThrottleKbitpersec() const;
    uint32_t getOptBwfillKbitTarget() const;
    uint32_t getOptBwfillProbeInterval() const;
    uint32_t getOptBwfillExceedTrigger() const;
    uint32_t getOptBwfillIncreaseInterval() const;
    uint32_t getOptBwfillMaxLatency() const;

    void setServerName(char *input);
    void setServerDesc(char *input);
    void setServerDir(char *input);
    void setServerSource(char *input);
    void setSeedStation(char *input);
    void setSeedNetwork(char *input);
    void setLogDir(char *input);
    void setLogType(char *input);
    void setQ660TcpAddr(char* input);
    void setQ660BasePort(char* input);
    void setQ660Priority(char* input);
    void setQ660SerialNumber(char* input);
    void setQ660Password(char* input);
    void setLockFile(char * input);
    void setStartMsg(char* input);
    void setStatusInterval(char* input);
    void setVerbosity(char* input);
    void setDiagnostic(char* input);
    void setLogLevel(char *input);
    void setFailedRegistrationsBeforeSleep(char *input);
    void setMinutesToSleepBeforeRetry(char *input);
    void setDutyCycle_MaxConnectTime(char *input);
    void setDutyCycle_SleepTime(char *input);
    void setMulticastEnabled(char * input);
    void setMulticastPort(char * input);
    void setMulticastHost(char *input);
    void setMulticastChannelList(char *input);
    void setContFileDir(char *input);
    void setLimitBackfill(char *input);
    void setWaitForClients(char *input);
    void setPacketQueueSize(char *input);
    // Bandwith control options
    void setOptThrottleKbitpersec(char *input);
    void setOptBwfillKbitTarget(char *input);
    void setOptBwfillProbeInterval(char *input);
    void setOptBwfillExceedTrigger(char *input);
    void setOptBwfillIncreaseInterval(char *input);
    void setOptBwfillMaxLatency(char *input);

    void setQ660BasePort(uint32_t);
    void setQ660Priority(uint32_t);
    void setQ660SerialNumber(uint64_t);
    void setStatusInterval(uint32_t);
    void setVerbosity(uint32_t);
    void setDiagnostic(uint32_t);
    void setWaitForClients(uint32_t);
    void setPacketQueueSize(uint32_t);
    // Bandwith control options
    void setOptThrottleKbitpersec(uint32_t);
    void setOptBwfillKbitTarget(uint32_t);
    void setOptBwfillProbeInterval(uint32_t);
    void setOptBwfillExceedTrigger(uint32_t) ;
    void setOptBwfillIncreaseInterval(uint32_t) ;
    void setOptBwfillMaxLatency(uint32_t);

private:

    char p_server_name[CFGWIDTH];	// Not in config file, but set by config reader.
    char p_server_desc[CFGWIDTH];
    char p_server_dir[CFGWIDTH];
    char p_server_source[CFGWIDTH];
    char p_seed_station[CFGWIDTH];
    char p_seed_network[CFGWIDTH];
    char p_logdir[CFGWIDTH];
    char p_logtype[CFGWIDTH];
    char     p_q660_tcpaddr[CFGWIDTH];
    uint16_t p_q660_base_port;
    uint16_t p_q660_priority;
    uint64_t p_q660_serial_number;
    char     p_q660_password[CFGWIDTH];
    char p_lockfile[CFGWIDTH];
    char     p_startmsg[CFGWIDTH];
    uint32_t p_statusinterval;
    uint32_t p_verbosity;
    uint32_t p_diagnostic;
    uint32_t p_logLevel;
    uint16_t p_failedRegistrationsBeforeSleep;
    uint16_t p_minutesToSleepBeforeRetry;
    uint16_t p_dutycycle_maxConnectTime;
    uint16_t p_dutycycle_sleepTime;
    uint16_t p_multicast_enabled;
    uint16_t p_multicast_port;
    char     p_multicast_host[CFGWIDTH];
    char     p_multicast_channellist[512];
    char     p_contFileDir[CFGWIDTH];
    uint32_t p_limitBackfill;
    uint32_t p_waitForClients;
    uint32_t p_packetQueueSize;
    // Bandwidth control options
    uint32_t p_opt_throttle_kbitpersec;
    uint32_t p_opt_bwfill_kbit_target;
    uint32_t p_opt_bwfill_probe_interval;
    uint32_t p_opt_bwfill_exceed_trigger;
    uint32_t p_opt_bwfill_increase_interval;
    uint32_t p_opt_bwfill_max_latency;

    bool     p_configured;
};

#endif
