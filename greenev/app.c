#include "greenev.h"

application * app = NULL;

static void _on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents);

///

void application_ctor(application * this)
{
	assert(app == NULL);
	app = this;

	this->exit_status = EXIT_SUCCESS;

	// Initialize default event loop
	struct ev_loop * main_loop = ev_default_loop(EVFLAG_AUTO);
	if (!main_loop)
	{
		LOG_ERROR("Could not initialise libev");
		exit(EXIT_FAILURE);
	}

	// Register SIGINT handlers
	ev_signal_init(&this->_SIGINT, _on_terminating_signal, SIGINT);
	ev_signal_start(EV_DEFAULT, &this->_SIGINT);
	this->_SIGINT.data = this;

	ev_signal_init(&this->_SIGTERM, _on_terminating_signal, SIGTERM);
	ev_signal_start(EV_DEFAULT, &this->_SIGTERM);
	this->_SIGTERM.data = this;
}

void application_dtor(application * this)
{
	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);

	app = NULL;
}

///

static void _on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents)
{
	application * this = w->data;

	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);

	if (w->signum == SIGINT)
	{
		printf("\n");
	}
}

///

int application_run(application * this)
{
	LOG_INFO("Application is up and running");

	// Start event loop
	for(bool keep_running=true; keep_running;)
	{
		keep_running = ev_run(EV_DEFAULT, 0);
	}

	return this->exit_status;
}
