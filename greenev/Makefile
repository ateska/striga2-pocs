CFLAGS:=-O0 -Wall -pthread -ggdb $(shell python3.3-config --cflags)
CFLAGS:=$(filter-out -O3 -g -DNDEBUG,$(CFLAGS)) # Filter out Python flags

LDLIBS=-lev -lpthread $(shell python3.3-config --libs --ldflags)

all: greenev

greenev: greenev.o app.o logging.o iothread.o cmdq.o pythread.o pygeapi.o

clean:
	${RM} *.o greenev
