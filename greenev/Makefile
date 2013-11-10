PYTHON_INC=$(shell python3.3-config --includes)
PYTHON_LDFLAGS=$(shell python3.3-config --libs --ldflags)

CFLAGS=-O0 -Wall -pthread -ggdb ${PYTHON_INC}
LDLIBS=-lev -lpthread ${PYTHON_LDFLAGS}

all: greenev

greenev: greenev.o app.o logging.o iothread.o cmdq.o pythread.o pygeapi.o

clean:
	${RM} *.o greenev
