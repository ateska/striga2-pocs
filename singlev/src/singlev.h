#ifndef SINGLEV_H_
#define SINGLEV_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>

#include <ev.h>

#include <Python.h>

// Logging

int logging_init();
void logging_finish();

void LOG_DEBUG(const char *fmt, ...);
void LOG_INFO(const char *fmt, ...);
void LOG_WARNING(const char *fmt, ...);
void LOG_ERROR(const char *fmt, ...);
void LOG_ERRNO(int errnum, const char *fmt, ...);

#endif ///SINGLEV_H_
