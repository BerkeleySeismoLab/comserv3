########################################################################
#
# Makefile for UCB client cs2ringserver
#
# The makefile in each directory should support the following targets:
#       all
#       clean
#       install
#

MAKEFILE := $(lastword $(MAKEFILE_LIST))
include ../../$(MAKEFILE).include

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
INCL	 = -I$(CSINCL) -I$(QLIB2_INCL) -I$(DALI_INCL)
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN)
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT) 
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)
LDLIBS	 = -m$(NUMBITS) $(DEBUG) $(CSULIB) $(QLIB2_LIB) $(DALI_LIB) -lm

########################################################################

P1 = cs2ringserver

SRCS1 	= $(P1).c comserv_subs.c scnl_convert.c socket_subs.c

OBJS1	= $(SRCS1:.c=.o)

ALL	= $(P1) 

all:		$(ALL)

$(P1):		$(OBJS1) $(CSULIB)
		$(CC) $(LDFLAGS) -o $@ $(OBJS1) $(LDLIBS)

cs2ringserver.o:	cs2ringserver.c \
		channel_info.h datasock_codes.h basics.h \
		$(CSINCL)/dpstruc.h $(CSINCL)/seedstrc.h $(CSINCL)/stuff.h \
		$(CSINCL)/timeutil.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h 
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

comserv_subs.o:		comserv_subs.c \
		channel_info.h datasock_codes.h basics.h \
		$(CSINCL)/dpstruc.h $(CSINCL)/seedstrc.h $(CSINCL)/stuff.h \
		$(CSINCL)/timeutil.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h 
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

scnl_convert.o:	scnl_convert.c 	scnl_convert.h
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

socket_subs.o:	socket_subs.c \
		channel_info.h datasock_codes.h basics.h \
		$(CSINCL)/dpstruc.h $(CSINCL)/seedstrc.h $(CSINCL)/stuff.h \
		$(CSINCL)/timeutil.h $(CSINCL)/service.h $(CSINCL)/cfgutil.h 
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(CSULIB):	FORCE
		(cd $(CSUDIR); make -f $(MAKEFILE))

FORCE:

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:	$(ALL) $(BINDIR)
		cp -p $(ALL) $(BINDIR)

$(BINDIR):
		mkdir $(BINDIR)
