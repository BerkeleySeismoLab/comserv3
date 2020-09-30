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
INCL	= -I$(CSINCL)
CFLAGS	= -m$(NUMBITS) $(DEBUG) $(COPT) $(INCL) $(OSDEFS) $(ENDIAN)
RANLIB	= ranlib 

########################################################################

LIB	= libcsutil.a

OBJECTS = service.o cfgutil.o stuff.o seedutil.o timeutil.o logging.o portingtools.o

ALL =		$(LIB)

all:		$(ALL)

$(LIB):		$(LIB)($(OBJECTS))
		$(RANLIB) $@

timeutil.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/stuff.h timeutil.c
		$(CC) -c $(CFLAGS) timeutil.c

seedutil.o:	$(CSINCL)/dpstruc.h $(CSINCL)/quanstrc.h $(CSINCL)/service.h \
		$(CSINCL)/seedstrc.h $(CSINCL)/stuff.h seedutil.c
		$(CC) -c $(CFLAGS) seedutil.c

service.o:	$(CSINCL)/dpstruc.h $(CSINCL)/service.h $(CSINCL)/stuff.h \
		service.c
		$(CC) -c $(CFLAGS) service.c

cfgutil.o:	$(CSINCL)/dpstruc.h cfgutil.c $(CSINCL)/stuff.h
		$(CC) -c $(CFLAGS) cfgutil.c

stuff.o:	$(CSINCL)/dpstruc.h stuff.c
		$(CC) -c $(CFLAGS) stuff.c

logging.o:	$(CSINCL)/logging.h logging.c
		$(CC) -c $(CFLAGS) logging.c

portingtools.o:	portingtools.c
		$(CC) -c $(CFLAGS) portingtools.c

clean:
		-rm -f *.o *~ core core.* $(ALL)

install:
