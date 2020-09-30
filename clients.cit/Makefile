########################################################################
#
# Master Makefile for the CIT comserv clients.
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

########################################################################

TARGETS	=  cs2mcast csstat 

all:
	for dir in $(TARGETS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) all ) ; \
	done

clean:
	for dir in $(TARGETS) ; do \
		echo clean for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) clean ) ; \
	done

install:	all $(BINDIR)
	for dir in $(TARGETS) ; do \
		echo install for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) BINDIR=../$(BINDIR) install ) ; \
	done

$(BINDIR):
	mkdir $(BINDIR)
