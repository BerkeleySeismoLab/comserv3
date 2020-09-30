########################################################################
#
# Makefile for UCB client cpick_card_server
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

P1 = cpick_card_server

SRCS1 	= $(P1).c syserr.c
OBJS1	= $(SRCS1:.c=.o)

ALL	= $(P1) 

all:		$(ALL)

$(P1):		$(OBJS1) $(CSULIB)
		$(CC) $(CFLAGS) -o $@ $(OBJS1) $(LDLIBS)

cpick_card_server.o:	cpick_card_server.c \
		$(CSINCL)/dpstruc.h $(CSINCL)/seedstrc.h $(CSINCL)/stuff.h \
		$(CSINCL)/timeutil.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h 
		$(CC) $(CFLAGS) -c $<

$(CSULIB):	FORCE
		(cd $(CSUDIR); make -f $(MAKEFILE))

FORCE:

clean:
		-rm *.o *~ core core.* $(ALL)

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) $(BINDIR)

$(BINDIR):
		mkdir $(BINDIR)

