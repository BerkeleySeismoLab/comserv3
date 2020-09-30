########################################################################
#
# Master Makefile for the comserv system servers and clients.
#
# The makefile in each directory should support the following targets:
#       all
#       clean
#       install
#

MAKEFILE := $(lastword $(MAKEFILE_LIST))
include	$(MAKEFILE).include

BINDIR	= bin.$(NUMBITS)

# Ensure desired LP (Long and Pointer) size for compilation has been set.
ifndef NUMBITS
$(error NUMBITS is not set)
endif

########################################################################

TARGETS	=  mserv q330serv q8serv clients
OBSOLETE = comserv 

all:
	for target in $(TARGETS) ; do \
		echo build for $${target} ... ; \
		( make -f $(MAKEFILE) $${target} ) ; \
	done

clean:
	for target in $(TARGETS) ; do \
		echo build for $${target}_clean ... ; \
		( make -f $(MAKEFILE) $${target}_clean ) ; \
	done

install:	all $(BINDIR)
	for target in $(TARGETS) ; do \
		echo build for $${target}_install ... ; \
		( make -f $(MAKEFILE) $${target}_install ) ; \
	done

########################################################################
# q8serv

Q8SERV_EXTERNAL_LIBS	= lib660
Q8SERV_SRC_DIRS		= libcomserv libcsutil q8serv_src
Q8SERV_DIRS		= q8serv_src
Q8SERV_ALL		= $(Q8SERV_EXTERNAL_LIBS) $(Q8SERV_SRC_DIRS)
Q8SERV_INSTALL		= $(Q8SERV_DIRS)

q8serv: 	
	for dir in $(Q8SERV_EXTERNAL_LIBS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) ) ; \
	done
	for dir in $(Q8SERV_SRC_DIRS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) all ) ; \
	done

q8serv_clean:
	for dir in $(Q8SERV_ALL) ; do \
		echo clean for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) clean ) ; \
	done

q8serv_install:	q8serv $(BINDIR)
	for dir in $(Q8SERV_DIRS) ; do \
		echo install for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) BINDIR=../$(BINDIR) install ) ; \
	done

########################################################################
# q330serv

Q330SERV_EXTERNAL_LIBS	= lib330
Q330SERV_SRC_DIRS	= libcomserv libcsutil q330serv_src
Q330SERV_DIRS		= q330serv_src
Q330SERV_ALL		= $(Q330SERV_EXTERNAL_LIBS) $(Q330SERV_SRC_DIRS)
Q330SERV_INSTALL		= $(Q330SERV_DIRS)

q330serv: 	
	for dir in $(Q330SERV_EXTERNAL_LIBS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) ) ; \
	done
	for dir in $(Q330SERV_SRC_DIRS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) all ) ; \
	done

q330serv_clean:
	for dir in $(Q330SERV_ALL) ; do \
		echo clean for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) clean ) ; \
	done

q330serv_install:	q330serv $(BINDIR)
	for dir in $(Q330SERV_DIRS) ; do \
		echo install for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) BINDIR=../$(BINDIR) install ) ; \
	done

########################################################################
# mserv

MSERV_EXTERNAL_LIBS	= libmsmcast
MSERV_SRC_DIRS		= libcsutil mserv_src
MSERV_DIRS		= mserv_src
MSERV_ALL		= $(MSERV_EXTERNAL_LIBS) $(MSERV_SRC_DIRS)
MSERV_INSTALL		= $(MSERV_DIRS)

mserv: 	
	for dir in $(MSERV_EXTERNAL_LIBS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) ) ; \
	done
	for dir in $(MSERV_SRC_DIRS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) all ) ; \
	done

mserv_clean:
	for dir in $(MSERV_ALL) ; do \
		echo clean for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) clean ) ; \
	done

mserv_install:	 mserv $(BINDIR)
	for dir in $(MSERV_DIRS) ; do \
		echo install for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) BINDIR=../$(BINDIR) install ) ; \
	done

########################################################################
# comserv

COMSERV_EXTERNAL_LIBS	= 
COMSERV_SRC_DIRS	= libcsutil comserv_src
COMSERV_DIRS		= comserv_src
COMSERV_ALL		= $(COMSERV_EXTERNAL_LIBS) $(COMSERV_SRC_DIRS)
COMSERV_INSTALL		= $(COMSERV_DIRS)

comserv: 	
	for dir in $(COMSERV_EXTERNAL_LIBS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) ) ; \
	done
	for dir in $(COMSERV_SRC_DIRS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) all ) ; \
	done

comserv_clean:
	for dir in $(COMSERV_ALL) ; do \
		echo clean for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) clean ) ; \
	done

comserv_install:	 comserv $(BINDIR)
	for dir in $(COMSERV_DIRS) ; do \
		echo install for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) BINDIR=../$(BINDIR) install ) ; \
	done

########################################################################
# clients

CLIENTS_EXT_LIBS	= qlib2
CLIENTS_SRC_DIRS	= libcsutil clients.quanterra clients.ucb clients.cit
CLIENTS_DIRS		= clients.quanterra clients.ucb clients.cit
CLIENTS_ALL		= $(CLIENTS_SRC_DIRS)

clients:
#	for dir in $(CLIENTS_EXT_LIBS) ; do \
#		echo build for $$dir ... ; \
#		( cd $$dir ; make -f $(MAKEFILE) ) ; \
#	done
	for dir in $(CLIENTS_SRC_DIRS) ; do \
		echo build for $$dir ... ; \
		( cd $$dir ; make -f $(MAKEFILE) all ) ; \
	done

clients_clean:
	for dir in $(CLIENTS_ALL) ; do \
		( cd $$dir ; make -f $(MAKEFILE) clean ) ; \
	done

clients_install:	clients $(BINDIR)
	for dir in $(CLIENTS_DIRS) ; do \
		( cd $$dir ; make -f $(MAKEFILE) BINDIR=../$(BINDIR) install ) ; \
	done

########################################################################

$(BINDIR):
	mkdir $(BINDIR)

force:	
