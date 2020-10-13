########################################################################
#
# Makefile for the CIT comserv client cs2mcast.
#
# The makefile in each directory should support the following targets:
#       all
#       clean
#       install
#

MAKEFILE := $(lastword $(MAKEFILE_LIST))
include	../../$(MAKEFILE).include

# Ensure desired LP (Long and Pointer) size for compilation has been set.
ifndef NUMBITS
$(error NUMBITS is not set)
endif

#########################################################################
# Set to the location of software on your system
CSDIR	= ../..
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
CSULIB	= $(CSUDIR)/libcsutil.a
LDLIBS	= $(CSULIB)

########################################################################
# Linux definitions
#
LD	 = $(CXX)
INCL	 = -I$(CSINCL)
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN)
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT)
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)

########################################################################

srcfiles = cs2mcast.C multicast_utils.C

headers = RetCodes.h

files	= $(srcfiles) $(headers)
objects = $(srcfiles:.C=.o)

program = cs2mcast

ALL	= $(program)

all: 		$(ALL)

$(program):	$(objects) $(CSULIB)
		$(LD) $(LDFLAGS) $(objects) -o $@ $(LDLIBS)

%.o:		%.C $(headers)
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

$(CSULIB):	FORCE
		( cd $(CSUDIR); make -f $(MAKEFILE) )

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) ${BINDIR}

$(BINDIR):
		mkdir $(BINDIR)
