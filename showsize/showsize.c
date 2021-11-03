#include <stdio.h>

#define pascal_h

#include <linux/limits.h>
#include "platform.h"

/* PROG_DL needs to be defined to include proper parts of q660util definitions. */
#ifndef	PROG_DL
#define PROG_DL
#endif

#include "libclient.h"
#include "libtypes.h"
#include "libmsgs.h"
#include "libsupport.h"
#include "libstrucs.h"

#include "xmlcfg.h"
#include "xmlseed.h"

//  #ifdef USE_GCC_PACKING
//  enum tpacket_class {PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE}  __attribute__ ((__packed__)) ;
//  #else
//  enum tpacket_class {PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE} ;
//  #endif

//  enum tminiseed_action {MSA_512, /* new 512 byte packet */
//                        MSA_ARC, /* new archival packet, non-incremental */
//                        MSA_FIRST, /* new archival packet, incremental */
//                        MSA_INC, /* incremental update to archival packet */
//                        MSA_FINAL, /* final incremental update */
//                        MSA_GETARC, /* request for last archival packet written */
//                        MSA_RETARC} ; /* client is returning last packet written */

//  typedef struct { /* format for miniseed and archival miniseed */
//    tcontext context ;
//    string9 station_name ; /* network and station */
//    string2 location ;
//    byte chan_number ; /* channel number according to tokens */
//    string3 channel ;
//    integer rate ; /* sampling rate */
//    double timestamp ; /* Time of data, corrected for any filtering */
//    word filter_bits ; /* OMF_xxx bits */
//    enum tpacket_class packet_class ; /* type of record */
//    enum tminiseed_action miniseed_action ; /* what this packet represents */
//    word data_size ; /* size of actual miniseed data */
//    pointer data_address ; /* pointer to miniseed record */
//  } tminiseed_call ;

int main (int argc, char **argv) 
{
    /* Object we want to check size of based on gcc compilation options for packing. */

    enum tpacket_class packet_class ; /* type of record */
    enum tminiseed_action miniseed_action ; /* type of record */
    tminiseed_call miniseed_call ; /* buffer for building miniseed callbacks */
    
    
    printf ("########################################\n");
#ifdef USE_GCC_PACKING
    printf ("Program compiled WITH USE_GCC_PACKING\n");
#else
    printf ("Program compiled WITHOUT USE_GCC_PACKING\n");
#endif
    printf ("########################################\n");

#ifdef USE_GCC_PACKING
    char *tpacket_class_string = "\
enum tpacket_class {PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE}  __attribute__ ((__packed__)) ;";
#else
    char *tpacket_class_string = "\
enum tpacket_class {PKC_DATA, PKC_EVENT, PKC_CALIBRATE, PKC_TIMING, PKC_MESSAGE, PKC_OPAQUE} ;";
#endif

    char *tminiseed_action_string = "\
enum tminiseed_action {MSA_512, /* new 512 byte packet */\n\
                      MSA_ARC, /* new archival packet, non-incremental */\n\
                      MSA_FIRST, /* new archival packet, incremental */\n\
                      MSA_INC, /* incremental update to archival packet */\n\
                      MSA_FINAL, /* final incremental update */\n\
                      MSA_GETARC, /* request for last archival packet written */\n\
                      MSA_RETARC} ; /* client is returning last packet written */n\
";

    char *tminiseed_call_string = "\
typedef struct { /* format for miniseed and archival miniseed */\n\
  tcontext context ;\n\
  string9 station_name ; /* network and station */\n\
  string2 location ;\n\
  byte chan_number ; /* channel number according to tokens */\n\
  string3 channel ;\n\
  integer rate ; /* sampling rate */\n\
  double timestamp ; /* Time of data, corrected for any filtering */\n\
  word filter_bits ; /* OMF_xxx bits */\n\
  enum tpacket_class packet_class ; /* type of record */\n\
  enum tminiseed_action miniseed_action ; /* what this packet represents */\n\
  word data_size ; /* size of actual miniseed data */\n\
  pointer data_address ; /* pointer to miniseed record */\n\
} tminiseed_call ;\
";

    printf ("\n");    

    printf ("%s\n", tpacket_class_string);
    printf ("\n");
    printf ("%s\n", tminiseed_action_string);
    printf ("\n");
    printf ("%s\n", tminiseed_call_string);
    printf ("\n");

    printf ("sizeof(enum packet_class) = %d\n", sizeof(enum tpacket_class));
    printf ("sizeof(enum tminiseed_action) = %d\n", sizeof(enum tminiseed_action));
    printf ("sizeof(tminiseed_call) = %d\n", sizeof(tminiseed_call));
    printf ("\n");

    printf ("offset context = %d\t\tsizeof(context) = %d\n",
	    (void *)&miniseed_call.context - (void *)&miniseed_call, sizeof(miniseed_call.context));
    printf ("offset station_name = %d\t\tsizeof(station_name) = %d\n",
	    (void *)&miniseed_call.station_name - (void *)&miniseed_call, sizeof(miniseed_call.station_name));
    printf ("offset location = %d\t\tsizeof(location) = %d\n",
	    (void *)&miniseed_call.location - (void *)&miniseed_call, sizeof(miniseed_call.location));
    printf ("offset chan_number = %d\t\tsizeof(chan_number) = %d\n",
	    (void *)&miniseed_call.chan_number - (void *)&miniseed_call, sizeof(miniseed_call.chan_number));
    printf ("offset channel = %d\t\tsizeof(channel) = %d\n",
	    (void *)&miniseed_call.channel - (void *)&miniseed_call, sizeof(miniseed_call.channel));
    printf ("offset rate = %d\t\tsizeof(rate) = %d\n",
	    (void *)&miniseed_call.rate - (void *)&miniseed_call, sizeof(miniseed_call.rate));
    printf ("offset timestamp = %d\t\tsizeof(timestamp) = %d\n",
	    (void *)&miniseed_call.timestamp - (void *)&miniseed_call, sizeof(miniseed_call.timestamp));
    printf ("offset filter_bits = %d\t\tsizeof(filter_bits) = %d\n",
	    (void *)&miniseed_call.filter_bits - (void *)&miniseed_call, sizeof(miniseed_call.filter_bits));
    printf ("offset packet_class = %d\tsizeof(packet_class) = %d\n",
	    (void *)&miniseed_call.packet_class - (void *)&miniseed_call, sizeof(miniseed_call.packet_class));
   printf ("offset miniseed_action = %d\tsizeof(miniseed_action) = %d\n",
	   (void *)&miniseed_call.miniseed_action - (void *)&miniseed_call, sizeof(miniseed_call.miniseed_action));
    printf ("offset data_size = %d\t\tsizeof(data_size) = %d\n",
	    (void *)&miniseed_call.data_size - (void *)&miniseed_call, sizeof(miniseed_call.data_size));
    printf ("offset data_address = %d\tsizeof(data_address) = %d\n",
	    (void *)&miniseed_call.data_address - (void *)&miniseed_call, sizeof(miniseed_call.data_address));
    printf ("\n");

    return 0;
}
