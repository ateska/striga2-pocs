PYTHON_INC=$(shell python3 -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_inc())")
PYTHON_LIBDIR=$(shell python3 -c "import distutils.sysconfig; print(distutils.sysconfig.get_config_vars()['LIBDIR'])")
PYTHON_VERSION=$(shell python3 -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_version())")

CFLAGS=-O0 -Wall -pthread -ggdb -I${PYTHON_INC}
LDLIBS=-lev -lpthread -L${PYTHON_LIBDIR} -lpython${PYTHON_VERSION}

all: greenev

greenev: greenev.o app.o logging.o iothread.o cmdq.o pythread.o

clean:
	${RM} *.o greenev
