#include "greenev.h"

struct application * app = NULL;

static void _on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents);
static void _on_application_appcmdq(struct ev_loop *loop, ev_async *w, int revents);

///

void application_ctor(struct application * this)
{
	assert(app == NULL);
	app = this;

	this->run_phase = app_run_phase_INIT;
	this->exit_status = EXIT_SUCCESS;
	this->app_cmq_pos = 0;
	pthread_mutex_init(&this->app_cmq_q_mtx, NULL);

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

	// Prepare asynch watcher for application command queue
	ev_async_init(&this->app_cmq_q_watcher, _on_application_appcmdq);
	this->app_cmq_q_watcher.data = this;
	ev_async_start(EV_DEFAULT, &this->app_cmq_q_watcher);

	ev_ref(EV_DEFAULT); // IO thread holds one reference to main loop
	io_thread_ctor(&this->io_thread);
}

void application_dtor(struct application * this)
{
	this->run_phase = app_run_phase_EXIT;

	io_thread_dtor(&this->io_thread);

	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);
	ev_async_stop(EV_DEFAULT, &this->app_cmq_q_watcher);

	pthread_mutex_destroy(&this->app_cmq_q_mtx);

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

	ev_unref(EV_DEFAULT); // ... for this->app_cmq_q_watcher

	io_thread_die(&this->io_thread);
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

/// Command queue implementation follows 

void application_command(struct application * this, int cmd_id, void * arg)
{
	struct app_cmd new_cmd = {.cmd_id = cmd_id, .arg = arg};

	// Critical (synchronised) section
	pthread_mutex_lock(&this->app_cmq_q_mtx);
	this->app_cmq_q[this->app_cmq_pos++] = new_cmd;
	pthread_mutex_unlock(&this->app_cmq_q_mtx);

	ev_async_send(EV_DEFAULT, &this->app_cmq_q_watcher);
}

static void _on_application_appcmdq(struct ev_loop *loop, ev_async *w, int revents)
{
	struct application * this = w->data;
	struct app_cmd act_cmd;

	for(;;)
	{
		// Critical (synchronised) section
		pthread_mutex_lock(&this->app_cmq_q_mtx);
		if (this->app_cmq_pos == 0)
		{
			pthread_mutex_unlock(&this->app_cmq_q_mtx);	
			break;
		}
		act_cmd = this->app_cmq_q[--this->app_cmq_pos];
		pthread_mutex_unlock(&this->app_cmq_q_mtx);

		switch (act_cmd.cmd_id)
		{
			case app_cmd_IO_THREAD_EXIT:
				if (this->run_phase != app_run_phase_DYING)
				{
					LOG_ERROR("IO thread ended prematurely - this is critical error.");
					abort();
				}
				ev_unref(EV_DEFAULT); // IO thread holds one reference to main loop
				break;

			default:
				LOG_WARNING("Unknown application command '%d'", act_cmd.cmd_id);
		}
	}
}
