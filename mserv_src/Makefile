########################################################################
#
# Makefile for the mserv server.
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
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
CSULIB	= $(CSUDIR)/libcsutil.a
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
SSDIR	= $(CSDIR)/libcomserv
LIBMSMCASTDIR  = $(CSDIR)/libmsmcast
CSULIB	= $(CSUDIR)/libcsutil.a
SSLIB	= $(SSDIR)/libcomserv.a
LIBMSMCASTLIB = $(LIBMSMCASTDIR)/libmsmcast.a

########################################################################
# Linux definitions  IGD
LD	 = $(CXX)
INCL	 = -I$(LIBMSMCASTDIR) -I$(SSDIR) -I$(CSINCL) 
LIBMSMCASTDEFS = -DUSE_GCC_PACKING
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN) $(LIBMSMCASTDEFS) -DMYIPADDR
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT)
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)
LDLIBS	 = $(SSLIB) $(LIBMSMCASTLIB) $(CSULIB) -lpthread

########################################################################

program	=  mserv

CXXfiles = mserv.C ConfigVO.C Verbose.C ReadConfig.C libmsmcastInterface.C 
Cfiles	=  mservcfg.c

headers = mserv.h ConfigVO.h Verbose.h ReadConfig.h global.h \
          lib330Interface.h mservcfg.h

sources = $(headers) $(files)
CXXobjects = $(CXXfiles:.C=.o)

cobjects = $(Cfiles:.c=.o)
objects = $(CXXobjects) $(cobjects)

ALL	= $(program)

all:		$(program)

$(program):	$(objects) $(SSLIB) $(CSULIB) $(LIB660LIB)
		$(LD) $(LDFLAGS) -o $@ $(objects) $(LDLIBS) 

%.o:		%.C
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

%.o:		$.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(CSULIB):	FORCE
		( cd $(CSUDIR); make -f $(MAKEFILE) )

$(SSLIB):	FORCE
		( cd $(SSDIR); make -f $(MAKEFILE) )

$(LIB660LIB):	FORCE
		( cd $(LIB660DIR); make -f $(MAKEFILE) )

FORCE:		

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) $(BINDIR)		

$(BINDIR):
		mkdir $(BINDIR)
