#include "pyglev.h" 

///

static PyMethodDef _pyglev_core_methods[] = 
{

	{NULL, NULL, 0, NULL}
};

static PyModuleDef _pyglev_core_module =
{
	PyModuleDef_HEAD_INIT, "pyglev.core", NULL, -1, _pyglev_core_methods,
	NULL, NULL, NULL, NULL
};

PyObject* PyInit_core()
{
	return PyModule_Create(&_pyglev_core_module);
}
