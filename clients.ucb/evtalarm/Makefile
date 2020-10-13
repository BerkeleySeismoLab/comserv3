########################################################################
#
# Makefile for UCB client evtalarm
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

########################################################################
# LINUX definitions
INCL	 = -I$(CSINCL) -I$(QLIB2_INCL)
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN)
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)
LDLIBS	 = -m$(NUMBITS) $(DEBUG) $(CSULIB) $(QLIB2_LIB) -lm

########################################################################

P1 = evtalarm

SRCS1 	= $(P1).c
OBJS1	= $(SRCS1:.c=.o)

ALL	= $(P1) 

all:		$(ALL)

$(P1):		$(OBJS1) $(CSULIB)
		$(CC) $(LDFLAGS) -o $@ $(OBJS1) $(LDLIBS)

evtalarm.o:	evtalarm.c \
		$(CSINCL)/dpstruc.h $(CSINCL)/seedstrc.h $(CSINCL)/stuff.h \
		$(CSINCL)/timeutil.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h 
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(CSULIB):	FORCE
		(cd $(CSUDIR); make -f $(MAKEFILE))

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL)
		cp -p $(ALL) $(BINDIR)

$(BINDIR):
		mkdir $(BINDIR)
