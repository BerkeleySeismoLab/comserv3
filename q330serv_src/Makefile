########################################################################
#
# Makefile for the q8serv server.
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
CSSRC	= $(CSDIR)/comserv_src
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
CSULIB	= $(CSUDIR)/libcsutil.a
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
SSDIR	= $(CSDIR)/libcomserv
LIB330DIR  = $(CSDIR)/lib330
CSULIB	= $(CSUDIR)/libcsutil.a
SSLIB	= $(SSDIR)/libcomserv.a
LIB330LIB = $(LIB330DIR)/lib330.a

########################################################################
# Linux definitions
LD	 = $(CXX)
INCL	 = -I$(LIB330DIR) -I$(SSDIR) -I$(CSINCL)
L330DEFS = -DUSE_GCC_PACKING
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN) $(L330DEFS) -DMYIPADDR
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT)
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)
LDLIBS	 = $(SSLIB) $(LIB330LIB) $(CSULIB) -lpthread

########################################################################

program =  q330serv

ALL	= $(program)

CXXfiles = q330serv.C ConfigVO.C Verbose.C ReadConfig.C lib330Interface.C # PacketQueue.C
Cfiles =  q330servcfg.c

headers = q330serv.h ConfigVO.h Verbose.h ReadConfig.h global.h \
          lib330Interface.h q330servcfg.h # PacketQueue.h

files = $(CXXFiles) $(Cfiles)
sourcefiles = $(files) $(headers)

CXXobjects = $(CXXfiles:.C=.o)
Cobjects = $(Cfiles:.c=.o)
objects = $(CXXobjects) $(Cobjects)

all:		$(ALL)

$(program): 	$(objects) $(sourcefiles) $(SSLIB) $(LIB330LIB) $(CSULIB)
		$(LD) $(LDFLAGS) -o $@ $(objects) $(LDLIBS)

%.o:		%.C
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

%.o:		$.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(CSULIB):	FORCE
		( cd $(CSUDIR); make -f $(MAKEFILE) )

$(SSLIB):	FORCE
		( cd $(SSDIR); make -f $(MAKEFILE) )

$(LIB330LIB):	FORCE
		( cd $(LIB330DIR); make -f $(MAKEFILE) )

FORCE:		

clean:
		-rm -f *.o *~ core core.* $(ALL)

install: 	$(ALL) $(BINDIR)
		cp -p $(ALL) $(BINDIR)		

$(BINDIR):
		mkdir $(BINDIR)
