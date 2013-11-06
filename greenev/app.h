#ifndef GREENEV_APP_H_
#define GREENEV_APP_H_

#define APP_CMD_Q_DEPTH 200

struct app_cmd
{
	int cmd_id;
	void * arg;
};

struct application
{
	int run_phase;
	int exit_status;

	ev_signal _SIGINT;
	ev_signal _SIGTERM;

	// Items related to application command queue
	struct app_cmd app_cmq_q[APP_CMD_Q_DEPTH];
	int app_cmq_pos;
	pthread_mutex_t app_cmq_q_mtx;
	struct ev_async app_cmq_q_watcher;

	// IO Thread
	struct io_thread io_thread;

} application;

// Singleton guard and global application instance pointer
extern struct application * app;

void application_ctor(struct application *);
void application_dtor(struct application *);

int application_run(struct application *);

// This method is callable for other threads
void application_command(struct application *, int cmd_id, void * arg);


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
};

#endif //GREENEV_APP_H_
