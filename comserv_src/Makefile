########################################################################
#
# Makefile for the comserv server.
#
# The makefile in each directory should support the following targets:
#       all
#       clean
#       install
#

MAKEFILE := $(lastword $(MAKEFILE_LIST))
include ../$(MAKEFILE).include

# Ensure desired LP (Long and Pointer) size for compilation has been set.
ifndef NUMBITS
$(error NUMBITS is not set)
endif

#########################################################################
# Set to the location of software on your system
CSDIR	= ..
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
CSULIB	= $(CSUDIR)/libcsutil.a

#########################################################################
# Notes:
# -D_BIG_ENDIAN_HEADER flag means that the headers of MSEED records are
#	going to be stored in BIG ENDIAN byte order (non-native) on
#	LITTLE-ENDIAN OS (Intel Linux)
# SEED standard recommends BIG_ENDIAN MSEED.

########################################################################
# Linux definitions
LD	= $(CC)
CPPFLAGS = -I$(CSINCL) $(OSDEFS) $(ENDIAN) -DMYIPADDR 
CFLAGS	= -m$(NUMBITS) $(DEBUG) $(COPT) 
LDFLAGS	= -m$(NUMBITS) $(DEBUG)
LDLIBS	= $(CSULIB)

########################################################################

program	= comserv

ALL	= $(program)

all:		$(ALL)

$(program):	server.o cscfg.o buffers.o comlink.o commands.o $(CSULIB)
		$(LD) $(LDFLAGS) -o comserv server.o cscfg.o buffers.o \
		comlink.o commands.o $(LDLIBS)

server.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/cfgutil.h $(CSINCL)/stuff.h $(CSINCL)/server.h server.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c server.c

cscfg.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/cfgutil.h $(CSINCL)/stuff.h $(CSINCL)/server.h cscfg.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c cscfg.c

buffers.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/stuff.h $(CSINCL)/server.h buffers.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c buffers.c

comlink.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/stuff.h $(CSINCL)/server.h comlink.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c comlink.c

commands.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/stuff.h $(CSINCL)/server.h commands.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c commands.c

$(CSULIB):	FORCE
		( cd $(CSUDIR); make -f $(MAKEFILE) )

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) $(BINDIR)
 
$(BINDIR):
		mkdir $(BINDIR)
