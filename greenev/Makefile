CFLAGS:=-O0 -Wall -pthread -ggdb $(shell python3-config --cflags)
CFLAGS:=$(filter-out -O3 -g -DNDEBUG,$(CFLAGS)) # Filter out Python flags

LDLIBS=-lev -lpthread $(shell python3-config --libs --ldflags)

all: greenev

greenev: greenev.o app.o logging.o iothread.o iobuffer.o cmdq.o pythread.o pygeapi.o

clean:
	${RM} *.o greenev
	${MAKE} -C tests clean

rebuild:
	${MAKE} clean
	${MAKE} all
	${MAKE} -C tests

.PHONY: clean all rebuild
