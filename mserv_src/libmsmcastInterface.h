#ifndef LIBMSMCASTINTERFACE_H
#define LIBMSMCASTINTERFACE_H

/* 
 * LibmsmcastInterface class
 *
 * Modifications:
 *
 * 2020-04-08  - DSN - Initial coding derived from lib330interface.C
 * 2020-09-29 DSN Updated for comserv3.
 */

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
#include <libstrucs.h>
#include "private_station_info.h"
#include "file_callback.h"
}

#include "ConfigVO.h"
#include "PacketQueue.h"

#define MAX_MULTICASTCHANNELENTRIES 256

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


class LibmsmcastInterface {
 public:
  LibmsmcastInterface(char *, ConfigVO configInfo);
  ~LibmsmcastInterface();

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
  void log_mserv_config(ConfigVO ourConfig);

  // These functions are static because they are callback routines from libmsmcast.
  static void state_callback(pointer p);
  static void miniseed_callback(pointer p);
  static void archival_miniseed_callback(pointer p);
  static void msg_callback(pointer p);
  static void onesec_callback(pointer p);
  static void file_callback(pointer p);
  static enum tfilekind translate_file (char *outfile, char *infile, char *prefix, pfile_owner pfo);

  // These variables must be static because they are used by callback routines.
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
  /* Structures for site-specific info. */
  tfile_owner fowner;
  private_station_info station_info;
  
};
#endif
