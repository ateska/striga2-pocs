#include "greenev.h"

static PyObject* py_geapi_io_thread_listen(PyObject *self, PyObject *args)
{
	PyObject * py_thread_capsule;
	const char *hostname;
	const char *port;
	int backlog;

	if (!PyArg_ParseTuple(args, "Ossi", &py_thread_capsule, &hostname, &port, &backlog))
		return NULL;

	if (app != NULL) io_thread_listen(&app->io_thread, hostname, port, backlog);

	Py_RETURN_NONE;
}


static PyObject* py_geapi_py_thread_next_cmd(PyObject *self, PyObject *args)
{
	PyObject * py_thread_capsule;
	PyThreadState *pthread_save;
	unsigned int timeout_ms;

	if (!PyArg_ParseTuple(args, "OI", &py_thread_capsule, &timeout_ms))
		return NULL;

	if (!PyCapsule_CheckExact(py_thread_capsule))
		return NULL;

	struct py_thread * py_thread = PyCapsule_GetPointer(py_thread_capsule, "struct py_thread *");
	if (py_thread == NULL)
		return NULL;

	int res;
	struct cmd cmd;

	pthread_save = PyEval_SaveThread(); // Release GIL

	//Get command from Python thread command queue
	res = cond_cmd_q_timed_get(py_thread->cmd_q, &cmd, timeout_ms);

	if (res == 0)
	{
		// Timeout
		PyEval_RestoreThread(pthread_save); // Acquire GIL
		Py_RETURN_NONE;
	}

	else if (res == 1)
	{
		//TODO: Format comand in order to make it compatible with Python
		PyEval_RestoreThread(pthread_save); // Acquire GIL
		return PyLong_FromLong(cmd.id);
	}

	else
	{
		PyEval_RestoreThread(pthread_save); // Acquire GIL
		PyErr_SetString(PyExc_RuntimeError, "when waiting on command queue mutex");
		return NULL;
	}
;
}


///

static PyMethodDef _py_geapi_methods[] = 
{
	{"io_thread_listen", py_geapi_io_thread_listen, METH_VARARGS, "Command IO thread to create listening socket on given host and port."},

	{"py_thread_next_cmd", py_geapi_py_thread_next_cmd, METH_VARARGS, "Block and wait for next command in py_thread command Q."},

	{NULL, NULL, 0, NULL}
};

static PyModuleDef _py_geapi_module =
{
	PyModuleDef_HEAD_INIT, PYGEAPI_MODULE, NULL, -1, _py_geapi_methods,
	NULL, NULL, NULL, NULL
};

PyObject* py_geapi_emb_init()
{
	return PyModule_Create(&_py_geapi_module);
}
