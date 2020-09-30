// 
// ======================================================================
// Copyright (C) 2000-2003 Instrumental Software Technologies, Inc. (ISTI)
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. If modifications are performed to this code, please enter your own 
// copyright, name and organization after that of ISTI.
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in
// the documentation and/or other materials provided with the
// distribution.
// 3. All advertising materials mentioning features or use of this
// software must display the following acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
// product, or is used in other commercial software products the
// customer must be informed that "This product includes software
// developed by Instrumental Software Technologies, Inc.
// (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
// must not be used to endorse or promote products derived from
// this software without prior written permission. For written
// permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
// nor may "ISTI" appear in their names without prior written
// permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
// acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com/)."
// 8. Redistributions of source code, or portions of this source code,
// must retain the above copyright notice, this list of conditions
// and the following disclaimer.
// THIS SOFTWARE IS PROVIDED BY INSTRUMENTAL SOFTWARE
// TECHNOLOGIES, INC. "AS IS" AND ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL INSTRUMENTAL SOFTWARE TECHNOLOGIES,
// INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
// 29 Sep 2020 DSN Updated for comserv3.
// 

#include <iostream>
#include <cstdlib>
#include "Logger.h"
#include "stdio.h"
#include "string.h"
#include "qservdefs.h"
#include "portingtools.h"

Logger::Logger() {
    //clearBuffer();
    stdoutLogging = false;
    fileLogging = false;
    coutLogging = true;	// default logging until other logging method set.
    memset (&this->logger_mutex, 0, sizeof(pthread_mutex_t));
    int rc = pthread_mutex_init (&logger_mutex, NULL);
    if (rc != 0) {
	fprintf (stderr, "Exit: Error %d initialing logger logger_mutex\n", rc);
	exit (rc);
    }
}

Logger::~Logger() {
    if(strlen(logBuff.str().c_str()) != 0) {
        endEntry();
    }
    int rc = pthread_mutex_destroy (&logger_mutex);
    if (rc != 0) {
	fprintf (stderr, "Exit: Error %d destroying logger logger_mutex\n", rc);
	exit (rc);
    }
}


void Logger::logToStdout(bool val) {
    stdoutLogging = val;
    if (val) coutLogging = false;
}

void Logger::logToFile(bool val) {
    fileLogging = val;
    if (val) coutLogging = false;
}

// Logger Operators.

Logger& Logger::operator<<(const char *val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << (const char *) val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(char *val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << (char *) val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(char val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(int8_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(uint8_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(int16_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(uint16_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(int32_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(uint32_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(void * val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(int64_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(uint64_t val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(float val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(double val) {
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << val;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

// the following are needed for rendering of std::endl
Logger& Logger::operator<<(std::ostream& (*f)(std::ostream&)){
    // we'll consider an endl a plea for an endEntry()
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << f;
    if(logBuff.str().c_str()[strlen(logBuff.str().c_str())-1] == '\n') {
	endEntry();
    }
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(std::ios& (*f)(std::ios&)){
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << f;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::operator<<(std::ios_base& (*f)(std::ios_base&)){
    int rc = pthread_mutex_lock (&logger_mutex);
    logBuff << f;
    rc = pthread_mutex_unlock (&logger_mutex);
    if (rc) {}	// suppress compiler warning
    return *this;
}

Logger& Logger::endEntry() {
    if(coutLogging) {
	std::cout << logBuff.str();
    }
    if(stdoutLogging) {
	LogMessage(CS_LOG_TYPE_INFO, "%s", (char *)logBuff.str().c_str());
    }
    if(fileLogging) {
	LogMessage(CS_LOG_TYPE_INFO, "%s", (char *)logBuff.str().c_str());
    }
    clearBuffer();
    return *this;
}

void Logger::clearBuffer() {
    logBuff.str("");
}


#ifdef _TEST_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char **argv) {
    Logger log;
    struct in_addr in;
    log.logToStdout(true);
    log << "Configured to send packets to " <<
	inet_ntoa(in) << " on port " <<
//	int rc = pthread_mutex_lock (&logger_mutex);
	std::hex <<  5330 << std::endl;
//	rc = pthread_mutex_unlock (&logger_mutex);
}
#endif
