#include "greenev.h"

static void * _py_thread_start(void *);
static void _py_thread_cleanup(void * arg);
static int _py_run_module(wchar_t *modname, int set_argv0);

///

void py_thread_ctor(struct py_thread * this)
{
	assert(Py_IsInitialized() == 0);

	int rc;

	// Prepare creation of python thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// Temporarily block all signals - to ensure that newly created thread inherites this ...
	sigset_t new_set, orig_set;
	sigfillset(&new_set);
	rc = pthread_sigmask(SIG_SETMASK, &new_set, &orig_set);
	if (rc != 0)
	{
		LOG_ERROR("Failed to set sigmask for python thread: %d", rc);
		abort();
	}

	// Create thread
	rc = pthread_create(&this->thread, &attr, _py_thread_start, this);
	if (rc != 0)
	{
		LOG_ERROR("Failed to create python thread: %d", rc);
		abort();
	}

	// Restore original signal mask
	rc = pthread_sigmask(SIG_SETMASK, &orig_set, NULL);
	if (rc != 0)
	{
		LOG_ERROR("Failed to set sigmask for python thread (2): %d", rc);
		abort();
	}

	pthread_attr_destroy(&attr);

}

void py_thread_dtor(struct py_thread * this)
{
	void * result;

	//TODO: Signalization to the thread that we are about to exit (maybe ...die() method should be implemented)
	pthread_join(this->thread, &result);

	Py_Finalize(); // Finalize Python interpretter
	// This function is here since premature call of this function spoils signal handlers set by libev
}

///

wchar_t *_py_thread_args[] = {L"-m", L"./greenev", L""};

static void * _py_thread_start(void * arg)
{
	//struct py_thread * this = arg;

	pthread_cleanup_push(_py_thread_cleanup, arg);

	Py_SetProgramName(_py_thread_args[1]);
	Py_InspectFlag = 1;

	PyImport_AppendInittab(PYGEAPI_MODULE, &py_geapi_emb_init);

	Py_InitializeEx(0); /* Skip signal initialization */

	PySys_SetArgv(2, _py_thread_args);

	PySys_ResetWarnOptions();
	
	int ret = _py_run_module(L"_greenev", 1);
	LOG_DEBUG("Python exited: %d", ret);
	//TODO: There should be some loop in the Python

	pthread_cleanup_pop(1);

	// Exit thread
	pthread_exit(NULL);
	return NULL;

}

static void _py_thread_cleanup(void * arg)
{
	//struct py_thread * this = arg;
}

static int _py_run_module(wchar_t *modname, int set_argv0)
{
	PyObject *module, *runpy, *runmodule, *runargs, *result;

	runpy = PyImport_ImportModule("runpy");
	if (runpy == NULL)
	{
		fprintf(stderr, "Could not import runpy module\n");
		return -1;
	}

	runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
	if (runmodule == NULL)
	{
		fprintf(stderr, "Could not access runpy._run_module_as_main\n");
		Py_DECREF(runpy);
		return -1;
	}

	module = PyUnicode_FromWideChar(modname, wcslen(modname));
	if (module == NULL)
	{
		fprintf(stderr, "Could not convert module name to unicode\n");
		Py_DECREF(runpy);
		Py_DECREF(runmodule);
		return -1;
	}

	runargs = Py_BuildValue("(Oi)", module, set_argv0);
	if (runargs == NULL)
	{
		fprintf(stderr,
			"Could not create arguments for runpy._run_module_as_main\n");
		Py_DECREF(runpy);
		Py_DECREF(runmodule);
		Py_DECREF(module);
		return -1;
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
		return -1;
	}
	Py_DECREF(result);
	return 0;
}
