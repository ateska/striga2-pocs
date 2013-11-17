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

	struct cmd_q * cmd_q;
};


// Singleton guard and global application instance pointer
extern struct application * app;

void application_ctor(struct application *, int argc, char **argv);
void application_dtor(struct application *);

int application_start(struct application *);
void application_run(struct application *);

// Issue application command (into application command queue)
static inline bool application_command(struct application * this, int cmd_id, void * data)
{
	return cmd_q_put(this->cmd_q, EV_DEFAULT, cmd_id, data);
}

void application_listen(struct application *, const char * host, const char * port, int backlog);


/// Application run phases
enum app_run_phases {
	app_run_phase_INIT = 0,
	app_run_phase_RUN = 100,
	app_run_phase_DYING = 990,
	app_run_phase_EXIT= 1000
};

/// Application commands
enum app_cmd_ids {
	app_cmd_id_LISTEN = 1,
};

#endif //SINGLEV_APP_H_
