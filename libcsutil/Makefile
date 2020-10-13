########################################################################
#
# Makefile for the comserv util library.
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

########################################################################
# Linux definitions
INCL	 = -I$(CSINCL)
CPPFLAGS = $(INCL) $(OSDEFS) $(ENDIAN)
CFLAGS	 = -m$(NUMBITS) $(DEBUG) $(COPT)
LDFLAGS	 = -m$(NUMBITS) $(DEBUG)
RANLIB	 = ranlib 

########################################################################

LIB	= libcsutil.a

OBJECTS = service.o cfgutil.o stuff.o seedutil.o timeutil.o logging.o portingtools.o

ALL =		$(LIB)

all:		$(ALL)

$(LIB):		$(LIB)($(OBJECTS))
		$(RANLIB) $@

timeutil.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/stuff.h timeutil.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c timeutil.c

seedutil.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/seedstrc.h $(CSINCL)/stuff.h seedutil.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c seedutil.c

service.o:	$(CSINCL)/dpstruc.h $(CSINCL)/service.h $(CSINCL)/stuff.h \
		service.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c service.c

cfgutil.o:	$(CSINCL)/dpstruc.h cfgutil.c $(CSINCL)/stuff.h
		$(CC) $(CFLAGS) $(CPPFLAGS) -c cfgutil.c

stuff.o:	$(CSINCL)/dpstruc.h stuff.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c stuff.c

logging.o:	$(CSINCL)/logging.h logging.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c logging.c

portingtools.o:	portingtools.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -c portingtools.c

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:
