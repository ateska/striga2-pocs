#ifndef PYGLEV_H_
#define PYGLEV_H_

#include <stdbool.h>
#include <netdb.h>

#include <pthread.h>

#include <ev.h>

#include <Python.h>
#include <structmember.h>

enum pyglev_evloop_cmd_ids
{
	pyglev_evloop_cmd_CALLABLE = 0,
	pyglev_evloop_cmd_LISTEN_CMD_01 = 50,
};

enum pyglev_error_types
{
	pyglev_error_type_ERRNO = 1,  // Errors reported vie errno
	pyglev_error_type_EAI = 2,    // Errors origins at getaddrinfo() function (DNS resolve)
	pyglev_error_type_GENER = 3,    // General errors (see codes bellow)
};

enum pyglev_general_error_codes
{
	pyglev_general_error_code_LISTEN_ERR = 1,
};

#include "cmdq.h"
#include "evloop.h"
#include "lstncmd.h"
#include "estsock.h"

#endif //PYGLEV_H_
