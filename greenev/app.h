#ifndef GREENEV_APP_H_
#define GREENEV_APP_H_

struct application
{
	int run_phase;
	int exit_status;

	ev_signal _SIGINT;
	ev_signal _SIGTERM;

	// Command queue
	struct watcher_cmd_q * app_cmd_q;

	// IO Thread
	struct io_thread io_thread;

	// Python thread
	struct py_thread py_thread;

};

// Singleton guard and global application instance pointer
extern struct application * app;

void application_ctor(struct application *, int argc, char **argv);
void application_dtor(struct application *);

int application_run(struct application *);

// This method is callable from other threads
static inline bool application_command(struct application * this, int app_cmd_id, void * arg)
{
	return watcher_cmd_q_put(this->app_cmd_q, app_cmd_id, arg);
}


/// Application run phases
enum app_run_phases {
	app_run_phase_INIT = 0,
	app_run_phase_RUN = 100,
	app_run_phase_DYING = 990,
	app_run_phase_EXIT= 1000
};

/// Application commands
enum app_cmd_id {
	app_cmd_IO_THREAD_EXIT = 1,
	app_cmd_PY_THREAD_EXIT = 2,
};

#endif //GREENEV_APP_H_
