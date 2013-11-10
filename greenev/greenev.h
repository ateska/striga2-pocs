#ifndef GREENEV_H_
#define GREENEV_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>

#include <pthread.h>

#include <ev.h>

#include <Python.h>

#include "cmdq.h"
#include "iothread.h"
#include "pythread.h"
#include "app.h"
#include "pygeapi.h"

// Logging

int logging_init();
void logging_finish();

void LOG_DEBUG(const char *fmt, ...);
void LOG_INFO(const char *fmt, ...);
void LOG_WARNING(const char *fmt, ...);
void LOG_ERROR(const char *fmt, ...);
void LOG_ERRNO(int errnum, const char *fmt, ...);

#endif //GREENEV_H_
