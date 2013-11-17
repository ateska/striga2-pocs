#include "singlev.h"

///

static PyObject * _pysevapi_application_run(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) return NULL;

    if (app == NULL) Py_RETURN_NONE;
    if (app->python_thread_state != NULL) Py_RETURN_NONE;

    // Release the global interpreter lock (GIL)
    app->python_thread_state = PyEval_SaveThread();

    application_run(app);

    // Acquire the global interpreter lock (GIL)
    PyEval_RestoreThread(app->python_thread_state);
    app->python_thread_state = NULL;

    Py_RETURN_NONE;
}

///

/*
static bool _pysevapi_add_type(PyObject *module, const char *name, PyTypeObject *type)
{
	if (PyType_Ready(type))
	{
		return false;
	}

	Py_INCREF(type);
	if (PyModule_AddObject(module, name, (PyObject *)type))
	{
		Py_DECREF(type);
		return false;
	}

	return true;
}
*/

///

static PyMethodDef _pysevapi_methods[] = 
{
	{"application_run", _pysevapi_application_run, METH_VARARGS, "Enter application-wide event loop."},
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

//	if (!_pysevapi_add_type(m, "application", &applicationType))
//		goto finish_decref;

	return m;

finish_decref:
	LOG_ERROR("Initialization of " PYSEVAPI_MODULE " failed.");
	Py_DECREF(m);
	return NULL;
}
