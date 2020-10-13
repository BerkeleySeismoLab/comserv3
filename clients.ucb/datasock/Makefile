########################################################################
#
# Makefile for UCB client datasock
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
INCL	 = -I$(CSINCL) -I$(QLIB2_INCL)
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN)
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)
LDLIBS	 = $(CSULIB) $(QLIB2_LIB) -lm

########################################################################

P1 = datasock

SRCS1 	= $(P1).c bld_statchans.c cs_station_list.c read_socket_request.c set_selectors.c
HDRS1	= bld_statchans.h cs_station_list.h datasock_codes.h read_socket_request.h set_selectors.h statchan.h
OBJS1	= $(SRCS1:.c=.o)

ALL	= $(P1) 

all:		$(ALL)

$(P1):		$(OBJS1) $(CSULIB)
		$(CC) $(LDFLAGS) -o $@ $(OBJS1) $(LDLIBS)

%.o:		%.c $(HDRS1)
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
