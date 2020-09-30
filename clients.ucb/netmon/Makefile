########################################################################
#
# Makefile for UCB client netmon
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
LD_RUN_PATH = 

########################################################################
# LINUX definitions
INCL	= -I$(CSINCL) -I$(QLIB2_INCL)
CFLAGS	= -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN)
LDLIBS	= -m$(NUMBITS) $(DEBUG) $(CSULIB) $(QLIB2_LIB) -lm

########################################################################

P1 = netmon

SRCS1 	= $(P1).c
OBJS1	= $(SRCS1:.c=.o)

ALL	= $(P1) 

all:		$(ALL)

$(P1):		$(OBJS1) $(CSULIB)
		$(CC) $(CFLAGS) -o $@ $(OBJS1) $(LDLIBS)

netmon.o:	netmon.c \
		$(CSINCL)/dpstruc.h $(CSINCL)/stuff.h \
		$(CSINCL)/service.h $(CSINCL)/cfgutil.h 
		$(CC) $(CFLAGS) -c $< -DALLOW_RECONFIG

$(CSULIB):	FORCE
		(cd $(CSUDIR); make -f $(MAKEFILE))

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL) run_netmon $(BINDIR)
		cp -p $(ALL) run_netmon $(BINDIR)
		chmod u+s $(BINDIR)/netmon

$(BINDIR):
		mkdir $(BINDIR)
