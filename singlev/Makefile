SUBDIRS=src
CLEANSUBDIRS=test

all: subdirs

test: subdirs
	@$(MAKE) -C test

rebuild:
	${MAKE} clean
	${MAKE} all test

ROOTDIR=.
include $(ROOTDIR)/rules.make
