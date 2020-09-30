########################################################################
#
# Makefile for the CIT comserv client csstat.
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
SSDIR	= $(CSDIR)/libcomserv
SSLIB	= $(SSDIR)/libcomserv.a

########################################################################
# Linux definitions
#
LD	 = $(CXX)
INCL	 = -I$(CSINCL) -I$(SSDIR) -I$(QLIB2_INCL)
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN)
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN)
LDLIBS	 = $(CSULIB) $(SSLIB) $(QLIB2_LIB) -lm

########################################################################

files = chan_stat.C cs_site_map.C csstat.C

headers = RetCodes.h cpascal.h un_pascal.h

sources = $(headers) $(files)
objects = $(files:.C=.o)

program = csstat

ALL	= $(program)

all: 		$(ALL)

$(program):	$(objects) $(files) $(CSULIB) $(SSLIB)
		$(LD) $(CXXFLAGS) $(objects) -o $@ $(LDLIBS)

#implicit rules for converting C and c files to object files
#
.C.o:
		$(CXX) $(CXXFLAGS) -c $<

$(CSULIB):	FORCE
		( cd $(CSUDIR); make -f $(MAKEFILE) )

$(SSLIB):	FORCE
		( cd $(SSDIR); make -f $(MAKEFILE) )

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL) 

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) ${BINDIR}

$(BINDIR):
		mkdir $(BINDIR)
