#include "greenev.h"

static void * _io_thread_start(void *);
static void _io_thread_cleanup(void * arg);
static void _on_iot_cmd(void * arg, struct cmd cmd);

///

void io_thread_ctor(struct io_thread * this)
{
	int rc;

	// Create thread-specific event loop
	this->loop = ev_loop_new(EVFLAG_NOSIGMASK);
	if (this->loop == NULL)
	{
		LOG_ERROR("Failed to create inbound event loop");
		abort();
	}

	cmd_q_ctor(&this->cmd_q, this->loop, _on_iot_cmd, this);

	// Prepare creation of inbound thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// Temporarily block all signals - to ensure that newly created thread inherites this ...
	sigset_t new_set, orig_set;
	sigfillset(&new_set);
	rc = pthread_sigmask(SIG_SETMASK, &new_set, &orig_set);
	if (rc != 0)
	{
		LOG_ERROR("Failed to set sigmask for inbound thread: %d", rc);
		abort();
	}

	// Create thread
	rc = pthread_create(&this->thread, &attr, _io_thread_start, this);
	if (rc != 0)
	{
		LOG_ERROR("Failed to create inbound thread: %d", rc);
		abort();
	}

	// Restore original signal mask
	rc = pthread_sigmask(SIG_SETMASK, &orig_set, NULL);
	if (rc != 0)
	{
		LOG_ERROR("Failed to set sigmask for inbound thread (2): %d", rc);
		abort();
	}

	pthread_attr_destroy(&attr);

}

void io_thread_dtor(struct io_thread * this)
{
	cmd_q_dtor(&this->cmd_q);
}

///

void io_thread_die(struct io_thread * this)
{
	io_thread_command(this, iot_cmd_IO_THREAD_DIE, NULL);
}

///

static void * _io_thread_start(void * arg)
{
	struct io_thread * this = arg;

	pthread_cleanup_push(_io_thread_cleanup, arg);  	
	cmd_q_start(&this->cmd_q);

	// Add artifical reference to prevent loop exit
	ev_ref(this->loop);

	// Start event loop
	for(bool keep_running=true; keep_running;)
	{
		keep_running = ev_run(this->loop, 0);
	}

	pthread_cleanup_pop(1);

	// Exit thread
	pthread_exit(NULL);
	return NULL;
}

static void _io_thread_cleanup(void * arg)
{
	struct io_thread * this = arg;
	cmd_q_stop(&this->cmd_q);

	if (app != NULL) application_command(app, app_cmd_IO_THREAD_EXIT, arg);
}

///

static void _on_iot_cmd(void * arg, struct cmd cmd)
{
	struct io_thread * this = arg;

	switch (cmd.id)
	{
		case app_cmd_IO_THREAD_EXIT:
			ev_unref(this->loop); // This should release artifical reference count for our loop and eventually exit iothread loop
			break;

		default:
			LOG_WARNING("Unknown IO thread command '%d'", cmd.id);
	}

}
