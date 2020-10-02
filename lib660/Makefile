########################################################################
#
# Makefile for lib660
#
# The makefile in each directory should support the following targets:
#       all
#       clean
#       install
#

MAKEFILE := $(lastword $(MAKEFILE_LIST))
include	../$(MAKEFILE).include

ifndef NUMBITS
$(error NUMBITS is not set)
endif

TARGET	= lib660.a
#OPTIMIZE = NORMAL
OPTIMIZE = DEBUG

QD	= ../q660util

SRCS =	libarchive.c libclient.c libcmds.c libcompress.c libcont.c\
	libdata.c libdataserv.c libdetect.c libfilters.c liblogs.c\
	libmsgs.c libopaque.c libpoc.c libsampcfg.c libsample.c\
	libseed.c libstats.c libstrucs.c libsupport.c libverbose.c\
	$(QD)/memutil.c $(QD)/readpackets.c $(QD)/sha256.c\
	$(QD)/xmlcfg.c $(QD)/xmlseed.c $(QD)/xmlsup.c


HDRS =	libarchive.h libclient.h libcmds.h libcompress.h\
	libcont.h libdata.h libdataserv.h libdetect.h\
	libfilters.h liblogs.h libmsgs.h libopaque.h\
	libpoc.h libsampcfg.h libsampglob.h libsample.h\
	libseed.h libstats.h libstrucs.h libsupport.h libtypes.h libverbose.h\
	$(QD)/memutil.h $(QD)/pascal.h $(QD)/platform.h\
	$(QD)/readpackets.h $(QD)/sha256.h $(QD)/ssifc.h $(QD)/utiltypes.h\
	$(QD)/xmlcfg.h $(QD)/xmlseed.h $(QD)/xmlsup.h Makefile


OBJS	= $(SRCS:%.c=%.o)

RANLIB	= ranlib

.PRECIOUS:	$(TARGET)

CC	= gcc

ifeq ($(OPTIMIZE),NORMAL)
CFLAGS  = -g -I $(QD) -DPROG_DL -DPROG_SS -DINLIB660 -DDEVSOH\
	-O2 -ffast-math -finline-functions\
	-funswitch-loops -g -DUSE_GCC_PACKING -w  -fmessage-length=0 \
	-m$(NUMBITS) -Dlinux -DBSL # -DBSL_DEBUG 
endif

ifeq ($(OPTIMIZE),DEBUG)
CFLAGS  = -g -I $(QD) -DPROG_DL -DPROG_SS -DINLIB660 -DDEVSOH\
	-O0 -ffast-math\
	-g -DUSE_GCC_PACKING -w \
	-m$(NUMBITS) -Dlinux -DBSL -DBSL_DEBUG 
endif

COPT	= 

$(TARGET)  := CFLAGS += $(COPT)

ALL	=  $(TARGET)

all:	$(ALL)

$(TARGET):	$(TARGET)($(OBJS)) $(HDRS)
	$(RANLIB) $@

clean:
	-rm -f *.o $(ALL)
