# This Makefile is not needed for the quanterra_comserv package.
# The Makefile for lib660 builds and includes the few files
# required for use with lib660.

ifndef NUMBITS
$(error NUMBITS is not set)
endif

TARGET	= libq660util.a

SRCS =	memutil.c readpackets.c sha256.c ssifc.c\
	xmlcfg.c xmlseed.c xmlsup.c qsockets.c


HDRS =	memutil.h pascal.h platform.h readpackets.h\
	sha256.h ssifc.h utiltypes.h xmlcfg.h\
	xmlseed.h xmlsup.c xmlsup.h qsockets.h


OBJS	= $(SRCS:%.c=%.o)

RANLIB	= ranlib

.PRECIOUS:	$(TARGET)

CC	= gcc
CFLAGS  = -g -DPROG_BE -DPROG_DL -DPROG_SS -DDUMPPACKETS -m$(NUMBITS) -Dlinux
#CFLAGS  = -g -DPROG_BE -DPROG_DL -DPROG_SS\
#	-march=armv7-a -O0 -mfpu=neon -ffast-math -finline-functions\
#	-funswitch-loops -g -DUSE_GCC_PACKING -w  -fmessage-length=0
COPT	= 

$(TARGET)  := CFLAGS += $(COPT)

ALL	=  $(TARGET)

all:	$(ALL)

$(TARGET):	$(TARGET)($(OBJS)) 
	$(RANLIB) $@

clean:
	-rm -f *.o $(ALL)

