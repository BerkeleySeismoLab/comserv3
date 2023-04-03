/* 
 * Lib660Interface class
 *
 * Modification History:
 *  2020-04-08 DSN Initial coding derived from lib330interface.C
 *  2020-09-29 DSN Updated for comserv3.
 *  2021-03-13 DSN Fixed creating and matching multicast channel+location list.
 */

#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <fnmatch.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "global.h"
#include "comserv_queue.h"
#include <linux/limits.h>
#include "lib660Interface.h"
#include "portingtools.h"

#define MAXWAITLIBSHUTDOWN      10

//: #define DEBUG_Lib660Interface
//: #define DEBUG_MULTICAST
//: #define DEBUG_FILE_CALLBACK	1
//: #define DEBUG_PQUEUE

static int throttle_free_packet_threshold;
static int unthrottle_free_packet_threshold;
static int min_free_packet_threshold;

// Static variables from Lib660Interface needed in static callback functions.
double Lib660Interface::timestampOfLastRecord = 0;
int Lib660Interface::num_multicastChannelEntries = 0;
multicastChannelEntry Lib660Interface::multicastChannelList[MAX_MULTICASTCHANNELENTRIES];
std::map<std::string, int> Lib660Interface::lowlatencymap;
std::map<std::string,int>::iterator Lib660Interface::it;


Lib660Interface::Lib660Interface(char *stationName, ConfigVO ourConfig) {
    pmodules mods;
    tmodule *mod;
    int x;

    g_log << "+++ lib660 Interface created" << std::endl;
    this->currentLibState = LIBSTATE_IDLE;
    this->initializeCreationInfo(stationName, ourConfig);
    this->initializeRegistrationInfo(ourConfig);

    mods = lib_get_modules();
    g_log << "+++ Lib660 Modules:" << std::endl;
    for(x=0; x <= MAX_MODULES - 1; x++) {
	mod = &(*mods)[x];
	if(!mod->name[0]) {
	    continue;
	}
	if( !(x % 5)) {
	    if(x > 0) {
		g_log << std::endl;
	    }
	    if(x < MAX_MODULES-1) {
		g_log << "+++ ";
	    }
	}
	g_log << (char *) mod->name << ":" << mod->ver << " ";
    }
    g_log << std::endl;
    g_log << "+++ Initializing station thread" << std::endl;

    g_log << "+++ IP info:" << std::endl;  
    g_log << "+++ IP Addr:" << this->registrationInfo.q660id_address << std::endl;
    log_q8serv_config(ourConfig);
    if(ourConfig.getMulticastEnabled()) {
	g_log << "+++ Multicast Enabled:" << std::endl;
	if( (mcastSocketFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	    ourConfig.setMulticastEnabled((char *)"0");
	    g_log << "XXX Unable to create multicast socket: errno=" << errno << " (" << strerror(errno) << ")"<< std::endl;
	}
    
	memset(&(mcastAddr), 0, sizeof(mcastAddr));
	mcastAddr.sin_family = AF_INET;
	mcastAddr.sin_addr.s_addr = inet_addr(ourConfig.getMulticastHost());
	mcastAddr.sin_port = htons(ourConfig.getMulticastPort());
	this->build_multicastChannelList((char *)ourConfig.getMulticastChannelList());
	g_log << "+++    Multicast IP: " << ourConfig.getMulticastHost() << std::endl;
	g_log << "+++    Multicast Port: " << ourConfig.getMulticastPort() << std::endl;
	g_log << "+++    Multicast Channels:" << std::endl;
	for(int i=0; i < num_multicastChannelEntries; i++) {
	    g_log << "+++        " << 
		(char *)multicastChannelList[i].channel << "." <<
		(char *)multicastChannelList[i].location << std::endl;
	}
    }

    // Allocate intermediate PacketQueue if needed.  
    // Reuse existing PacketQueue if one was previously created.
    int npackets = ourConfig.getPacketQueueSize();
    if (npackets <= 0) npackets = DEFAULT_PACKETQUEUE_QUEUE_SIZE;
    if (g_packetQueue == NULL) {
	packetQueue = new PacketQueue(npackets);
	g_packetQueue = packetQueue;
	if (g_packetQueue != NULL) {
	    g_log << "+++ Created intermediate PacketQueue of " << npackets << " packets"<< std::endl;
	}
	else {
	    g_log << "+++ ERROR - unable to created intermediate PacketQueue of " << npackets << " packets"<< std::endl;
	}
    }
    else {
	g_log << "+++ Using existing intermediate PacketQueue of " << npackets << " packets"<< std::endl;
    }

    // Set thresholds for the intermediate PacketQueue.
    throttle_free_packet_threshold = (0.2 * npackets);
    unthrottle_free_packet_threshold = (0.8 * npackets);
    min_free_packet_threshold = (0.1 * npackets);

    lib_create_context(&(this->stationContext), &(this->creationInfo));
    if(this->creationInfo.resp_err == LIBERR_NOERR) {
	g_log << "+++ Station context created: address is " << (void *)this->stationContext << std::endl;
	g_stationContext = this->stationContext;	//:: DEBUG
    } else {
	this->handleError(creationInfo.resp_err);
    }
}


Lib660Interface::~Lib660Interface() {
    enum tliberr errcode;
    topstat opstat;
    int countdown;

    g_log << "+++ Cleaning up lib660 Interface" << std::endl;

    // Stop the thread.
    this->startDeregistration();
    countdown = MAXWAITLIBSHUTDOWN;

    while(this->stationContext && countdown--) {
	this->currentLibState = lib_get_state(this->stationContext, &errcode, &opstat);
	// wait for IDLE state
	if (errcode == LIBERR_INVCTX) {
	    // library already closed somehow
	    break;
	}
	if (this->currentLibState == LIBSTATE_IDLE || this->currentLibState == LIBSTATE_TERM) {
	    // proceed to TERM
	    break;
	}
	sleep(1);
    }

    this->changeState(LIBSTATE_TERM, LIBERR_CLOSED);
    countdown = MAXWAITLIBSHUTDOWN;

    while (this->stationContext && countdown--) {
	// wait for TERM state
	this->currentLibState = lib_get_state(this->stationContext, &errcode, &opstat);
	if (errcode == LIBERR_INVCTX) {
	    // library already closed somehow
	    break;
	}
	if (this->currentLibState == LIBSTATE_TERM) {
	    // proceed to shutdown
	    break;
	}
	sleep(1);
    }

    if (mcastSocketFD >= 0) {
	g_log << "+++ Multicast socket close" << std::endl;
	int err = close(mcastSocketFD);
	if (err != 0) g_log << "XXX Error closing multicast socket: errno=" << errno << " (" << strerror(errno) << ")"<< std::endl;
	mcastSocketFD = -1;
    }
    errcode = lib_destroy_context(&(this->stationContext));
    if(errcode != LIBERR_NOERR) {
	this->handleError(errcode);
    }
    g_stationContext = NULL;	//:: DEBUG
    g_log << "+++ lib660 Interface destroyed" << std::endl;
}


void Lib660Interface::initializeRegistrationInfo(ConfigVO ourConfig) {
    // First zero the registrationInfo structure.
    memset (&this->registrationInfo, 0, sizeof(this->registrationInfo));
    strcpy(this->registrationInfo.q660id_pass, ourConfig.getQ660Password());
    strcpy(this->registrationInfo.q660id_address, ourConfig.getQ660TcpAddr());
    this->registrationInfo.q660id_baseport = ourConfig.getQ660BasePort();
    /* Start of new items in lib660.  Possibly configurable items or change. */
    this->registrationInfo.spare1 = FALSE;
    this->registrationInfo.prefer_ipv4 = TRUE;
    this->registrationInfo.start_newer = TRUE;
    /* End of new items in lib660.  Possibly configurable items. */
    this->registrationInfo.opt_dynamic_ip = 0;
    this->registrationInfo.opt_hibertime = ourConfig.getMinutesToSleepBeforeRetry();
    this->registrationInfo.opt_conntime = ourConfig.getDutyCycle_MaxConnectTime();
    this->registrationInfo.opt_connwait = ourConfig.getDutyCycle_SleepTime();
    this->registrationInfo.opt_regattempts = ourConfig.getFailedRegistrationsBeforeSleep();
    this->registrationInfo.opt_ipexpire = 0;
    this->registrationInfo.opt_disc_latency = 0;
    this->registrationInfo.opt_q660_cont = 0;
    this->registrationInfo.opt_maxsps = 1000;
    this->registrationInfo.opt_token = 0;
    this->registrationInfo.opt_start = OST_LAST;	//:: DSN -  Make configurable?
    this->registrationInfo.opt_end = 0;
    this->registrationInfo.opt_client_mode = LMODE_BSL;
    this->registrationInfo.opt_limit_last_backfill = ourConfig.getLimitBackfill();
    // Bandwidth control
    this->registrationInfo.opt_throttle_kbitpersec = ourConfig.getOptThrottleKbitpersec();
    this->registrationInfo.opt_bwfill_kbit_target = ourConfig.getOptBwfillKbitTarget();
    this->registrationInfo.opt_bwfill_probe_interval = ourConfig.getOptBwfillProbeInterval();
    this->registrationInfo.opt_bwfill_exceed_trigger = ourConfig.getOptBwfillExceedTrigger();
    this->registrationInfo.opt_bwfill_increase_interval = ourConfig.getOptBwfillIncreaseInterval();
    this->registrationInfo.opt_bwfill_max_latency = ourConfig.getOptBwfillMaxLatency();
}


void Lib660Interface::initializeCreationInfo(char *stationName, ConfigVO ourConfig) {
    // First zero the creationInfo structure.
    memset (&this->creationInfo, 0, sizeof(this->creationInfo));	
    // Fill out the parts of the creationInfo that we know about
    uint64_t serial = ourConfig.getQ660SerialNumber();
  
    memcpy(this->creationInfo.q660id_serial, &serial, sizeof(uint64_t));
    this->creationInfo.q660id_priority = ourConfig.getQ660Priority();

    strcpy(this->creationInfo.host_software, APP_VERSION_STRING);
    strcpy(this->creationInfo.host_ident, APP_IDENT_STRING);

    /* Set private station info used for file callback functions. */
    strcpy (this->station_info.stationName, stationName);
    if(strlen(ourConfig.getContFileDir())) {
	sprintf(this->station_info.conf_path_prefix, "%s/%s.", ourConfig.getContFileDir(), stationName);
    } 
    else {
	sprintf(this->station_info.conf_path_prefix, "%s.", stationName);
    }

    this->fowner.call_fileacc = (tcallback)&this->file_callback;
    this->fowner.station_ptr = (void *)&this->station_info;
    this->creationInfo.file_owner = &this->fowner;

    this->creationInfo.opt_verbose = ourConfig.getLogLevel();
    this->creationInfo.opt_minifilter = OMF_ALL;
    this->creationInfo.opt_aminifilter = OMF_ALL;
    this->creationInfo.amini_exponent = 0;
    this->creationInfo.amini_512highest = 1000;
    this->creationInfo.mini_embed = 0;
    this->creationInfo.mini_separate = 1;
    this->creationInfo.mini_firchain = 0;
    this->creationInfo.call_minidata = this->miniseed_callback;
    this->creationInfo.call_aminidata = NULL;
    this->creationInfo.resp_err = LIBERR_NOERR;
    this->creationInfo.call_state = this->state_callback;
    this->creationInfo.call_messages = this->msg_callback;
    if(ourConfig.getMulticastEnabled()) {
	this->creationInfo.call_secdata = this->onesec_callback;
	this->creationInfo.call_lowlatency = this->lowlatency_callback;
    } else {
	this->creationInfo.call_secdata = NULL;
	this->creationInfo.call_lowlatency = NULL;
    }
}


void Lib660Interface::startRegistration() { 
    enum tliberr errcode;
    g_log << "+++ Starting registration with Q660" << std::endl;
    errcode = lib_register(this->stationContext, &(this->registrationInfo));
    if(errcode != LIBERR_NOERR) {
	this->handleError(errcode);
    }  
}


void Lib660Interface::startDeregistration() {
    g_log << "+++ Starting deregistration from Q660" << std::endl;
    this->changeState(LIBSTATE_IDLE, LIBERR_CLOSED);
}


void Lib660Interface::changeState(enum tlibstate newState, enum tliberr reason) {
    lib_change_state(this->stationContext, newState, reason);
}


void Lib660Interface::startDataFlow() {
    g_log << "+++ Requesting dataflow to start" << std::endl;
    this->changeState(LIBSTATE_RUN, LIBERR_NOERR);
    /* Clear the lowlatency map. */
    g_log << "+++ Clearing lowlantecy map" << std::endl;
    lowlatencymap.clear();
}


void Lib660Interface::flushData() {
    enum tliberr errcode;
    g_log << "+++ Requesting flush data queues" << std::endl;
    errcode = lib_flush_data(this->stationContext);
    if(errcode != LIBERR_NOERR) {
	this->handleError(errcode);
    }  
}


int Lib660Interface::waitForState(enum tlibstate waitFor, int maxSecondsToWait, void(*funcToCallWhileWaiting)()) {
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 250000000;
    for(int i=0; i < (maxSecondsToWait*4); i++) {
	if(this->getLibState() != waitFor) {
	    nanosleep(&t, NULL);
	    funcToCallWhileWaiting();
	} else {
	    return 1;
	}
    }
    return 0;
}


void Lib660Interface::displayStatusUpdate() {
    enum tlibstate currentState;
    enum tliberr lastError;
    topstat libStatus;
    time_t rightNow = time(NULL);
    char timeString[80];
    int i;

    currentState = lib_get_state(this->stationContext, &lastError, &libStatus);

    // do some internal maintenence if required (this should NEVER happen)
    if(currentState != getLibState()) {
	string63 newStateName;
	g_log << "XXX Current lib660 state mismatch.  Fixing..." << std::endl;
	lib_get_statestr(currentState, (pchar)&newStateName);
	g_log << "+++ State change to '" << newStateName << "'" << std::endl;
	this->setLibState(currentState);
    }

    // version and localtime
    // Trim newline from the ctime string - interferes with logging function.
    ctime_r(&rightNow,timeString);
    i = strlen(timeString);
    while (i > 0 && timeString[--i] == '\n') {
	timeString[i] = '\0';
    }
    g_log << "+++ " << APP_VERSION_STRING << " status for " << libStatus.station_name 
	  << ". Local time: " << timeString << std::endl;
  
    // BPS entries
    g_log << "--- BPS from Q660 (min/hour/day): ";
    for(int i=(int)AD_MINUTE; i <= (int)AD_DAY; i = i + 1) {
	if(libStatus.accstats[LOGF_RECVBPS][i] != INVALID_ENTRY) {
	    g_log << (int) libStatus.accstats[LOGF_RECVBPS][i] << "Bps";
	} else {
	    g_log << "---";
	}
	if(i != AD_DAY) {
	    g_log << "/";
	} else {
	    g_log << std::endl;
	}
    }

    // packet entries
    g_log << "--- Packets from Q660 (min/hour/day): ";
    for(int i=(int)AD_MINUTE; i <= (int)AD_DAY; i = i + 1) {
	if(libStatus.accstats[LOGF_PACKRECV][i] != INVALID_ENTRY) {
	    g_log << (int) libStatus.accstats[LOGF_PACKRECV][i] << "Pkts";
	} else {
	    g_log << "---";
	}
	if(i != AD_DAY) {
	    g_log << "/";
	} else {
	    g_log << std::endl;
	}
    }

    // percent of the buffer left, and the clock quality
    g_log << "--- Q660 Packet Buffer Available: " << 100-(libStatus.pkt_full) << "%, Clock Quality: " << 
	libStatus.clock_qual << "%" << std::endl;
}


enum tlibstate Lib660Interface::getLibState() {
    return this->currentLibState;
}

void Lib660Interface::log_q8serv_config(ConfigVO ourConfig) {
    g_log << "+++ Config info:" << std::endl;
    g_log << "+++   ServerName =                     " << ourConfig.getServerName() << std::endl;
    g_log << "+++   ServerDesc =                     " << ourConfig.getServerDesc() << std::endl;
    g_log << "+++   ServerDir =                      " << ourConfig.getServerDir() << std::endl;
    g_log << "+++   ServerSource =                   " << ourConfig.getServerSource() << std::endl;
    g_log << "+++   SeedStation =                    " << ourConfig.getSeedStation() << std::endl;
    g_log << "+++   SeedNetwork =                    " << ourConfig.getSeedNetwork() << std::endl;
    g_log << "+++   LogDir =                         " << ourConfig.getLogDir() << std::endl;
    g_log << "+++   LogType =                        " << ourConfig.getLogType() << std::endl;
    g_log << "+++   Q660TcpAddr =                    " << ourConfig.getQ660TcpAddr() << std::endl;
    g_log << "+++   Q660BasePort =                   " << ourConfig.getQ660BasePort() << std::endl;
    g_log << "+++   Q660Priority =                   " << ourConfig.getQ660Priority() << std::endl;
    g_log << "+++   Q660SerialNumber =               " << ourConfig.getQ660SerialNumber() << std::endl;
    g_log << "+++   Q660Password =                   " << ourConfig.getQ660Password() << std::endl;
    g_log << "+++   LockFile =                       " << ourConfig.getLockFile() << std::endl;
    g_log << "+++   StartMsg =                       " << ourConfig.getStartMsg() << std::endl;
    g_log << "+++   StatusInterval =                 " << ourConfig.getStatusInterval() << std::endl;
    g_log << "+++   Verbosity =                      " << ourConfig.getVerbosity() << std::endl;
    g_log << "+++   Diagnostic =                     " << ourConfig.getDiagnostic() << std::endl;
    g_log << "+++   LogLevel =                       " << ourConfig.getLogLevel() << std::endl;
    g_log << "+++   FailedRegistrationsBeforeSleep = " << ourConfig.getFailedRegistrationsBeforeSleep() << std::endl;
    g_log << "+++   MinutesToSleepBeforeRetry =      " << ourConfig.getMinutesToSleepBeforeRetry() << std::endl;
    g_log << "+++   DutyCycle_MaxConnectTime =       " << ourConfig.getDutyCycle_MaxConnectTime() << std::endl;
    g_log << "+++   DutyCycle_SleepTime =            " << ourConfig.getDutyCycle_SleepTime() << std::endl;
    g_log << "+++   MulticastEnabled =               " << ourConfig.getMulticastEnabled() << std::endl;
    g_log << "+++   MulticastPort =                  " << ourConfig.getMulticastPort() << std::endl;
    g_log << "+++   MulticastHost =                  " << ourConfig.getMulticastHost() << std::endl;
    g_log << "+++   MulticastChannelList =           " << ourConfig.getMulticastChannelList() << std::endl;
    g_log << "+++   ContFileDir =                    " << ourConfig.getContFileDir() << std::endl;
    g_log << "+++   LimitBackfill =                  " << ourConfig.getLimitBackfill() << std::endl;
    g_log << "+++   WaitForClients =                 " << ourConfig.getWaitForClients() << std::endl;
    g_log << "+++   PacketQueueSize =                " << ourConfig.getPacketQueueSize() << std::endl;
    // Bandwith control options
    g_log << "+++   OptThrottleKbitpersec =          " << ourConfig.getOptThrottleKbitpersec() << std::endl;
    g_log << "+++   OptBwfillKbitTarget =            " << ourConfig.getOptBwfillKbitTarget() << std::endl;
    g_log << "+++   OptBwfillProbeInterval =         " << ourConfig.getOptBwfillProbeInterval() << std::endl;
    g_log << "+++   OptBwfillExceedTrigger =         " << ourConfig.getOptBwfillExceedTrigger() << std::endl;
    g_log << "+++   OptBwfillIncreaseInterval =      " << ourConfig.getOptBwfillIncreaseInterval() << std::endl;
    g_log << "+++   OptBwfillMaxLatency =            " << ourConfig.getOptBwfillMaxLatency() << std::endl;
}


/***********************************************************************
 *
 * lib660 callbacks and associated routines.
 * Since they are callback routines, they are all static, and cannot
 * access non-static class variables.
 *
 ***********************************************************************/


/***********************************************************************
 * state_callback:
 * 	Receive lib660's new state, and set interface's view of the state.
 ***********************************************************************/
void Lib660Interface::state_callback(pointer p) {
    tstate_call *state;

    state = (tstate_call *)p;
 
    if(state->state_type == ST_STATE) {
	g_libInterface->setLibState((enum tlibstate)state->info);
    }
}


/***********************************************************************
 * onesec_callback:
 * 	Receive onesec single channel uncompessed data packets from lib660.
 * 	Multicast the packet if configured, and channel NOT seen by 
 * 	lowlatency_callback.
 *	Convert lib660 epocch time to lib330 epoch time for
 *	compatibility with q330 onesec multicast packets.
 *	All values are multicast in network byte order.
 ***********************************************************************/
void Lib660Interface::onesec_callback(pointer p) {
    onesec_pkt msg;
    tonesec_call *src = (tonesec_call*)p;
    int retval;
    char temp[32];
    uint32_t q330_timestamp_sec;
    char *tp;
  
    // Translate tonesec_call to onesec_pkt;
    memset(&msg, 0, sizeof(msg));
    memset(temp, 0, sizeof(temp));
    strncpy(temp,src->station_name,9);
    tp = strtok((char*)temp,(char*)"-");
    // If the station_name is not valid, ignore the packet.
    // The station_name is not valid when not connected to a Q8.
    // However, lib660 still generates SOH channels with invalid station_name.
    if (tp == NULL) {
	strncpy(temp,src->station_name,9);
#ifdef DEBUG_Lib660Interface 
	g_log << "ERROR in " << __func__ << ": Bad format for station_name: " << temp << std::endl;
#endif
	return;
    }
    strcpy(msg.net,tp);
    strcpy(msg.station,strtok(NULL,(char*)"-"));
    strcpy(msg.channel,src->channel);
    strcpy(msg.location,src->location);
    // Ensure that channel is 3 characters, and location is 2 characters, blank padded.
    int lc = strlen(msg.channel);
    int ll = strlen(msg.location);
    if (lc < 3) strncat(msg.channel,"   ", 3-lc);
    if (ll < 2) strncat(msg.location,"  ", 2-ll);
    msg.rate = htonl((int)src->rate);

    // Determine whether to multicast this packet.
    int fnmatch_flags = 0;
    for(int i=0; i < num_multicastChannelEntries; i++) {
	if( fnmatch(multicastChannelList[i].channel, msg.channel, fnmatch_flags) == 0 && 
	    fnmatch(multicastChannelList[i].location, msg.location, fnmatch_flags) == 0) {

	    // Only multicast if it is not a lowlatency channel. */
	    std::string chanloc = (std::string)msg.channel + "." + (std::string)msg.location;
	    it = lowlatencymap.find(chanloc);
	    if (it == lowlatencymap.end()) {
		// Channel is not a lowlatency channel.  Multicast it.
		// All fields in multicast msg must be in network byte order.
		for(int i=0;i<src->sample_count;i++){
		    msg.samples[i] = htonl((int)src->samples[i]);
		}
		int msgsize = ONESEC_PKT_HDR_LEN + src->sample_count * sizeof(int);
		
		// Convert q660 epoch time to q330 epoch time for the multicast timestamp.
		// q660 epoch time starts at 2016-01-01T00:00:00 UTC.
		// q330 epoch time starts at 2000-01-01T00:00:00 UTC.
		// Both systems use a nominal epoch time (all days have 86400 seconds),
		// and do not count leapseconds.
		q330_timestamp_sec = (uint32_t)src->timestamp + Q660_to_Q330_sec_offset;
		msg.timestamp_sec = q330_timestamp_sec;
		msg.timestamp_usec = (src->timestamp - (double)((uint32_t)src->timestamp))*1000000;
#ifdef DEBUG_Lib660Interface
		g_log << __func__ << ": channnel: " << 
		    msg.station << "." << msg.net << "." << msg.channel << "." << msg.location << 
		    " sample_count=" << src->sample_count << 
//		    " q330_timestamp=" << msg.timestamp_sec << "," << msg.timestamp_usec << std::endl;
		    " Q8_timestamp=" << std::fixed << src->timestamp << std::endl;
#endif
		msg.timestamp_sec = htonl(msg.timestamp_sec);
		msg.timestamp_usec = htonl(msg.timestamp_usec);
		retval = sendto(mcastSocketFD, &msg, msgsize, 0, (struct sockaddr *) &(mcastAddr), sizeof(mcastAddr));
#ifdef DEBUG_MULTICAST      
		g_log << "Multicasting " << msg.station << "." << msg.net << "." <<  msg.channel << "." << msg.location << std::endl;
#endif
		if(retval < 0) {
		    g_log << "XXX Unable to send multicast packet: " << strerror(errno) << std::endl;
		}
	    }
	} 
    }
}


/***********************************************************************
 * lowlatency_callback
 *	Receive lowlatency single channel uncompessed data packets from lib660.
 *	Multicast the packet if configured.
 *	Convert lib660 epocch time to lib330 epoch time for compatibility
 *	with q330 onesec multicast packets.
 *	All values are multicast in network byte order.
 ***********************************************************************/
void Lib660Interface::lowlatency_callback(pointer p) {
    onesec_pkt msg;
    tonesec_call *src = (tonesec_call*)p;
    int retval;
    char temp[32];
    uint32_t q330_timestamp_sec;
    char *tp;
  
    // Translate tonesec_call to onesec_pkt;
    memset(&msg, 0, sizeof(msg));
    memset(temp, 0, sizeof(temp));
    strncpy(temp,src->station_name,9);
    tp = strtok((char*)temp,(char*)"-");
    // If the station_name is not valid, ignore the packet.
    // The station_name is not valid when not connected to a Q8.
    // However, lib660 still generates SOH channels with invalid station_name.
    if (tp == NULL) {
	strncpy(temp,src->station_name,9);
#ifdef DEBUG_Lib660Interface 
	g_log << "ERROR in " << __func__ << ": Bad format for station_name: " << temp << std::endl;
#endif
	return;
    }
    strcpy(msg.net,tp);
    strcpy(msg.station,strtok(NULL,(char*)"-"));
    strcpy(msg.channel,src->channel);
    strcpy(msg.location,src->location);
    // Ensure that channel is 3 characters, and location is 2 characters, blank padded.
    int lc = strlen(msg.channel);
    int ll = strlen(msg.location);
    if (lc < 3) strncat(msg.channel,"   ", 3-lc);
    if (ll < 2) strncat(msg.location,"  ", 2-ll);
    msg.rate = htonl((int)src->rate);


    // Determine whether to multicast this packet.
    int fnmatch_flags = 0;
    for(int i=0; i < num_multicastChannelEntries; i++) {
	if( fnmatch(multicastChannelList[i].channel, msg.channel, fnmatch_flags) == 0 && 
	    fnmatch(multicastChannelList[i].location, msg.location, fnmatch_flags) == 0) {

	    // Add channel to the lowlatency map it if is not there already.
	    std::string chanloc = (std::string)msg.channel + "." + (std::string)msg.location;
	    it = lowlatencymap.find(chanloc);
	    if (it == lowlatencymap.end()) {
		lowlatencymap[chanloc] = 1;
	    }
	    // All fields in multicast msg must be in network byte order.
	    for(int i=0;i<src->sample_count;i++){
		msg.samples[i] = htonl((int)src->samples[i]);
	    }
	    int msgsize = ONESEC_PKT_HDR_LEN + src->sample_count * sizeof(int);

	    // Convert q660 epoch time to q330 epoch time for the multicast timestamp.
	    // q660 epoch time starts at 2016-01-01T00:00:00 UTC.
	    // q330 epoch time starts at 2000-01-01T00:00:00 UTC.
	    // Both systems use a nominal epoch time (all days have 86400 seconds),
	    // and do not count leapseconds.
	    q330_timestamp_sec = (uint32_t)src->timestamp + Q660_to_Q330_sec_offset;
	    msg.timestamp_sec = q330_timestamp_sec;
	    msg.timestamp_usec = (src->timestamp - (double)((uint32_t)src->timestamp))*1000000;
#ifdef DEBUG_Lib660Interface
	    g_log << __func__ << ": channnel: " << 
		msg.station << "." << msg.net << "." << msg.channel << "." << msg.location << 
		" sample_count=" << src->sample_count << 
//		" q330_timestamp=" << msg.timestamp_sec << "," << msg.timestamp_usec << std::endl;
		" Q8_timestamp=" << std::fixed << src->timestamp << std::endl;
#endif
	    msg.timestamp_sec = htonl(msg.timestamp_sec);
	    msg.timestamp_usec = htonl(msg.timestamp_usec);
	    retval = sendto(mcastSocketFD, &msg, msgsize, 0, (struct sockaddr *) &(mcastAddr), sizeof(mcastAddr));
#ifdef DEBUG_MULTICAST      
	    g_log << "Multicasting " << msg.station << "." << msg.net << "." <<  msg.channel << "." << msg.location << std::endl;
#endif
	    if(retval < 0) {
		g_log << "XXX Unable to send multicast packet: " << strerror(errno) << std::endl;
	    }
	} 
    }
}


/***********************************************************************
 * miniseed_callback:
 *	Receive miniseed data packets from lib660.
 *	Queue packet in PacketQueue.
 ***********************************************************************/
void Lib660Interface::miniseed_callback(pointer p) {
    tminiseed_call *data = (tminiseed_call *) p;
    short packetType = 0;
    static int throttling = 0;

    /*
     * Map from datalogger-specific library packet_type to comserv packet_type.
     */
    switch(data->packet_class) {
    case PKC_DATA:
	packetType = RECORD_HEADER_1;
	if(data->timestamp < Lib660Interface::timestampOfLastRecord) {
	    g_log << "XXX Packets being received out of order" << std::endl;
	    Lib660Interface::timestampOfLastRecord = data->timestamp;
	}
	break;
    case PKC_EVENT:
	packetType = DETECTION_RESULT;
	break;
    case PKC_CALIBRATE:
	packetType = CALIBRATION;
	break;
    case PKC_TIMING:
	packetType = CLOCK_CORRECTION;
	break;
    case PKC_MESSAGE:
	packetType = COMMENTS;
	break;
    case PKC_OPAQUE:
	packetType = BLOCKETTE;
	break;
    }

#ifdef DEBUG_SNCL
    {
	char *msbuf = (char *)data->data_address;
	char S[8], N[8], C[8], L[8];
	memmove(S, msbuf+8, 5);
	S[5] = '\0';
	memmove(L, msbuf+13, 2);
	L[2] = '\0';
	memmove(C, msbuf+15, 3);
	C[3] = '\0';
	memmove(N, msbuf+18, 2);
	N[2] = '\0';
	g_log << "MSEED S.N.C.L=" << S << "." << N << "." << C << "." << L << std::endl;
    }
#endif

    // Put the packet in the intermediate packet queue.
    packetQueue->enqueuePacket((char *)data->data_address, data->data_size, packetType);

    // Throttle (delay) for up to 1 second if we are in danger of filling the packet queue.
    // Since this function is called from the lib660 thread, this should help
    // slow input from the data logger.
    // We rely on the main thread to dequeue the packets into the comserv buffers.
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 100000000;
    for(int i = 0; i < 10; i++) {
	int nfree = packetQueue->numFree();
#ifdef DEBUG_PQUEUE
	g_log << "--- nfree in pq = " << nfree << std::endl;
#endif
	if ((! throttling) && (nfree < throttle_free_packet_threshold)) {
	    g_log << "XXX Start delay in miniseed_callback. Intermediate PacketQueue nfree = " << nfree << std::endl;
	    ++throttling;
	}
	if ((throttling) && (nfree >= unthrottle_free_packet_threshold)) {
	    g_log << "--- End delay in miniseed_callback. Intermediate PacketQuue nfree = " << nfree << std::endl;
	    throttling = 0;
	}
	if (! throttling) break;
	nanosleep(&t, NULL);
    }
}


/***********************************************************************
 * queueNearFull:
 *	Return whether the PacketQueue has less the the specified 
 *	minimum number of free packets.  
 * Return: 1 if true , 0 if false.
 ***********************************************************************/
int Lib660Interface::queueNearFull() {
    int nearFull = (packetQueue->numFree() < min_free_packet_threshold);
    return nearFull;
}


/***********************************************************************
 * processPacketQueue:
 *	Transfer packets from from the intermediat PacketQueue to the 
 *	appropriate comserv queue.  
 *	Called from the main thread.
 * Return 1 on success flushing all packets to the comserv packet queues.
 * Return 0 on failure to flush all packets to the comserv packet queues.
 ***********************************************************************/
int Lib660Interface::processPacketQueue() {
    int sendFailed = 0;

    // We do not want to dequeue a packet unless we are guaranteed that
    // there is room in the comserv packet queues to accept it.
    // Otherwise, we risk losing the packet.
    while (packetQueue->numQueued() > 0) {
	if(comserv_anyQueueBlocking()) {
	    return 0;
	}
	QueuedPacket thisPacket = packetQueue->dequeuePacket();
	if (thisPacket.dataSize != 0) {
	    sendFailed = comserv_queue((char *)thisPacket.data, thisPacket.dataSize, thisPacket.packetType);
	    if(sendFailed) {
		// This should only happen if a packet is mal-formed and the type is not
		// identifiable by the comserv queueing system.
		return 0;
	    }
	}
    }    
    return 1;
}


/***********************************************************************
 * archival_miniseed_callback:
 *	Receive archival miniseed data packets from lib660.
 ***********************************************************************/
void Lib660Interface::archival_miniseed_callback(pointer p) {
    //tminiseed_call *data = (tminiseed_call *) p;
    //g_log << "Archival (" << data->channel << ") with timestamp: " << data->timestamp << " " << data->data_size << " bytes" << std::endl;
}

/***********************************************************************
 * msg_callback:
 *	Receive messages from lib660.
 ***********************************************************************/
void Lib660Interface::msg_callback(pointer p) {
    tmsg_call *msg = (tmsg_call *) p;
    string95 msgText;
    char dataTime[32];

    lib_get_msg(msg->code, (pchar)&msgText);
  
    if(!msg->datatime) {
	g_log << "LOG {" << msg->code << "} " << msgText << " " << msg->suffix << std::endl;
    } else {
	jul_string(msg->datatime, (char *) (&dataTime));
	g_log << "LOG {" << msg->code << "} " << " [" << dataTime << "] " << msgText << " "  << msg->suffix << std::endl;
    }    

}


/************************************************************************
 *
 * Linux Q8 file handling for q8serv and derivatives programs.
 * Originally borrowed from Quanterra Seneca code.
 * Modified for Linux by Doug Neuhauser - doug@seismo.berkeley.edu 
 *
 * Provide specialized mapping of "cont" directory.
 * Functions in lib660/libcont.c refer to "cont" files with a 
 * pathname of the format: "cont/filename".
 * This function maps translates these "cont" file pathnames
 * to a site-configurable path prefix.  It also returns the
 * appropriate filetype for these translated files.
 *
 ************************************************************************/

#if DEBUG_FILE_CALLBACK > 0
static const char *FK_str[] = { "File_Unknown", "File_CFG", "File_Cont", "File_Data" } ;
static const char *FAT_str[] = { "OPEN", "CLOSE", "DELETE", "SEEK", "READ", "WRITE", "SIZE", "CL_RDIR", "DIR_FIRST", "DIR_NEXT", "DIR_CLOSE" } ;
static const char *tag = (char *)"q8serv_file";
#endif

/***********************************************************************
 * translate_file:
 *	Translate generic lib660 filenames used for cont file to 
 *	specific Lib660Interface pathnames.
 ***********************************************************************/
enum tfilekind Lib660Interface::translate_file (char *outfile, char *infile, char *prefix, pfile_owner pfo)
{
    char *dir = NULL;
    char *fn = NULL;
    char instr[PATH_MAX];
    char *p;
  
    strcpy (outfile, "") ;   /* initialize outfile to empty string. */
    strcpy (instr, infile) ; /* working copy, just in case 's' is a constant */
    /* Check to see if infile has a directory component. */
    /* If so, split it into dir and fn. */
    p = strchr(instr, '/') ;
    if (p)
    {
	/* Split infile into initial dir and fn. */
	*p++ = 0 ; /* terminate dir and point to remainder of filename */
	fn = p;
	dir = (pchar)&(instr) ; /* dir is first part */
    }
    else
    {
	/* No initial dir -> simple filename or full pathname for data file. */
	/* Filetype is DATA file. */
	strcpy(outfile, infile) ; /* No dir in q8serv means a data file */
	return FK_IDXDAT ;
    }
    if (strcmp(dir, "cont") == 0)
    {
	/* If we have a prefix, replace dir with prefix. */
	/* Filetype is CONTINUITY file */
	strcpy (outfile, prefix);
	strcat (outfile, fn);
	return FK_CONT ;
    }	
    else
    {
	/* File type is UNKNOWN file. */
	return FK_UNKNOWN ; /* Unknown directory prefix */
    }
}

/************************************************************************
 * file_callback:
 *	Lib66 file access callback.
 *	File access  callback in lib660 is only for cont files.
 * 	For cont files, remove cont/ prefix provided by lib660,
 * 	and add the optional station-specific directory and file prefix.
 ************************************************************************/
void Lib660Interface::file_callback (pointer p)
{
    tfileacc_call *pfa ;
    char fname[PATH_MAX] = "";
    int flags ;
    mode_t rwmode ;
    off_t result, long_offset ;
    ssize_t numread ;
    ssize_t numwrite ;
    struct stat sb ;
    int filekind = FK_UNKNOWN;
  
    private_station_info *pstation_info = NULL;
    char *conf_path_prefix = NULL;
    pfile_owner owner;
    char options_str[128];
  
    pfa = (tfileacc_call *)p ;
    pfa->fault = FALSE ;

    /* Prepend optional private conf_path_prefix to the file.
       Otherwise, set prefix to stationName.
    */
    owner = pfa->owner;
    if (owner && owner->station_ptr)
	pstation_info = (private_station_info *)owner->station_ptr;
    if (pstation_info && (pstation_info->conf_path_prefix[0] != '\0'))
	conf_path_prefix = pstation_info->conf_path_prefix;
    else
	conf_path_prefix = pstation_info->stationName;

    /* Translate the input file for OPEN and DEL operations and determine filetype. */
    /* These are the only operations that provide a filename. */
    if (pfa->fileacc_type == FAT_OPEN || pfa->fileacc_type == FAT_DEL)
    {
	filekind=translate_file ((pchar)fname, pfa->fname, conf_path_prefix, pfa->owner);
    }
    else
    {
	if (pfa->fname) strcpy (fname, pfa->fname);
	else strcpy(fname, "");
	filekind = FK_UNKNOWN;
    }

    /* Perform the specified file operation. */
    switch (pfa->fileacc_type)
    {

    case FAT_OPEN :
	options_str[0] = '\0';
	if (filekind == FK_UNKNOWN)
	{
	    pfa->fault = TRUE ;
#if DEBUG_FILE_CALLBACK > 0
	    LogMessage (CS_LOG_TYPE_DEBUG, "%s: path=%s (fn=%s,owner=%p, pstation_info=%p) op=%s type=%s fd=%d err=%d\n", tag,
			fname, pfa->fname, owner, pstation_info, FAT_str[pfa->fileacc_type], FK_str[filekind], pfa->handle, pfa->fault) ;
#endif
	    return ;
	}
	if (pfa->options & LFO_CREATE)
	{
	    flags = O_CREAT | O_TRUNC ;
	    strcat(options_str,"O_CREAT|O_TRUNC") ;
	}
	else
	{
	    flags = 0 ;
	}
	if (pfa->options & LFO_WRITE)
	{
	    if (pfa->options & LFO_READ)
	    {
		flags = flags | O_RDWR ;
		if (strlen(options_str)) strcat(options_str, "|") ;;
		strcat(options_str, "O_RDWR") ;
	    }
	    else
	    {
		flags = flags | O_WRONLY ;
		if (strlen(options_str)) strcat(options_str, "|") ;;
		strcat(options_str, "O_WRONLY") ;
	    }
	}
	else if (pfa->options & LFO_READ)
	{
	    flags = flags | O_RDONLY ;
	    if (strlen(options_str)) strcat(options_str, "|") ;;
	    strcat(options_str, "O_RDONLY") ;
	}
	rwmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;
	pfa->handle = open (fname, flags, rwmode) ;
	if (pfa->handle < 0)
	{
	    pfa->fault = TRUE ;
	    pfa->handle = INVALID_FILE_HANDLE ;
	}
#if DEBUG_FILE_CALLBACK > 0
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: path=%s (fn=%s,owner=%p, pstation_info=%p) op=%s type=%s fd=%d err=%d\n", tag,
		    fname, pfa->fname, owner, pstation_info, FAT_str[pfa->fileacc_type], FK_str[filekind], pfa->handle, pfa->fault) ;
#endif
	break ;

    case FAT_CLOSE :
	close (pfa->handle) ;
#if DEBUG_FILE_CALLBACK > 0
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: op=%s fd=%d err=%d\n", tag,
		    FAT_str[pfa->fileacc_type], pfa->handle, pfa->fault);
#endif
	break ;

    case FAT_DEL :
	if (filekind == FK_UNKNOWN)
	{
	    pfa->fault = TRUE ;
#if DEBUG_FILE_CALLBACK > 1
	    LogMessage (CS_LOG_TYPE_DEBUG, "%s: path=%s op=%s type=%s fd=%d err=%d\n", tag,
			fname, FAT_str[pfa->fileacc_type], FK_str[filekind], pfa->handle, pfa->fault);
#endif
	    return ;
	}
	pfa->fault = remove (fname) ;
#if DEBUG_FILE_CALLBACK > 1
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: path=%s op=%s type=%s fd=%d err=%d\n", tag,
		    fname, FAT_str[pfa->fileacc_type], FK_str[filekind], pfa->handle, pfa->fault);
#endif
	break ;
 
    case FAT_SEEK :
	long_offset = pfa->options ;
	result = lseek(pfa->handle, long_offset, SEEK_SET) ;
#ifdef __APPLE__
	if (result < 0)
	{
	    pfa->fault = TRUE ;
	}
#else
	if (result != long_offset)
	{
	    pfa->fault = TRUE ;
	}
#endif
#if DEBUG_FILE_CALLBACK > 1
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: op=%s fd=%d offset=%jd err=%d\n", tag,
		    FAT_str[pfa->fileacc_type],  pfa->handle, (intmax_t)result, pfa->fault);
#endif
	break ;

    case FAT_READ :
	numread = read (pfa->handle, pfa->buffer, pfa->options) ;
	if ((int)numread != pfa->options)
	{
	    pfa->fault = TRUE ;
	}
#if DEBUG_FILE_CALLBACK > 1
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: op=%s fd=%d nread=%d err=%d\n", tag,
		    FAT_str[pfa->fileacc_type], pfa->handle, numread, pfa->fault);
#endif
	break ;

    case FAT_WRITE :
	numwrite = write(pfa->handle, pfa->buffer, pfa->options) ;
	if ((int)numwrite != pfa->options)
	{
	    pfa->fault = TRUE ;
	}	  
#if DEBUG_FILE_CALLBACK > 1
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: op=%s fd=%d nwrite=%d err=%d\n", tag,
		    FAT_str[pfa->fileacc_type], pfa->handle, numwrite, pfa->fault);
#endif
	break ;

    case FAT_SIZE :
	pfa->fault = fstat (pfa->handle, &(sb)) ;
	pfa->options = (int)sb.st_size ;
#if DEBUG_FILE_CALLBACK > 1
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: op=%s fd=%d err=%d\n", tag,
		    FAT_str[pfa->fileacc_type], pfa->handle, pfa->fault);
#endif
	break ;

    default :
	/* Remainder of cases only required for balelib support */
	pfa->fault = TRUE;
#if DEBUG_FILE_CALLBACK > 1
	LogMessage (CS_LOG_TYPE_DEBUG, "%s: path=%s op=%s type=%s fd=%d err=%d\n", tag,
		    fname, "UNKNOWN_OP", FK_str[FK_UNKNOWN], pfa->handle, pfa->fault);
#endif
	break ; 
    }
    return ;
}


/***********************************************************************
 *
 * For internal use only
 *
 ***********************************************************************/

/***********************************************************************
 * sendUserMessage:
 ***********************************************************************/
int Lib660Interface::sendUserMessage(char *msg) { return 0;}


/***********************************************************************
 * setLibState:
 *	Set interface class's view of the current lib660 state.
 *	Do NOT call startDataFLow if we are in LIBSTAT_RUNWAIT.
 ***********************************************************************/
void Lib660Interface::setLibState(enum tlibstate newState) {
    string63 newStateName;

    lib_get_statestr(newState, (pchar)&newStateName);
    g_log << "+++ State change to '" << newStateName << "'" << std::endl;
    this->currentLibState = newState;

    /*
     * Do NOT automatically call startDataFlow if we are in LIBSTATE_RUNWAIT.
     * Allow main program to decide when to call startDataFlow.
     */

    switch (newState) {
    case LIBSTATE_IDLE:
    case LIBSTATE_WAIT:
	/* Clear the lowlatency map. */
	g_log << "+++ Clearing lowlantecy map" << std::endl;
	lowlatencymap.clear();
	break;
    default:
	break;
    }
}


/***********************************************************************
 * handleError:
 *	Handle an error condition.  Log, cleanup etc...
 ***********************************************************************/
void Lib660Interface::handleError(enum tliberr errcode) {
    string63 errmsg;
    lib_get_errstr(errcode, (pchar)&errmsg);
    g_log << "XXX : Encountered error: " << errmsg << std::endl;
}


/***********************************************************************
 * build_multicastChannelList:
 *	Parse multicast channellist string into a multicastChannelEntry array.
 ***********************************************************************/
bool Lib660Interface::build_multicastChannelList(char * input) {
    char localInput[2048];
    char *tok;
    char *loc;

    int nc = sizeof(multicastChannelList[0].channel);
    int nl = sizeof(multicastChannelList[0].location);
    int mc = 3; // max length of SEED channel.
    int ml = 2; // max length of SEED location.
    int lc, ll;

    num_multicastChannelEntries = 0;
    memset(multicastChannelList, 0, sizeof(multicastChannelList));

    strncpy(localInput, input, sizeof(localInput));
    tok = strtok(localInput, ",");

    /* Process each entry in the input string.
       Entry is one of the following formats:
       a.  channel		# ANY location code.
       b.  channel.		# BLANK location code.
       c.  channnel.--		# BLANK location code.
       c.  channel.location	# Explicit location code.
       Channel and location codes may include the '?' wildcard character.
    */
    int n = 0;
    while(tok != NULL) {
	if (num_multicastChannelEntries >= MAX_MULTICASTCHANNELENTRIES) {
	    return false;
	}
	loc = strchr(tok,'.');
	if (loc) {
	    // We have a channel.location token.
	    *loc++ = '\0';
	    lc = strlen(tok);
	    ll = strlen(loc);
	    // Copy the channel.  Blank pad it to mc number of characters.
	    strncpy(multicastChannelList[n].channel,tok,nc-1);
	    if (lc < mc) strncat(multicastChannelList[n].channel, "   ", mc-lc);
	    // An empty, "-", or "--" location string means blank location code.
	    if (*loc == '\0' || *loc == '-') {
		strncpy(multicastChannelList[n].location,"  ",nl-1);
	    }
	    else {
		// Copy the location.  Blank pad it to lc number of characters.
		strncpy(multicastChannelList[n].location,loc,nl-1);
		if (ll < ml) strncat(multicastChannelList[n].location, "  ", ml-ll);
	    }
	}
	else {
	    // We have just a channel token.  Assume all location codes.
	    lc = strlen(tok);
	    strncpy(multicastChannelList[n].channel,tok,nc-1);
	    if (lc < mc) strncat(multicastChannelList[n].channel, "   ", mc-lc);
	    strncpy(multicastChannelList[n].location,"??",nl-1);
	}
	// Ensure that we null-terminated entries of maximal SEED component lengths.
	multicastChannelList[n].channel[mc] = '\0';
	multicastChannelList[n].location[ml] = '\0';
	++n;
	num_multicastChannelEntries = n;
	tok = strtok(NULL, ",");
    }
    return true;
}
