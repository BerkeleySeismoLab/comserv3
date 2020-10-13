########################################################################
#
# Makefile for the libcomserv library.
#
# The makefile in each directory should support the following targets:
#       all
#       clean
#       install
#

MAKEFILE := $(lastword $(MAKEFILE_LIST))
include	../$(MAKEFILE).include

# Ensure desired LP (Long and Pointer) size for compilation has been set.
ifndef NUMBITS
$(error NUMBITS is not set)
endif

#########################################################################
# Set to the location of software on your system
CSDIR	= ..
COMSERV = $(CSDIR)/comserv_src
CSINCL	= $(CSDIR)/include

#########################################################################
# Notes:
# -D_BIG_ENDIAN_HEADER flag means that the headers of MSEED records are
#	going to be stored in BIG ENDIAN byte order (non-native) on
#	LITTLE-ENDIAN OS (Intel Linux)
# SEED standard recommends BIG_ENDIAN MSEED.

########################################################################
# Linux definitions
#
INCL	 = -I$(CSINCL)
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN) 
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT)
RANLIB	 = ranlib 

########################################################################

LIB	 = libcomserv.a

OBJECTS = comserv_subs.o comserv_queue.o csconfig.o buffers.o client_handler.o \
	define_comserv_vars.o Logger.o PacketQueue.o

.PRECIOUS:	$(LIB)

ALL =		$(LIB)

all:		$(ALL)

$(LIB):		$(LIB)($(OBJECTS))
		$(RANLIB) $@

comserv_subs.o:	comserv_subs.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c comserv_subs.c

comserv_queue.o:	comserv_queue.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c comserv_queue.c

client_handler.o: client_handler.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c client_handler.c

csconfig.o:	csconfig.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c csconfig.c

define_comserv_vars.o:	define_comserv_vars.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c define_comserv_vars.c

buffers.o:	$(COMSERV)/buffers.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c buffers.c

Logger.o:	Logger.C
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c Logger.C

PacketQueue.o:	PacketQueue.C
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c PacketQueue.C

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL)
		echo "Built server_subs libs"
