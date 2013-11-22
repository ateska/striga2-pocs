#include "pyglev.h" 

///

static PyMethodDef _pyglev_core_functions[] = 
{
	{NULL, NULL, 0, NULL}
};

static PyModuleDef _pyglev_core_module =
{
	PyModuleDef_HEAD_INIT,
	"pyglev.core",          // name of module
	NULL,                   // documentation string
	-1,                     // size of per-interpreter state of the module
	_pyglev_core_functions  // module functions
};

PyMODINIT_FUNC PyInit_core(void)
{
	if (PyType_Ready(&pyglev_core_event_loop_type) < 0) return NULL;
	if (PyType_Ready(&pyglev_core_listen_cmd_type) < 0) return NULL;

	PyObject *m = PyModule_Create(&_pyglev_core_module);
	if (m == NULL) return NULL;

	if (PyModule_AddStringConstant(m, "__version__", "TODO"))
		goto finish_decref;

	if (PyModule_AddIntConstant(m, "EV_READ", EV_READ))
		goto finish_decref;

	if (PyModule_AddIntConstant(m, "EV_WRITE", EV_WRITE))
		goto finish_decref;

	Py_INCREF(&pyglev_core_event_loop_type);
    PyModule_AddObject(m, "event_loop", (PyObject *)&pyglev_core_event_loop_type);

	Py_INCREF(&pyglev_core_listen_cmd_type);
    PyModule_AddObject(m, "listen_cmd", (PyObject *)&pyglev_core_listen_cmd_type);

	return m;

finish_decref:
	Py_DECREF(m);
	return NULL;
}
