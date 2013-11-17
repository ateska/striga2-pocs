#ifndef SINGLEV_APP_H_
#define SINGLEV_APP_H_

struct application
{
	int run_phase;
	int exit_status;

	ev_signal _SIGINT;
	ev_signal _SIGTERM;

	struct established_ev_socket * established_ev_sockets;
	struct listening_ev_socket * listening_ev_sockets;

	PyThreadState *python_thread_state;
};

// Singleton guard and global application instance pointer
extern struct application * app;

void application_ctor(struct application *, int argc, char **argv);
void application_dtor(struct application *);

int application_start(struct application *);
void application_run(struct application *);


/// Application run phases
enum app_run_phases {
	app_run_phase_INIT = 0,
	app_run_phase_RUN = 100,
	app_run_phase_DYING = 990,
	app_run_phase_EXIT= 1000
};

#endif //SINGLEV_APP_H_
