#include "greenev.h"

static void * _io_thread_start(void *);
static void _io_thread_cleanup(void * arg);

///

void io_thread_ctor(struct io_thread * this)
{
	int rc;

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
}

///

void io_thread_die(struct io_thread * this)
{

}

///

static void * _io_thread_start(void * arg)
{
	pthread_cleanup_push(_io_thread_cleanup, arg);  

//	struct io_thread * this = arg;

/*	ev_async_start(this->loop, &this->async_watcher);

	// Start event loop
	for(bool keep_running=true; keep_running;)
	{
		keep_running = ev_run(this->loop, 0);
	}
*/

	pthread_cleanup_pop(1);

	// Exit thread
	pthread_exit(NULL);
	return NULL;
}

static void _io_thread_cleanup(void * arg)
{
	if (app == NULL) return;
	application_command(app, app_cmd_IO_THREAD_EXIT, arg);
}
