#include "greenev.h"

static PyObject* py_geapi_demo_call(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(77);
}

///

static PyMethodDef _py_geapi_methods[] = 
{
	{"demo_call", py_geapi_demo_call, METH_VARARGS, "Return the number of arguments received by the process."},
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
