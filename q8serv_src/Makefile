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
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
CSULIB	= $(CSUDIR)/libcsutil.a
CSINCL	= $(CSDIR)/include
CSUDIR	= $(CSDIR)/libcsutil
SSDIR	= $(CSDIR)/libcomserv
LIB660DIR  = $(CSDIR)/lib660
Q660UTILDIR = $(CSDIR)/q660util
CSULIB	= $(CSUDIR)/libcsutil.a
SSLIB	= $(SSDIR)/libcomserv.a
LIB660LIB = $(LIB660DIR)/lib660.a

########################################################################
# Linux definitions  IGD
LD	 = $(CXX)
INCL	 = -I$(LIB660DIR) -I$(Q660UTILDIR) -I$(SSDIR) -I$(CSINCL) 
L660DEFS = -DUSE_GCC_PACKING -DBSL -DLIB660_BETA  # -DDEBUG_Lib660Interface 
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN) $(L660DEFS) -DMYIPADDR
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
CXXFLAGS = -m$(NUMBITS) $(DEBUG) $(COPT) -std=c++11
LDFLAGS	 = -m$(NUMBITS) $(DEBUG) 
LDLIBS	 = $(SSLIB) $(LIB660LIB) $(CSULIB) -lpthread
# NOTE: Do not need to build or load libq660util.a since the required
#	file from q660util are included in lib660.a by lib660's Makefile.

########################################################################

program	=  q8serv

CXXfiles = q8serv.C ConfigVO.C Verbose.C ReadConfig.C lib660Interface.C
Cfiles	=  q8servcfg.c

headers = q8serv.h ConfigVO.h Verbose.h ReadConfig.h global.h \
          lib330Interface.h file_callback.h q8servcfg.h

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

%.o:		%.c
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
