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
 * 2020-09-29 DSN Updated for comserv3.
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
#include "mservDiag.h"
#include "portingtools.h"
#include "global.h"

ConfigVO::ConfigVO(mserv_cfg cfg) {
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
    setMcastIf(cfg.mcastif);
    setUdpAddr(cfg.udpaddr);
    setIPPort(cfg.ipport);
    setLockFile(cfg.lockfile);
    setStartMsg(cfg.startmsg);
    setStatusInterval(cfg.statusinterval);
    setVerbosity(cfg.verbosity);
    setDiagnostic(cfg.diagnostic);
    setLogLevel(cfg.loglevel);
    setContFileDir(cfg.contFileDir);
    setWaitForClients(cfg.waitForClients);

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
    memset(p_mcastif, 0, sizeof(p_mcastif));
    memset(p_udpaddr, 0, sizeof(p_udpaddr));
    p_ipport = 0;
    memset(p_lockfile, 0, sizeof(p_lockfile));
    memset(p_startmsg, 0, sizeof(p_startmsg));
    p_statusinterval = 0;
    p_verbosity = D_SILENT;
    p_diagnostic = D_NO_TARGET;
    p_logLevel = 0;
    memset(p_contFileDir, 0, sizeof(p_contFileDir));
    p_waitForClients = 0;

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

char * ConfigVO::getMcastIf() const
{
    return (char *)p_mcastif; 
}

char * ConfigVO::getUdpAddr() const
{
    return (char *)p_udpaddr; 
}

uint32_t ConfigVO::getIPPort() const
{
    return (p_ipport);
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

char* ConfigVO::getContFileDir() const
{
    return (char*)p_contFileDir;
}

uint32_t ConfigVO::getWaitForClients() const {
    return p_waitForClients;
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
 
void ConfigVO::setMcastIf(char* input)
{
    strcpy(p_mcastif, input);
}
 
void ConfigVO::setUdpAddr(char* input)
{
    strcpy(p_udpaddr, input);
}
 
void ConfigVO::setIPPort(char* input)
{
    uint32_t port = atoi(input);
    if (port <= 0) 
    {
        g_log << "xxx Error converting input to ipport number : " <<
            port << std::endl;
    }
    else
    {
        p_ipport = port;
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

void ConfigVO::setContFileDir(char *input) {
    strncpy(p_contFileDir, input, 255);
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
        g_log << "xxx Error converting input to waitForClients number : " << input << std::endl;
        p_waitForClients = 0;
    }
}

//**********************************************************************

//
// Set values from numeric.
// 

void ConfigVO::setIPPort(uint32_t a)
{
    p_ipport = a;
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

void ConfigVO::setLogLevel(uint32_t a) 
{
    p_diagnostic = a;
}
