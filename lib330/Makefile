########################################################################
#
# Makefile for lib330
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

TARGET	= lib330.a

SRCS =	libarchive.c libclient.c libcmds.c libcompress.c libcont.c \
	libctrldet.c libcvrt.c libdetect.c libdss.c libfilters.c \
	liblogs.c libmd5.c libmsgs.c libnetserv.c libopaque.c \
	libpoc.c libsampcfg.c libsample.c libseed.c libslider.c \
	libstats.c libstrucs.c libsupport.c libtokens.c libtypes.c \
	libverbose.c q330cvrt.c q330io.c

HDRS =	libarchive.h libclient.h libcmds.h libcompress.h libcont.h \
	libctrldet.h libcvrt.h libdetect.h libdss.h libfilters.h \
	liblogs.h libmd5.h libmsgs.h libnetserv.h libopaque.h \
	libpoc.h libsampcfg.h libsampglob.h libsample.h libseed.h libslider.h \
	libstats.h libstrucs.h libsupport.h libtokens.h libtypes.h \
	libverbose.h pascal.h platform.h q330cvrt.h q330io.h q330types.h

OBJS	= $(SRCS:%.c=%.o)

RANLIB	= ranlib

.PRECIOUS:	$(TARGET)

CC	= gcc
CFLAGS  = -m$(NUMBITS) -Dlinux -DUSE_GCC_PACKING $(DEBUG) # -DBSL_Q330_TCP_DEBUG
DEBUG	= -g
COPT	= 


$(TARGET) := CFLAGS += $(COPT)

ALL	=  $(TARGET)

all:		$(ALL)

$(TARGET):	$(TARGET)($(OBJS))
		$(RANLIB) $@


clean:
		-rm -f *.o *~ core core.* $(ALL)
