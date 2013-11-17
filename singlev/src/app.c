#include "singlev.h"

struct application * app = NULL;

static wchar_t *_application_python_args[] = {L"./singlev", L""};
static void _on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents);

///

void application_ctor(struct application * this, int argc, char **argv)
{
	assert(app == NULL);
	assert(Py_IsInitialized() == 0);

	app = this;

	this->established_ev_sockets = NULL;
	this->listening_ev_sockets = NULL;
	this->python_thread_state = NULL;
	this->run_phase = app_run_phase_INIT;
	this->exit_status = EXIT_SUCCESS;

	// Initialize default event loop
	struct ev_loop * main_loop = ev_default_loop(EVFLAG_AUTO);
	if (!main_loop)
	{
		LOG_ERROR("Could not initialise libev");
		exit(EXIT_FAILURE);
	}

	// Register SIGINT signal handler
	ev_signal_init(&this->_SIGINT, _on_terminating_signal, SIGINT);
	ev_signal_start(EV_DEFAULT, &this->_SIGINT);
	this->_SIGINT.data = this;

	// Register SIGTERM signal handler
	ev_signal_init(&this->_SIGTERM, _on_terminating_signal, SIGTERM);
	ev_signal_start(EV_DEFAULT, &this->_SIGTERM);
	this->_SIGTERM.data = this;

	// Configure Python interpreter
	Py_SetProgramName(_application_python_args[0]);
	Py_InspectFlag = 1;

	// Ensure that embed API is available
	PyImport_AppendInittab(PYSEVAPI_MODULE, &pysevapi_init);

	// Initialize Python interpreter
	Py_InitializeEx(0); // 0 to skip signal initialization
	PySys_SetArgv(1, _application_python_args);

	PySys_ResetWarnOptions();

}


void application_dtor(struct application * this)
{
	assert(this->python_thread_state == NULL);

	// Finalize Python interpreter
	Py_Finalize();

	this->run_phase = app_run_phase_EXIT;

	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);

	app = NULL;
}

///

static void _application_die(struct application * this)
/*
This function initiates process of application exit.
It should inform all subcomponents about app termination intention.

Actual exit is controlled by main event loop, respective by its reference count.
Whenever this count reaches zero, applicaton will exit.
Therefore subcomponents should 'manipulate' this reference count in order to prevent
premature exit.

It is 'protected' function, should not be called from 'external' object.
*/
{
	if (this->run_phase == app_run_phase_DYING) return;
	if (this->run_phase == app_run_phase_EXIT) return;

	assert(this->run_phase == app_run_phase_RUN);
	this->run_phase = app_run_phase_DYING;

	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);	
}


static void _on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents)
{
	struct application * this = w->data;
	if (w->signum == SIGINT)
	{
		printf("\n");
	}

	_application_die(this);
}

///

int application_start(struct application * this)
{
	this->run_phase = app_run_phase_RUN;
	LOG_INFO("Application is up and running");

	_pyutils_run_module(L"demo", 1);

	// Ensure that we are dying properly ...
	_application_die(this);

	return this->exit_status;
}

void application_run(struct application * this)
// This method is called from Python interpretter
{
	// Start event loop
	for(bool keep_running=true; keep_running;)
	{
		keep_running = ev_run(EV_DEFAULT, 0);
	}
}
