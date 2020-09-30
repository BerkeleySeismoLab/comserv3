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
LD_RUN_PATH = 

########################################################################
# LINUX definitions
INCL	= -I$(CSINCL)
CFLAGS	= -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN)
LDFLAGS	= -m$(NUMBITS) $(DEBUG)
LDLIBS	= $(CSULIB) -lm

########################################################################

ALL =		config dataread dpda msgmon

all:		$(ALL)

config:		config.o $(CSULIB)
		$(CC) $(LDFLAGS) -o config config.o $(CSULIB) $(LDLIBS)

dataread:	dataread.o $(CSULIB)
		$(CC) $(LDFLAGS) -o dataread dataread.o $(CSULIB) $(LDLIBS)

dpda:		dpda.o $(CSULIB)
		$(CC) $(LDFLAGS) -o dpda dpda.o $(CSULIB) $(LDLIBS)

msgmon:		msgmon.o $(CSULIB)
		$(CC) $(LDFLAGS) -o msgmon msgmon.o $(CSULIB) $(LDLIBS)

config.o:	config.c $(CSINCL)/dpstruc.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h $(CSINCL)/stuff.h
		$(CC) -c $(CFLAGS) config.c

dataread.o:	$(CSINCL)/dpstruc.h $(CSINCL)/service.h $(CSINCL)/seedstrc.h $(CSINCL)/stuff.h $(CSINCL)/timeutil.h dataread.c
		$(CC) -c $(CFLAGS) dataread.c

dpda.o:		dpda.c $(CSINCL)/dpstruc.h $(CSINCL)/service.h 	$(CSINCL)/cfgutil.h $(CSINCL)/stuff.h
		$(CC) -c $(CFLAGS) dpda.c

msgmon.o:	msgmon.c $(CSINCL)/dpstruc.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h $(CSINCL)/stuff.h
		$(CC) -c $(CFLAGS) msgmon.c

$(CSULIB):	FORCE
		( cd $(CSUDIR); make -f $(MAKEFILE) )

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) $(BINDIR)

$(BINDIR):
		mkdir $(BINDIR)
