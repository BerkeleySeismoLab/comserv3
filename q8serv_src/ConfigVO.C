/*
 * File: ConfigVO.C
 *
 * Purpose : This class encapsulates the configuration information that
 * is read from a q8serv configuration file. It has constructors for both
 * data and string types. There are get and set methods to access the
 * data once it is set.
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
 *  2020-09-29 DSN Updated for comserv3.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "qservdefs.h"
#include "ConfigVO.h"
#include "q8Diag.h"
#include "portingtools.h"
#include "global.h"

ConfigVO::ConfigVO(q8serv_cfg cfg) {
    // Info from STATIONS_INI
    setServerName(cfg.server_name);
    setServerDesc(cfg.server_desc);
    setServerDir(cfg.server_dir);
    setServerSource(cfg.server_source);
    setSeedStation(cfg.seed_station);
    setSeedNetwork(cfg.seed_network);
    // Info from global defaults or program section.
    setLogDir(cfg.logdir);
    setLogType(cfg.logtype);
    setQ660UdpAddr(cfg.udpaddr);
    setQ660BasePort(cfg.baseport);
    setQ660Priority(cfg.priority);
    setQ660SerialNumber(cfg.serialnumber);
    setQ660Password(cfg.password);
    setLockFile(cfg.lockfile);
    setStartMsg(cfg.startmsg);
    setStatusInterval(cfg.statusinterval);
    setVerbosity(cfg.verbosity);
    setDiagnostic(cfg.diagnostic);
    setLogLevel(cfg.loglevel);
    setFailedRegistrationsBeforeSleep(cfg.failedRegistrationsBeforeSleep);
    setMinutesToSleepBeforeRetry(cfg.minutesToSleepBeforeRetry);
    setDutyCycle_MaxConnectTime(cfg.dutycycle_maxConnectTime);
    setDutyCycle_SleepTime(cfg.dutycycle_sleepTime);
    setMulticastEnabled(cfg.multicastEnabled);
    setMulticastPort(cfg.multicastPort);
    setMulticastHost(cfg.multicastHost);
    setMulticastChannelList(cfg.multicastChannelList);
    setContFileDir(cfg.contFileDir);
    setLimitBackfill(cfg.limitBackfill);
    setWaitForClients(cfg.waitForClients);
    setOptThrottleKbitpersec(cfg.opt_throttle_kbitpersec);
    setOptBwfillKbitTarget(cfg.opt_bwfill_kbit_target);
    setOptBwfillProbeInterval(cfg.opt_bwfill_probe_interval);
    setOptBwfillExceedTrigger(cfg.opt_bwfill_exceed_trigger);
    setOptBwfillIncreaseInterval(cfg.opt_bwfill_increase_interval);
    setOptBwfillMaxLatency(cfg.opt_bwfill_max_latency);

    p_configured = true;
}

ConfigVO::ConfigVO()
{
    memset(p_server_name, 0, sizeof(p_server_name));
    memset(p_server_desc, 0, sizeof(p_server_desc));
    memset(p_server_dir, 0, sizeof(p_server_dir));
    memset(p_server_source, 0, sizeof(p_server_source));
    memset(p_seed_station, 0, sizeof(p_seed_station));
    memset(p_seed_network, 0, sizeof(p_seed_network));
    memset(p_logdir, 0, sizeof(p_logdir));
    memset(p_logtype, 0, sizeof(p_logtype));
    memset(p_q660_udpaddr, 0, sizeof(p_q660_udpaddr));
    p_q660_base_port = 0;
    p_q660_priority = 0;
    p_q660_serial_number = 0;
    memset(p_q660_password, 0, sizeof(p_q660_password));
    memset(p_lockfile, 0, sizeof(p_lockfile));
    memset(p_startmsg, 0, sizeof(p_startmsg));
    p_statusinterval = DEFAULT_STATUS_INTERVAL;
    p_verbosity = D_SILENT;
    p_diagnostic = D_NO_TARGET;
    p_logLevel = 0;
    p_failedRegistrationsBeforeSleep = 0;
    p_minutesToSleepBeforeRetry = 0;
    p_dutycycle_maxConnectTime = 0;
    p_dutycycle_sleepTime = 0;
    p_multicast_enabled = 0;
    p_multicast_port = 0;
    memset(p_multicast_host, 0, sizeof(p_multicast_host));
    memset(p_multicast_channellist, 0, sizeof(p_multicast_channellist));
    memset(p_contFileDir, 0, sizeof(p_contFileDir));
    p_limitBackfill = 0;
    p_waitForClients = 0;
    p_opt_throttle_kbitpersec = 0;
    p_opt_bwfill_kbit_target = 0;
    p_opt_bwfill_probe_interval = 0;
    p_opt_bwfill_exceed_trigger = 0;
    p_opt_bwfill_increase_interval = 0;
    p_opt_bwfill_max_latency = 0;
    
    p_configured = false;
}

//**********************************************************************

//
// Get values.
// 

char * ConfigVO::getServerName() const
{
    return (char *)p_server_name; 
}

char * ConfigVO::getServerDesc() const
{
    return (char *)p_server_desc; 
}

char * ConfigVO::getServerDir() const
{
    return (char *)p_server_dir; 
}

char * ConfigVO::getServerSource() const
{
    return (char *)p_server_source; 
}

char * ConfigVO::getSeedStation() const
{
    return (char *)p_seed_station; 
}

char * ConfigVO::getSeedNetwork() const
{
    return (char *)p_seed_network; 
}

char * ConfigVO::getLogDir() const
{
    return (char *)p_logdir; 
}

char * ConfigVO::getLogType() const
{
    return (char *)p_logtype; 
}

char * ConfigVO::getQ660UdpAddr() const
{
    return (char *)p_q660_udpaddr; 
}

uint32_t ConfigVO::getQ660BasePort() const
{
    return p_q660_base_port;
}

uint32_t ConfigVO::getQ660Priority() const
{
    return p_q660_priority;
}

uint64_t ConfigVO::getQ660SerialNumber() const
{
    return p_q660_serial_number;
}

char * ConfigVO::getQ660Password() const
{
    return (char *)p_q660_password;
}

char* ConfigVO::getLockFile() const
{
    return (char*)p_lockfile;
}

char* ConfigVO::getStartMsg() const
{
    return (char*)p_startmsg;
}

uint32_t ConfigVO::getStatusInterval() const
{
    return p_statusinterval;
}

uint32_t ConfigVO::getVerbosity() const
{
    return p_verbosity;
}

uint32_t ConfigVO::getDiagnostic() const
{
    return p_diagnostic;
}

uint32_t ConfigVO::getLogLevel() const {
    return p_logLevel;
}

uint16_t ConfigVO::getFailedRegistrationsBeforeSleep() const {
    return p_failedRegistrationsBeforeSleep;
}

uint16_t ConfigVO::getMinutesToSleepBeforeRetry() const {
    return p_failedRegistrationsBeforeSleep;
}

uint16_t ConfigVO::getDutyCycle_MaxConnectTime() const {
    return p_dutycycle_maxConnectTime;
}

uint16_t ConfigVO::getDutyCycle_SleepTime() const {
    return p_dutycycle_sleepTime;
}

uint16_t ConfigVO::getMulticastEnabled() const {
    return p_multicast_enabled;
}

uint16_t ConfigVO::getMulticastPort() const {
    return p_multicast_port;
}

char *ConfigVO::getMulticastHost() const {
    return (char *)p_multicast_host;
}

char *ConfigVO::getMulticastChannelList() const {
    return (char *)p_multicast_channellist;
}

char * ConfigVO::getContFileDir() const {
    return (char *)p_contFileDir;
}

uint32_t ConfigVO::getLimitBackfill() const {
    return p_limitBackfill;
}

uint32_t ConfigVO::getWaitForClients() const {
    return p_waitForClients;
}

uint32_t ConfigVO::getOptThrottleKbitpersec() const {
    return p_opt_throttle_kbitpersec;
}

uint32_t ConfigVO::getOptBwfillKbitTarget() const {
    return p_opt_bwfill_kbit_target;
}

uint32_t ConfigVO::getOptBwfillProbeInterval() const {
    return p_opt_bwfill_probe_interval;
}

uint32_t ConfigVO::getOptBwfillExceedTrigger() const {
    return p_opt_bwfill_exceed_trigger;
}

uint32_t ConfigVO::getOptBwfillIncreaseInterval() const {
    return p_opt_bwfill_increase_interval;
}

uint32_t ConfigVO::getOptBwfillMaxLatency() const {
    return p_opt_bwfill_max_latency;
}

//**********************************************************************

//
// Set values from string.
// 

void ConfigVO::setServerName(char* input)
{
    strcpy(p_server_name,input);
}

void ConfigVO::setServerDesc(char* input)
{
    strcpy(p_server_desc,input);
}

void ConfigVO::setServerDir(char* input)
{
    strcpy(p_server_dir,input);
}

void ConfigVO::setServerSource(char* input)
{
    strcpy(p_server_source,input);
}

void ConfigVO::setSeedStation(char* input)
{
    strcpy(p_seed_station,input);
}

void ConfigVO::setSeedNetwork(char* input)
{
    strcpy(p_seed_network,input);
}

void ConfigVO::setLogDir(char* input)
{
    strcpy(p_logdir, input);
}
 
void ConfigVO::setLogType(char* input)
{
    strcpy(p_logtype, input);
}
 
void ConfigVO::setQ660UdpAddr(char* input)
{
    strcpy(p_q660_udpaddr, input);
}
 
void ConfigVO::setQ660BasePort(char* input)
{
    uint16_t port = atoi(input);
    if(port <= 0)
    {
	g_log << "xxx Error converting input to Q660 base port number : " <<  
	    port << std::endl;
    }
    else
    {
	p_q660_base_port = port;
    }
}

void ConfigVO::setQ660Priority(char* input)
{
    uint16_t priority = atoi(input);
    if(priority <= 0)
    {
	g_log << "xxx Error converting input to Q660 priority number : " <<  
	    priority << std::endl;
    }
    else
    {
	p_q660_priority = priority;
    }
}
  
void ConfigVO::setQ660SerialNumber(char* input)
{
    uint64_t sn;
    errno = 0;
    sn = strtoull(input,0,0);
    if(sn <= 0 || errno != 0)
    {
	g_log << "xxx Error converting input to serial number : input = "
	      << input << " output = " << sn << std::endl;
    }
    else
    {
	p_q660_serial_number = sn;
    }
}

void ConfigVO::setQ660Password(char* input)
{
    strcpy(p_q660_password, input);
}
  
void ConfigVO::setLockFile(char* input)
{
    strlcat(p_lockfile,input,sizeof(p_lockfile)); // always null terminates the string
}

void ConfigVO::setStartMsg(char* input)
{
    char msg[256];
    sprintf(msg,"Starting %s %s", APP_IDENT_STRING, input);
    strlcat(p_startmsg,input,sizeof(p_startmsg)); // always null terminates the string
}

void ConfigVO::setStatusInterval(char* input)
{
    int len = strlen (input);
    if(len > 0)
    {
	uint32_t interval = atoi(input);
	if(interval < MIN_STATUS_INTERVAL)
	{
	    g_log << 
		"+++ StatusInterval in station.ini is too short." << std::endl;
	    g_log << "+++ Minimum StatusInterval is: " 
		  << MIN_STATUS_INTERVAL << " seconds. " << std::endl;
	    g_log << "+++ Setting StatusInterval to " 
		  << MIN_STATUS_INTERVAL
		  << " rather than : " << interval << std::endl;
	    interval = MIN_STATUS_INTERVAL;
	}
	else if(interval > MAX_STATUS_INTERVAL)
	{
	    g_log << 
		"+++ StatusInterval in station.ini is too long." << std::endl;
	    g_log << "+++ Maximum StatusInterval is: " 
		  << MAX_STATUS_INTERVAL << " seconds. " << std::endl;
	    g_log << "+++ Setting StatusInterval to " << MAX_STATUS_INTERVAL << 
		" rather than : " << interval << std::endl;
	    interval = MAX_STATUS_INTERVAL;
	}
	g_log << "+++ Setting StatusInterval to: " << interval 
	      << std::endl;
	p_statusinterval = interval;
    }
    else
    {
	g_log << "+++ Setting StatusInterval to default value: "
	      << DEFAULT_STATUS_INTERVAL 
	      << std::endl;
	p_statusinterval = DEFAULT_STATUS_INTERVAL;
    }
}
  
void ConfigVO::setVerbosity(char* input)
{
    int len = strlen (input);
    if(len > 0)
    {
	int32_t level = atoi(input);

	if(level > D_EVERYTHING)
	{
	    p_verbosity = D_EVERYTHING;
	}
	else
	{
	    p_verbosity = level;
	}
    }
    else
    {
	p_verbosity = D_SILENT;
    }
}

void ConfigVO::setDiagnostic(char* input)
{
    int len = strlen (input);
    if(len > 0)
    {
	uint32_t target = atoi(input);
	p_diagnostic = target;
    }
    else
    {
	p_diagnostic = D_NO_TARGET;
    }
}

void ConfigVO::setLogLevel(char *input) {
    int logLevel = 0;
    char *tok;
    char localInput[255];
    strcpy(localInput, input);

    tok = strtok(localInput, ", ");

    while(tok != NULL) {
	if(!strncasecmp("SD", tok, 2)) {
	    logLevel |= VERB_SDUMP;
	}
	if(!strncasecmp("RM", tok, 2)) {
	    logLevel |= VERB_REGMSG;
	}
	if(!strncasecmp("VB", tok, 2)) {
	    logLevel |= VERB_LOGEXTRA;
	}
	if(!strncasecmp("SM", tok, 2)) {
	    logLevel |= VERB_AUXMSG;
	}
	if(!strncasecmp("PD", tok, 2)) {
	    logLevel |= VERB_PACKET;
	}
	tok = strtok(NULL, ", ");
    }
  
    p_logLevel = logLevel;
}

void ConfigVO::setFailedRegistrationsBeforeSleep(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_failedRegistrationsBeforeSleep = 0;
	return;
    }
    p_failedRegistrationsBeforeSleep = atoi(input);
    if(p_failedRegistrationsBeforeSleep <= 0) {
	g_log << "xxx Error converting input to num registrations : " << input << std::endl;
    }
}

void ConfigVO::setMinutesToSleepBeforeRetry(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_minutesToSleepBeforeRetry = 0;
	return;
    }
    p_minutesToSleepBeforeRetry = atoi(input);
    if(p_minutesToSleepBeforeRetry <= 0) {
	g_log << "xxx Error converting input to num minutes : " << input << std::endl;
    }
}

void ConfigVO::setDutyCycle_MaxConnectTime(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_dutycycle_maxConnectTime = 0;
	return;
    }
    p_dutycycle_maxConnectTime = atoi(input);
    if(p_dutycycle_maxConnectTime <= 0) {
	g_log << "xxx Error converting input to num minutes : " << input << std::endl;
    }
}

void ConfigVO::setDutyCycle_SleepTime(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_dutycycle_sleepTime = 0;
	return;
    }
    p_dutycycle_sleepTime = atoi(input);
    if(p_dutycycle_sleepTime <= 0) {
	g_log << "xxx Error converting input to num minutes : " << input << std::endl;
    }
}

void ConfigVO::setMulticastEnabled(char *input) {
    if(  !strcasecmp(input, "yes") ||
	 !strcasecmp(input, "1") || 
	 !strcasecmp(input, "true") ) {
	p_multicast_enabled = 1;
    } else {
	p_multicast_enabled = 0;
    }
}

void ConfigVO::setMulticastPort(char *input) {
    int port = atoi(input);
    if(port < 0) {
	p_multicast_port = 0;
    } else {
	p_multicast_port = port;
    }
}

void ConfigVO::setMulticastHost(char *input) {
    strncpy(p_multicast_host, input, 255);
}

void ConfigVO::setMulticastChannelList(char *input) {
    memset (p_multicast_channellist, 0, sizeof(p_multicast_channellist));
    strncpy(p_multicast_channellist, input, sizeof(p_multicast_channellist)-1);
}

void ConfigVO::setContFileDir(char *input) {
    strncpy(p_contFileDir, input, 255);
}

void ConfigVO::setLimitBackfill(char* input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_limitBackfill = 0;
	return;
    }
    p_limitBackfill = atoi(input);
    if (p_limitBackfill <= 0)
    {
	g_log << "xxx Error converting input to Q660 limitBackfill number : " << input << std::endl;
	p_limitBackfill = 0;
    }
}

void ConfigVO::setWaitForClients(char* input)
{
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_waitForClients = 0;
	return;
    }
    p_waitForClients = atoi(input);
    if (p_waitForClients <= 0)
    {
	g_log << "xxx Error converting input to Q660 waitForClients number : " << input << std::endl;
	p_waitForClients = 0;
    }
}

void ConfigVO::setOptThrottleKbitpersec(char* input)
{
    uint32_t val;
    errno = 0;
    val = strtoul(input,0,0);
    if(val <= 0 || errno != 0)
    {
	g_log << "xxx Error converting input to optBwfillTrottleKbitpersec : input = "
	      << input << " output = " << val << std::endl;
    }
    else
    {
	p_opt_throttle_kbitpersec = val;
    }
}

void ConfigVO::setOptBwfillKbitTarget(char* input)
{
    uint32_t val;
    errno = 0;
    val = strtoul(input,0,0);
    if(val <= 0 || errno != 0)
    {
	g_log << "xxx Error converting input to optBwfillKbitTarget : input = "
	      << input << " output = " << val << std::endl;
    }
    else
    {
	p_opt_bwfill_kbit_target = val;
    }
}

void ConfigVO::setOptBwfillProbeInterval(char* input)
{
    uint32_t val;
    errno = 0;
    val = strtoul(input,0,0);
    if(val <= 0)
    {
	g_log << "xxx Error converting input to optBwfillProbeInterval : input = "
	      << input << " output = " << val << std::endl;
    }
    else
    {
	p_opt_bwfill_probe_interval = val;
    }
}

void ConfigVO::setOptBwfillExceedTrigger(char* input)
{
    uint32_t val;
    errno = 0;
    val = strtoul(input,0,0);
    if(val <= 0 || errno != 0)
    {
	g_log << "xxx Error converting input to optBwfillMaxLatency : input = "
	      << input << " output = " << val << std::endl;
    }
    else
    {
	p_opt_bwfill_exceed_trigger = val;
    }
}

void ConfigVO::setOptBwfillIncreaseInterval(char* input)
{
    uint32_t val;
    errno = 0;
    val = strtoul(input,0,0);
    if(val <= 0 || errno != 0)
    {
	g_log << "xxx Error converting input to optBwfillIncreaseInterval : input = "
	      << input << " output = " << val << std::endl;
    }
    else
    {
	p_opt_bwfill_increase_interval = val;
    }
}

void ConfigVO::setOptBwfillMaxLatency(char* input)
{
    uint32_t val;
    errno = 0;
    val = strtoul(input,0,0);
    if(val <= 0 || errno != 0)
    {
	g_log << "xxx Error converting input to optBwfillMaxLatency : input = "
	      << input << " output = " << val << std::endl;
    }
    else
    {
	p_opt_bwfill_max_latency = val;
    }
}
//**********************************************************************

//
// Set values from numeric.
// 

void ConfigVO::setQ660BasePort(uint32_t a)
{
    p_q660_base_port = a;
}

void ConfigVO::setQ660Priority(uint32_t a)
{
    p_q660_priority = a;
}
  
void ConfigVO::setQ660SerialNumber(uint64_t a)
{
    p_q660_serial_number = a;
}

void ConfigVO::setStatusInterval(uint32_t a) 
{
    p_statusinterval = a;
}
void ConfigVO::setVerbosity(uint32_t a)
{
    p_verbosity = a;
}

void ConfigVO::setDiagnostic(uint32_t a) 
{
    p_diagnostic = a;
}

void ConfigVO::setWaitForClients(uint32_t a) 
{
    p_waitForClients = a;
}

void ConfigVO::setOptThrottleKbitpersec(uint32_t a)
{
    p_opt_throttle_kbitpersec = a;
}

void ConfigVO::setOptBwfillKbitTarget(uint32_t a)
{
    p_opt_bwfill_kbit_target = a;
}

void ConfigVO::setOptBwfillProbeInterval(uint32_t a)
{
	p_opt_bwfill_probe_interval = a;
}

void ConfigVO::setOptBwfillExceedTrigger(uint32_t a)
{
    p_opt_bwfill_exceed_trigger = a;
}

void ConfigVO::setOptBwfillIncreaseInterval(uint32_t a)
{
	p_opt_bwfill_increase_interval = a;
}

void ConfigVO::setOptBwfillMaxLatency(uint32_t a)
{
    p_opt_bwfill_max_latency = a;
}
