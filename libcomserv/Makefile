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
INCL	= -I$(CSINCL)
CFLAGS	= -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN) 
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN)
RANLIB	= ranlib 

########################################################################

LIB	 = libcomserv.a

# This library includes .o files from 2 external source code files:
#	$(COMSERV)/buffers.c

OBJECTS = comserv_subs.o comserv_queue.o csconfig.o buffers.o client_handler.o \
	define_comserv_vars.o Logger.o PacketQueue.o

.PRECIOUS:	$(LIB)

ALL =		$(LIB)

all:		$(ALL)

$(LIB):		$(LIB)($(OBJECTS))
		$(RANLIB) $@

comserv_subs.o:	comserv_subs.c
		$(CC) -c $(CFLAGS) comserv_subs.c

comserv_qeue.o:	comserv_queue.c
		$(CC) -c $(CFLAGS) comserv_queue.c

client_handler.o: client_handler.c
		$(CC) -c $(CFLAGS) client_handler.c

csconfig.o:	csconfig.c
		$(CC) -c $(CFLAGS) csconfig.c

define_comserv_vars.o:	define_comserv_vars.c
		$(CC) -c $(CFLAGS) define_comserv_vars.c

buffers.o:	$(COMSERV)/buffers.c
		$(CC) -c $(CFLAGS) $(COMSERV)/buffers.c

Logger.o:	Logger.C
		$(CXX) -c $(CXXFLAGS) Logger.C

PacketQueue.o:	PacketQueue.C
		$(CXX) -c $(CXXFLAGS) PacketQueue.C

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL)
		echo "Built server_subs libs"
