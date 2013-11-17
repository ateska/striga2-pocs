BIN=unit_tests
OBJS=unit_tests.o check_cmdq.o
UTOBJS=../src/cmdq.o ../src/logging.o

LDLIBS+=-lcheck

all: $(BIN)
	./unit_tests

ROOTDIR=..
include $(ROOTDIR)/rules.make
