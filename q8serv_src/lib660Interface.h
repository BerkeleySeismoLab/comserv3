/* 
 * Lib660Interface class
 *
 * Modification History:
 *  2020-04-08 DSN Initial coding derived from lib330interface.C
 *  2020-09-29 DSN Updated for comserv3.
 */

#ifndef __LIB660INTERFACE_H__
#define __LIB660INTERFACE_H__

#include <iostream>
#include <map>
#include <string>

/*
** pascal.h and C++ don't get along well (and, or, xor etc... mean something in C++)
*/
#define pascal_h
#include <platform.h>

/*
** These are C functions, and should be treated as such
*/
#define PROG_DL 1
extern "C" {
#include <libclient.h>
#include <libtypes.h>
#include <libmsgs.h>
#include <libsupport.h>
#include <libstrucs.h>
#include "q8_private_station_info.h"
#include "file_callback.h"
}

#include "ConfigVO.h"
#include "PacketQueue.h"

#define MAX_MULTICASTCHANNELENTRIES	256

typedef struct {
    char channel[4];
    char location[4];
} multicastChannelEntry;

struct onesec_pkt{
    char net[4];
    char station[16];
    char channel[16];
    char location[4];
    uint32_t rate;
    uint32_t timestamp_sec;
    uint32_t timestamp_usec;
    int32_t samples[MAX_RATE];
};

#define ONESEC_PKT_HDR_LEN 52

// Q330 epoch start time is 2000-01-01T00:00:00 
// Q660 epoch start time is 2016-01-01T00:00:00
// Both systems count in nominal seconds (ignoring leapseconds),
// eg 1 day is always 8640 seconds.

// Define the number of nominal seconds between Q330 and Q660 epoch start times.
#define Q660_to_Q330_sec_offset	(((16 * 365) + 4) * 86400)

// Strict compilers and loaders only allow externals to be defined in 1 file.
// One and only one source file should define DEFINE_EXTERNAL.

#ifdef	DEFINE_EXTERNAL
#define	EXTERN
#else
#define	EXTERN extern
#endif

// These variable must be global because they are used in callback routines 
// called from outside the class, so they cannot be class variables.

EXTERN PacketQueue *packetQueue;
EXTERN struct sockaddr_in mcastAddr;
#ifdef DEFINE_EXTERNAL
EXTERN int mcastSocketFD = -1;
#else
EXTERN int mcastSocketFD;
#endif
EXTERN char multicastChannelList[256][8];

extern tcontext g_stationContext;	//:: DEBUG

class Lib660Interface {
public:
    Lib660Interface(char *, ConfigVO configInfo);
    ~Lib660Interface();

    void startRegistration();
    void startDeregistration();
    void changeState(enum tlibstate, enum tliberr);
    void startDataFlow();
    void flushData();
    void displayStatusUpdate();
    int waitForState(enum tlibstate, int, void(*)());
    enum tlibstate getLibState();
    int processPacketQueue();
    int queueNearFull();
    bool build_multicastChannelList(char *);
    void log_q8serv_config(ConfigVO ourConfig);

    // These functions are static because they are callback routines from lib660.
    static void state_callback(pointer p);
    static void miniseed_callback(pointer p);
    static void archival_miniseed_callback(pointer p);
    static void msg_callback(pointer p);
    static void onesec_callback(pointer p);
    static void lowlatency_callback(pointer p);
    static void file_callback(pointer p);
    static enum tfilekind translate_file (char *outfile, char *infile, char *prefix, pfile_owner pfo);

    // These variables must be static because they are used by callback routines.
    static int num_multicastChannelEntries;
    static multicastChannelEntry multicastChannelList[MAX_MULTICASTCHANNELENTRIES];
    static double timestampOfLastRecord;
    /* Map and interator for list of lowlatency channels. */
    static std::map<std::string, int> lowlatencymap;
    static std::map<std::string,int>::iterator it;

private:
    int sendUserMessage(char *);
    void initializeCreationInfo(char *, ConfigVO);
    void initializeRegistrationInfo(ConfigVO);
    void handleError(enum tliberr);
    void setLibState(enum tlibstate);

    tcontext stationContext;
    tpar_register registrationInfo;
    tpar_create   creationInfo;
    enum tlibstate currentLibState;
    /* Structures for site-specific info. */
    tfile_owner fowner;
    private_station_info station_info;

};

#endif
