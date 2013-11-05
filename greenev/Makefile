CFLAGS=-O0 -Wall -pthread -ggdb
LDLIBS=-lev -lpthread

all: greenev

greenev: greenev.o app.o logging.o

clean:
	${RM} *.o greenev
