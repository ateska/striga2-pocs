#import "singlev.h"

///

bool _pyutils_run_module(wchar_t *modname, int set_argv0)
{
	PyObject *module, *runpy, *runmodule, *runargs, *result;

	runpy = PyImport_ImportModule("runpy");
	if (runpy == NULL)
	{
		LOG_ERROR("Could not import runpy module");
		return false;
	}

	runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
	if (runmodule == NULL)
	{
		LOG_ERROR("Could not access runpy._run_module_as_main");
		Py_DECREF(runpy);
		return false;
	}

	module = PyUnicode_FromWideChar(modname, wcslen(modname));
	if (module == NULL)
	{
		LOG_ERROR("Could not convert module name to unicode");
		Py_DECREF(runpy);
		Py_DECREF(runmodule);
		return false;
	}

	runargs = Py_BuildValue("(Oi)", module, set_argv0);
	if (runargs == NULL)
	{
		LOG_ERROR("Could not create arguments for runpy._run_module_as_main");
		Py_DECREF(runpy);
		Py_DECREF(runmodule);
		Py_DECREF(module);
		return false;
	}

	result = PyObject_Call(runmodule, runargs, NULL);
	if (result == NULL)
	{
		PyErr_Print();
	}

	Py_DECREF(runpy);
	Py_DECREF(runmodule);
	Py_DECREF(module);
	Py_DECREF(runargs);
	if (result == NULL)
	{
		return false;
	}
	Py_DECREF(result);

	return true;
}

///

/*
bool _pyutils_add_type(PyObject *module, const char *name, PyTypeObject *type)
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
