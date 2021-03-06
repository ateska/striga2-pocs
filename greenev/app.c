#include "greenev.h"

struct application * app = NULL;

static void _on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents);
static void _on_application_cmd(void * arg, struct cmd cmd);

///

void application_ctor(struct application * this, int argc, char **argv)
{
	assert(app == NULL);
	app = this;

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

	// Create application command queue
	this->app_cmd_q = watcher_cmd_q_new(EV_DEFAULT, 256, _on_application_cmd, this);
	watcher_cmd_q_start(this->app_cmd_q);

	// Create IO thrad
	ev_ref(EV_DEFAULT); // IO thread holds one reference to main loop
	io_thread_ctor(&this->io_thread);

	// Initialize Python virtual machine
	ev_ref(EV_DEFAULT); // Python thread holds one reference to main loop
	py_thread_ctor(&this->py_thread);
}


void application_dtor(struct application * this)
{
	this->run_phase = app_run_phase_EXIT;

	// Destroy python thread
	py_thread_dtor(&this->py_thread);

	// Destroy IO thread
	io_thread_dtor(&this->io_thread);

	// Destroy my command queue
	watcher_cmd_q_delete(this->app_cmd_q);

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
	this->run_phase = app_run_phase_DYING;

	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);	

	io_thread_die(&this->io_thread);
	py_thread_die(&this->py_thread);
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

int application_run(struct application * this)
{
	this->run_phase = app_run_phase_RUN;
	LOG_INFO("Application is up and running");

	// Start event loop
	for(bool keep_running=true; keep_running;)
	{
		keep_running = ev_run(EV_DEFAULT, 0);
	}

	return this->exit_status;
}

///

static void _on_application_cmd(void * arg, struct cmd cmd)
{
	struct application * this = arg;

	switch (cmd.id)
	{
		case app_cmd_IO_THREAD_EXIT:
			if (this->run_phase != app_run_phase_DYING)
			{
				LOG_ERROR("IO thread ended prematurely - this is critical error.");
				abort();
			}
			ev_unref(EV_DEFAULT); // IO thread holds one reference to main loop
			break;


		case app_cmd_PY_THREAD_EXIT:
			if (this->run_phase != app_run_phase_DYING)
			{
				LOG_ERROR("Python thread ended prematurely - this is critical error.");
				//TODO: Uncomment this as soon as Python event loop is stable: abort();
			}
			ev_unref(EV_DEFAULT); // Python thread holds one reference to main loop
			break;


		default:
			LOG_WARNING("Unknown application command '%d'", cmd.id);
	}
}
