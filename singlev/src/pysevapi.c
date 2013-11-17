#include "singlev.h"

static bool _pysevapi_app_ready();

///

static PyObject * _pysevapi_application_run(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, "")) return NULL;
	if (!_pysevapi_app_ready()) return NULL;

	if (app->python_thread_state != NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "Application event loop seems to be already activated");
		return NULL;
	}

	// Release the global interpreter lock (GIL)
	app->python_thread_state = PyEval_SaveThread();

	application_run(app);

	// Acquire the global interpreter lock (GIL)
	PyEval_RestoreThread(app->python_thread_state);
	app->python_thread_state = NULL;

	Py_RETURN_NONE;
}


static PyObject * _pysevapi_application_listen(PyObject *self, PyObject *args)
{
	const char *hostname;
	const char *port;
	int backlog;
	int concurent;

	bool result;

	if (!PyArg_ParseTuple(args, "ssii", &hostname, &port, &backlog, &concurent)) return NULL;
	if (!_pysevapi_app_ready()) return NULL;

	//TODO: concurent

	Py_BEGIN_ALLOW_THREADS;
	result = application_command(app, app_cmd_id_LISTEN, NULL);
	Py_END_ALLOW_THREADS;

	if (!result)
	{
		PyErr_SetString(PyExc_RuntimeError, "Failed to issue a application command");
		return NULL;
	}

	Py_RETURN_NONE;
}

///

static PyMethodDef _pysevapi_methods[] = 
{
	{"application_run", _pysevapi_application_run, METH_VARARGS, "Enter application-wide event loop."},
	{"application_listen", _pysevapi_application_listen, METH_VARARGS, "Start listening on given host/port, incomming connections are assigned with new greenlet."},
	{NULL, NULL, 0, NULL}
};

static PyModuleDef _pysevapi_module =
{
	PyModuleDef_HEAD_INIT, PYSEVAPI_MODULE, NULL, -1, _pysevapi_methods,
	NULL, NULL, NULL, NULL
};

PyObject* pysevapi_init()
{
	PyObject* m = PyModule_Create(&_pysevapi_module);
	if (!m) return NULL;

	if (PyModule_AddStringConstant(m, "__version__", "TODO"))
	{
		goto finish_decref;
	}

//	if (!_pyutils_add_type(m, "application", &applicationType))
//		goto finish_decref;

	return m;

finish_decref:
	LOG_ERROR("Initialization of " PYSEVAPI_MODULE " failed.");
	Py_DECREF(m);
	return NULL;
}

/////

static bool _pysevapi_app_ready()
{
	if (app == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "Application is not ready");
		return false;
	}

	if (app->run_phase != app_run_phase_RUN)
	{
		PyErr_SetString(PyExc_RuntimeError, "Application is not running");
		return false;
	}

	return true;
}
