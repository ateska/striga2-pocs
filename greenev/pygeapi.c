#include "greenev.h"

static PyObject* py_geapi_io_thread_listen(PyObject *self, PyObject *args)
{
	PyObject * py_thread_capsule;
	const char *hostname;
	const char *port;
	int backlog;

	if (!PyArg_ParseTuple(args, "Ossi", &py_thread_capsule, &hostname, &port, &backlog)) return NULL;

	if (app != NULL) io_thread_listen(&app->io_thread, hostname, port, backlog);

	Py_RETURN_NONE;
}

///

static PyMethodDef _py_geapi_methods[] = 
{
	{"io_thread_listen", py_geapi_io_thread_listen, METH_VARARGS, "Command IO thread to create listening socket on given host and port."},
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
