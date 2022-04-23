/* 
 * Lib330Interface class
 *
 * Modifications:
 *  21 May 2012 - DSN - Close mcastSocketFD on Lib330Interface destruction.
 *  27 Feb 2013 - DSN - fix memset arg in Lib330Interface::initializeRegistrationInfo
 *  2015/09/25  - DSN - added log message when creating lib330 interface.
 *  2020-09-29 DSN Updated for comserv3.
 *  2021-03-13 DSN Fixed creating and matching multicast channel+location list.
 *  2022-03-16 DSN Added support for TCP connection to Q330/baler.
 */

#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fnmatch.h>

#include "global.h"
#include "comserv_queue.h"
#include "lib330Interface.h"
#include "portingtools.h"

// #define DEBUG_Lib330Interface

static int throttle_free_packet_threshold = 0.5 * PACKETQUEUE_QUEUE_SIZE;
static int min_free_packet_threshold = 0.1 * PACKETQUEUE_QUEUE_SIZE;

// Static variables from Lib330Interface needed in static callback functions.
double Lib330Interface::timestampOfLastRecord = 0;
int Lib330Interface::num_multicastChannelEntries = 0;
multicastChannelEntry Lib330Interface::multicastChannelList[MAX_MULTICASTCHANNELENTRIES];


Lib330Interface::Lib330Interface(char *stationName, ConfigVO ourConfig) {
    pmodules mods;
    tmodule *mod;
    int x;

    g_log << "+++ lib330 Interface created" << std::endl;
    this->currentLibState = LIBSTATE_IDLE;
    this->initializeCreationInfo(stationName, ourConfig);
    this->initializeRegistrationInfo(ourConfig);

    mods = lib_get_modules();
    g_log << "+++ Lib330 Modules:" << std::endl;
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

    lib_create_context(&(this->stationContext), &(this->creationInfo));
    if(this->creationInfo.resp_err == LIBERR_NOERR) {
	g_log << "+++ Station thread created" << std::endl;
    } else {
	this->handleError(creationInfo.resp_err);
    }


}


Lib330Interface::~Lib330Interface() {
    enum tliberr errcode;
    g_log << "+++ Cleaning up lib330 Interface" << std::endl;
    this->startDeregistration();
    while(this->getLibState() != LIBSTATE_IDLE) {
	sleep(1);
    }
    this->changeState(LIBSTATE_TERM, LIBERR_CLOSED);
    while(getLibState() != LIBSTATE_TERM) {
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
    g_log << "+++ lib330 Interface destroyed" << std::endl;
}

void Lib330Interface::startRegistration() { 
    enum tliberr errcode;
    this->ping();
    g_log << "+++ Starting registration with Q330" << std::endl;
    errcode = lib_register(this->stationContext, &(this->registrationInfo));
    if(errcode != LIBERR_NOERR) {
	this->handleError(errcode);
    }  
}

void Lib330Interface::startDeregistration() {
    g_log << "+++ Starting deregistration from Q330" << std::endl;
    this->changeState(LIBSTATE_IDLE, LIBERR_NOERR);
}

void Lib330Interface::changeState(enum tlibstate newState, enum tliberr reason) {
    lib_change_state(this->stationContext, newState, reason);
}

void Lib330Interface::startDataFlow() {
    g_log << "+++ Requesting dataflow to start" << std::endl;
    this->changeState(LIBSTATE_RUN, LIBERR_NOERR);
}

void Lib330Interface::flushData() {
    enum tliberr errcode;
    g_log << "+++ Requesting flush data queues" << std::endl;
    errcode = lib_flush_data(this->stationContext);
    if(errcode != LIBERR_NOERR) {
	this->handleError(errcode);
    }
}

void Lib330Interface::ping() {
    lib_unregistered_ping(this->stationContext, &(this->registrationInfo));
}

int Lib330Interface::waitForState(enum tlibstate waitFor, int maxSecondsToWait, void(*funcToCallWhileWaiting)()) {
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


void Lib330Interface::displayStatusUpdate() {
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
	g_log << "XXX Current lib330 state mismatch.  Fixing..." << std::endl;
	lib_get_statestr(currentState, &newStateName);
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
    g_log << "--- BPS from Q330 (min/hour/day): ";
    for(int i=(int)AD_MINUTE; i <= (int)AD_DAY; i = i + 1) {
	if(libStatus.accstats[AC_READ][i] != INVALID_ENTRY) {
	    g_log << (int) libStatus.accstats[AC_READ][i] << "Bps";
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
    g_log << "--- Packets from Q330 (min/hour/day): ";
    for(int i=(int)AD_MINUTE; i <= (int)AD_DAY; i = i + 1) {
	if(libStatus.accstats[AC_PACKETS][i] != INVALID_ENTRY) {
	    g_log << (int) libStatus.accstats[AC_PACKETS][i] << "Pkts";
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
    g_log << "--- Q330 Packet Buffer Available: " << 100-(libStatus.pkt_full) << "%, Clock Quality: " << 
	libStatus.clock_qual << "%" << std::endl;
}

enum tlibstate Lib330Interface::getLibState() {
    return this->currentLibState;
}

/***********************************************************************
 *
 * lib330 callbacks
 *
 ***********************************************************************/


/***********************************************************************
 * state_callback:
 * 	Receive lib330's new state, and set interface's view of the state.
 ***********************************************************************/
void Lib330Interface::state_callback(pointer p) {
    tstate_call *state;

    state = (tstate_call *)p;

    if(state->state_type == ST_STATE) {
	g_libInterface->setLibState((enum tlibstate)state->info);
    }
}


/***********************************************************************
 * onesec_callback:
 *	Receive one second data packets from lib330.
 *	Multicast the packet if configured.
 *
 *  2011 modification to one-second multicast packet:
 *  1.  New structure with explicitly sized data types.
 *  2.  All values are multicast in network byte order.
 ***********************************************************************/
void Lib330Interface::onesec_callback(pointer p) {
    onesec_pkt msg;
    tonesec_call *src = (tonesec_call*)p;
    int retval;
    char temp[32];

    // Translate tonesec_call to onesec_pkt;
    memset(&msg, 0, sizeof(msg));
    strncpy(temp,src->station_name,9);
    strcpy(msg.net,strtok((char*)temp,(char*)"-"));
    strcpy(msg.station,strtok(NULL,(char*)"-"));
    strcpy(msg.channel,src->channel);
    strcpy(msg.location,src->location);
    // Ensure that channel is 3 characters, and location is 2 characters, blank padded.
    int lc = strlen(msg.channel);
    int ll = strlen(msg.location);
    if (lc < 3) strncat(msg.channel,"   ", 3-lc);
    if (ll < 2) strncat(msg.location,"  ", 2-ll);
    msg.rate = htonl((int)src->rate);
    msg.timestamp_sec = htonl((int)src->timestamp);
    msg.timestamp_usec = htonl((int)((src->timestamp - (double)(((int)src->timestamp)))*1000000));

    //#ifdef ENDIAN_LITTLE
    //std::cout <<" TimeStamp for "<<msg.net<<"."<<msg.station<<"."<<msg.channel<<std::endl;
    //g_log <<" : "<<msg.timestamp<<std::endl;
    //SwapDouble((double*)&msg.timestamp);
    //g_log <<" (after swap) TimeStamp for "<<msg.net<<"."<<msg.station<<"."<<msg.channel;
    //g_log <<" : "<<msg.timestamp<<std::endl;
    //#endif

    for(int i=0;i<src->rate;i++){
	msg.samples[i] = htonl((int)src->samples[i]);
    }
    int msgsize = ONESEC_PKT_HDR_LEN + src->rate * sizeof(int);

    // Determine whether to multicast this packet.
    int fnmatch_flags = 0;
    for(int i=0; i < num_multicastChannelEntries; i++) {
	if( fnmatch(multicastChannelList[i].channel, msg.channel, fnmatch_flags) == 0 && 
	    fnmatch(multicastChannelList[i].location, msg.location, fnmatch_flags) == 0) {
	    retval = sendto(mcastSocketFD, &msg, msgsize, 0, (struct sockaddr *) &(mcastAddr), sizeof(mcastAddr));
#ifdef DEBUG_MULTICAST      
	    std::cout << "Multicasting " << msg.station << "." << msg.net << "." <<  msg.channel << "." << msg.location << std::endl;
#endif
	    if(retval < 0) {
		g_log << "XXX Unable to send multicast packet: " << strerror(errno) << std::endl;
	    }
	} 
    }
}


/***********************************************************************
 * miniseed_callback:
 *	Receive miniseed data packets from lib330.
 *	Queue packet in PacketQueue.
 ***********************************************************************/
void Lib330Interface::miniseed_callback(pointer p) {
    tminiseed_call *data = (tminiseed_call *) p;
    short packetType = 0;
    static int throttling = 0;

    /*
     * Map from datalogger-specific library packet_type to comserv packet_type.
     */
    if(data->packet_class != PKC_DATA) {
	switch(data->packet_class) {
	case PKC_DATA:	// never get here, but stops compiler warning.
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
    } else {
	packetType = RECORD_HEADER_1;
	if(data->timestamp < Lib330Interface::timestampOfLastRecord) {
	    g_log << "XXX Packets being received out of order" << std::endl;
	    Lib330Interface::timestampOfLastRecord = data->timestamp;
	}
    }

    // Put the packet in the intermediate packet queue.
    packetQueue.enqueuePacket((char *)data->data_address, data->data_size, packetType);

    // Throttle (delay) for up to 1 second if we are in danger of filling the packet queue.
    // Since this function is called from the lib330 thread, this should help
    // slow input from the data logger.
    // We rely on the main thread to dequeue the packets into the comserv buffers.
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 250000000;
    for(int i=0; i < 4; i++) {
	int nfree = packetQueue.numFree();
	if (nfree < throttle_free_packet_threshold) {
	    if (! throttling) {
		g_log << "XXX Limited space in intermediate queue. Start delay in miniseed_callback" << std::endl;
		++throttling;
	    }
	    nanosleep(&t, NULL);
	}
	else {
	    if (throttling) {
		g_log << "XXX End delay in miniseed_callback" << std::endl;
		throttling = 0;
	    }
	    break;
	}
    }
}


/***********************************************************************
 * queueNearFull:
 *	Return whether the PacketQueue has less the the specified 
 *	minimum number of free packets.  
 * Return: 1 if true , 0 if false.
 ***********************************************************************/
int Lib330Interface::queueNearFull() {
    int nearFull = (packetQueue.numFree() < min_free_packet_threshold);
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
int Lib330Interface::processPacketQueue() {
    int sendFailed = 0;

    // We do not want to dequeue a packet unless we are guaranteed that
    // there is room in the comserv packet queues to accept it.
    // Otherwise, we risk losing the packet.
    while (packetQueue.numQueued() > 0) {
	if(comserv_anyQueueBlocking()) {
	    return 0;
	}
	QueuedPacket thisPacket = packetQueue.dequeuePacket();
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
 *	Receive archival miniseed data packets from lib330.
 ***********************************************************************/
void Lib330Interface::archival_miniseed_callback(pointer p) {
    //tminiseed_call *data = (tminiseed_call *) p;
    //g_log << "Archival (" << data->channel << ") with timestamp: " << data->timestamp << " " << data->data_size << " bytes" << std::endl;
}

/***********************************************************************
 * msg_callback:
 *	Receive archival miniseed data packets from lib330.
 ***********************************************************************/
void Lib330Interface::msg_callback(pointer p) {
    tmsg_call *msg = (tmsg_call *) p;
    string95 msgText;
    char dataTime[32];

    lib_get_msg(msg->code, &msgText);

    if(!msg->datatime) {
	g_log << "LOG {" << msg->code << "} " << msgText << " " << msg->suffix << std::endl;
    } else {
	jul_string(msg->datatime, (char *) (&dataTime));
	g_log << "LOG {" << msg->code << "} " << " [" << dataTime << "] " << msgText << " "  << msg->suffix << std::endl;
    }    

}


/***********************************************************************
 *
 * For internal use only
 *
 ***********************************************************************/


/***********************************************************************
 * sendUserMessage:
 ***********************************************************************/
int Lib330Interface::sendUserMessage(char *msg) { return 0;}


/***********************************************************************
 * setLibState:
 *	Set interface class's view of the current lib330 state.
 *	Do NOT call startDataFLow if we are in LIBSTAT_RUNWAIT.
 ***********************************************************************/
void Lib330Interface::setLibState(enum tlibstate newState) {
    string63 newStateName;

    lib_get_statestr(newState, &newStateName);
    g_log << "+++ State change to '" << newStateName << "'" << std::endl;
    this->currentLibState = newState;

    /*
     * Do NOT automatically call startDataFlow if we are in LIBSTATE_RUNWAIT.
     * Allow main program to decide when to call startDataFlow.
     */
}


/*
** Handle an error condition.  Log, cleanup etc...
*/
void Lib330Interface::handleError(enum tliberr errcode) {
    string63 errmsg;
    lib_get_errstr(errcode, &errmsg);
    g_log << "XXX : Encountered error: " << errmsg << std::endl;
}

void Lib330Interface::initializeRegistrationInfo(ConfigVO ourConfig) {
    // First zero the creationInfo structure.
    memset (&this->registrationInfo, 0, sizeof(this->registrationInfo));
    uint64_t auth = ourConfig.getQ330AuthCode();
    memcpy(this->registrationInfo.q330id_auth, &auth, sizeof(uint64_t));
    if (strlen(ourConfig.getQ330UdpAddr()) > 0) {
	strcpy(this->registrationInfo.q330id_address, ourConfig.getQ330UdpAddr());
    }
    if (strlen(ourConfig.getQ330TcpAddr()) > 0) {
	strcpy(this->registrationInfo.q330id_address, ourConfig.getQ330TcpAddr());
	this->registrationInfo.host_mode = HOST_TCP;
    }
    this->registrationInfo.q330id_baseport = ourConfig.getQ330BasePort();
    strcpy(this->registrationInfo.host_interface, "");
    this->registrationInfo.host_mincmdretry = 5;
    this->registrationInfo.host_maxcmdretry = 40;
    this->registrationInfo.host_ctrlport = 0;
    this->registrationInfo.host_dataport = 0;
    this->registrationInfo.opt_latencytarget = 0;
    this->registrationInfo.opt_closedloop = 0;
    this->registrationInfo.opt_dynamic_ip = 0;
    this->registrationInfo.opt_hibertime = ourConfig.getMinutesToSleepBeforeRetry();
    this->registrationInfo.opt_conntime = ourConfig.getDutyCycle_MaxConnectTime();
    this->registrationInfo.opt_connwait = ourConfig.getDutyCycle_SleepTime();
    this->registrationInfo.opt_regattempts = ourConfig.getFailedRegistrationsBeforeSleep();
    this->registrationInfo.opt_ipexpire = 0;
    this->registrationInfo.opt_buflevel = ourConfig.getDutyCycle_BufferLevel();
}

void Lib330Interface::initializeCreationInfo(char *stationName, ConfigVO ourConfig) {
    // First zero the creationInfo structure.
    memset (&this->creationInfo, 0, sizeof(this->creationInfo));	
    // Fill out the parts of the creationInfo that we know about
    uint64_t serial = ourConfig.getQ330SerialNumber();
    char contFile[512];

    memcpy(this->creationInfo.q330id_serial, &serial, sizeof(uint64_t));
    switch(ourConfig.getQ330DataPortNumber()) {
    case 1:
	this->creationInfo.q330id_dataport = LP_TEL1;
	break;
    case 2:
	this->creationInfo.q330id_dataport = LP_TEL2;
	break;
    case 3:
	this->creationInfo.q330id_dataport = LP_TEL3;
	break;
    case 4:
	this->creationInfo.q330id_dataport = LP_TEL4;
	break;
    }
    strncpy(this->creationInfo.q330id_station, stationName, 5);
    this->creationInfo.host_timezone = 0;
    strcpy(this->creationInfo.host_software, APP_VERSION_STRING);
    if(strlen(ourConfig.getContFileDir())) {
	sprintf(contFile, "%s/q330serv_cont_%s.bin", ourConfig.getContFileDir(), stationName);
    } else {
	sprintf(contFile, "q330serv_cont_%s.bin", stationName);
    }
    strcpy(this->creationInfo.opt_contfile, contFile);
    this->creationInfo.opt_verbose = ourConfig.getLogLevel();
    this->creationInfo.opt_zoneadjust = 1;
    this->creationInfo.opt_secfilter = OSF_ALL;
    this->creationInfo.opt_minifilter = OMF_ALL;
    this->creationInfo.opt_aminifilter = OMF_ALL;
    this->creationInfo.amini_exponent = 0;
    this->creationInfo.amini_512highest = -1000;
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
    } else {
	this->creationInfo.call_secdata = NULL;
    }
    this->creationInfo.call_lowlatency = NULL;
}


// Parse multicast channellist string into a multicastChannelEntry array.
bool Lib330Interface::build_multicastChannelList(char * input) {
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
