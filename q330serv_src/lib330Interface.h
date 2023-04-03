/* 
 * Lib330Interface class
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 */

#ifndef __LIB330INTERFACE_H__
#define __LIB330INTERFACE_H__

/*
** pascal.h and C++ don't get along well (and, or, xor etc... mean something in C++)
*/
#define pascal_h

/*
** These are C functions, and should be treated as such
*/
#define PROG_DL 1
extern "C" {
#include <libclient.h>
#include <libtypes.h>
#include <libmsgs.h>
#include <libsupport.h>
}

#include "ConfigVO.h"
#include "PacketQueue.h"

#define MAX_MULTICASTCHANNELENTRIES 256

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
// Q330 systems count in nominal seconds (ignoring leapseconds),
// eg 1 day is always 8640 seconds.

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


class Lib330Interface {
public:
    Lib330Interface(char *, ConfigVO configInfo);
    ~Lib330Interface();

    void startRegistration();
    void startDeregistration();
    void changeState(enum tlibstate, enum tliberr);
    void startDataFlow();
    void flushData();
    void displayStatusUpdate();
    int waitForState(enum tlibstate, int, void(*)());
    enum tlibstate getLibState();
    void ping();
    int processPacketQueue();
    int queueNearFull();
    bool build_multicastChannelList(char *);
    void log_q330serv_config(ConfigVO ourConfig);

    // These functions are static because they are callback routes from lib330.
    static void state_callback(pointer p);
    static void miniseed_callback(pointer p);
    static void archival_miniseed_callback(pointer p);
    static void msg_callback(pointer p);
    static void onesec_callback(pointer p);

    // These variables must be static because they are used by callback routines.
    static int num_multicastChannelEntries;
    static multicastChannelEntry multicastChannelList[MAX_MULTICASTCHANNELENTRIES];
    static double timestampOfLastRecord;

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

};

#endif
