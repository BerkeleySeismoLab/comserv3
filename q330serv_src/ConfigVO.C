/*
 * File: ConfigVO.C
 *
 * Purpose : This class encapsulates the configuration information that
 * is read from a q330serv configuration file. It has constructors for both
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
 *  2022-03-16 DSN Added support for TCP connection to Q330/baler.
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
#include "q330Diag.h"
#include "global.h"
#include "portingtools.h"

ConfigVO::ConfigVO(q330serv_cfg cfg) {
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
    setQ330UdpAddr(cfg.udpaddr);
    setQ330TcpAddr(cfg.tcpaddr);
    setQ330BasePort(cfg.baseport);
    setQ330DataPortNumber(cfg.dataport);
    setQ330SerialNumber(cfg.serialnumber);
    setQ330AuthCode(cfg.authcode);
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
    setDutyCycle_BufferLevel(cfg.dutycycle_bufferLevel);
    setMulticastEnabled(cfg.multicastEnabled);
    setMulticastPort(cfg.multicastPort);
    setMulticastHost(cfg.multicastHost);
    setMulticastChannelList(cfg.multicastChannelList);
    setContFileDir(cfg.contFileDir);
    setWaitForClients(cfg.waitForClients);
    setPacketQueueSize(cfg.packetQueueSize);

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
    strcpy(p_q330_udpaddr, "");
    strcpy(p_q330_tcpaddr, "");
    p_q330_base_port = 5330;
    p_q330_data_port = 1;
    p_q330_serial_number = 0x00;
    p_q330_auth_code = 0x00;
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
    p_dutycycle_bufferLevel = 0;
    p_multicast_enabled = 0;
    p_multicast_port = 0;
    strcpy(p_multicast_host, "");
    memset(p_multicast_channellist, 0, sizeof(p_multicast_channellist));
    strcpy(p_contFileDir, "");
    p_waitForClients = 0;
    p_packetQueueSize = DEFAULT_PACKETQUEUE_QUEUE_SIZE;

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

char * ConfigVO::getQ330UdpAddr() const
{
    return (char *)p_q330_udpaddr; 
}

char * ConfigVO::getQ330TcpAddr() const
{
    return (char *)p_q330_tcpaddr; 
}

uint32_t ConfigVO::getQ330BasePort() const
{
    return p_q330_base_port;
}

uint32_t ConfigVO::getQ330DataPortNumber() const
{
    return p_q330_data_port;
}

uint64_t ConfigVO::getQ330SerialNumber() const
{
    return p_q330_serial_number;
}

uint64_t ConfigVO::getQ330AuthCode() const
{
    return p_q330_auth_code;
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

uint16_t ConfigVO::getDutyCycle_BufferLevel() const {
    return p_dutycycle_bufferLevel;
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

uint32_t ConfigVO::getWaitForClients() const {
    return p_waitForClients;
}

uint32_t ConfigVO::getPacketQueueSize() const {
    return p_packetQueueSize;
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
 
void ConfigVO::setQ330UdpAddr(char* input)
{

    strcpy(p_q330_udpaddr, input);
}
void ConfigVO::setQ330TcpAddr(char* input)
{

    strcpy(p_q330_tcpaddr, input);
}
 
void ConfigVO::setQ330BasePort(char* input)
{
  uint16_t port = atoi(input);
  if(port <= 0)
  {
    g_log << "xxx Error converting input to Q330 base port number : input = " <<  
	input << " output = " << port << std::endl;
  }
  else
  {
    p_q330_base_port = port;
  }
}

void ConfigVO::setQ330DataPortNumber(char* input)
{
    uint16_t port = atoi(input);
    if(port <= 0)
    {
	g_log << "xxx Error converting input to Q330 data port number : input = " 
	      << input << " output = " << port << std::endl;
    }
    else
    {
	p_q330_data_port = port;
    }
}
  
void ConfigVO::setQ330SerialNumber(char* input)
{
    uint64_t sn = strtoull(input,0,0);
    if(sn <= 0)
    {
	g_log << "xxx Error converting input to serial number : input = "
	      << input << " output = " << sn << std::endl;
    }
    else
    {
	p_q330_serial_number = sn;
    }
}

void ConfigVO::setQ330AuthCode(char* input)
{
    uint64_t code = strtoull(input,0,0);
    if(code < 0)
    {
	g_log << "xxx Error converting input to auth code : input = " 
	      << input << " output = " << code << std::endl;
    }
    else
    {
	p_q330_auth_code = code;
    }
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
	    g_log << "+++ " << APP_IDENT_STRING << " minimum StatusInterval is: " 
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
	    g_log << "+++ " << APP_IDENT_STRING << " maximum StatusInterval is: " 
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
	if(!strncasecmp("CR", tok, 2)) {
	    logLevel |= VERB_RETRY;
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
	g_log << "xxx Error converting input to failedRegistrationsBeforeSleep : input = "
	      << input << " output = " << p_failedRegistrationsBeforeSleep << std::endl;
    }
}

void ConfigVO::setMinutesToSleepBeforeRetry(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_minutesToSleepBeforeRetry = 0;
	return;
    }
    p_minutesToSleepBeforeRetry = atoi(input);
    if(p_minutesToSleepBeforeRetry <= 0) {
	g_log << "xxx Error converting input to minutesToSleepBeforeRetry : input = "
	      << input << " output = " << p_minutesToSleepBeforeRetry << std::endl;
    }
}

void ConfigVO::setDutyCycle_MaxConnectTime(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_dutycycle_maxConnectTime = 0;
	return;
    }
    p_dutycycle_maxConnectTime = atoi(input);
    if(p_dutycycle_maxConnectTime <= 0) {
	g_log << "xxx Error converting input to dutycycle_maxConnectTime : input = " 
	      << input << " output = " << p_dutycycle_maxConnectTime << std::endl;
    }
}

void ConfigVO::setDutyCycle_SleepTime(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_dutycycle_sleepTime = 0;
	return;
    }
    p_dutycycle_sleepTime = atoi(input);
    if(p_dutycycle_sleepTime <= 0) {
	g_log << "xxx Error converting input to dutycycle_sleepTime : input = " 
	      << input << " output = " << p_dutycycle_sleepTime << std::endl;
    }
}

void ConfigVO::setDutyCycle_BufferLevel(char *input) {
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_dutycycle_bufferLevel = 0;
	return;
    }
    p_dutycycle_bufferLevel = atoi(input);
    if(p_dutycycle_bufferLevel <= 0) {
	g_log << "xxx Error converting input to dutycycle_bufferLevel : input = " 
	      << input << " output = " << p_dutycycle_bufferLevel << std::endl;
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
    strcpy(this->p_contFileDir, input);
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
	g_log << "xxx Error converting input to Q330 waitForClients number : " << input << std::endl;
	p_waitForClients = 0;
    }
}

void ConfigVO::setPacketQueueSize(char* input)
{
    if(!strcmp(input, "") || !strcmp(input, "0")) {
	p_packetQueueSize = DEFAULT_PACKETQUEUE_QUEUE_SIZE;
	return;
    }
    p_packetQueueSize = atoi(input);
    if (p_packetQueueSize <= 0)
    {
	g_log << "xxx Error converting input to Q330 packetQueueSize number : " << input << std::endl;
	p_packetQueueSize = 0;
    }
}

//**********************************************************************

//
// Set values from numeric.
// 

void ConfigVO::setQ330BasePort(uint32_t a)
{
    p_q330_base_port = a;
}

void ConfigVO::setQ330DataPortNumber(uint32_t a)
{
    p_q330_data_port = a;
}
  
void ConfigVO::setQ330SerialNumber(uint64_t a)
{
    p_q330_serial_number = a;
}

void ConfigVO::setQ330AuthCode(uint64_t a)
{
    p_q330_auth_code = a;
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

void ConfigVO::setPacketQueueSize(uint32_t a) 
{
    p_packetQueueSize = a;
}
