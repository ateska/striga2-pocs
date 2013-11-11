CFLAGS:=-O0 -Wall -pthread -ggdb $(shell python3-config --cflags)
CFLAGS:=$(filter-out -O3 -g -DNDEBUG,$(CFLAGS)) # Filter out Python flags

LDLIBS=-lev -lpthread $(shell python3-config --libs --ldflags) -lcheck

all: unit_tests
	./unit_tests

clean:
	${RM} check_cmdq *.o

unit_tests: unit_tests.o check_cmdq.o ../cmdq.o
