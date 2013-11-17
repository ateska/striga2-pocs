#include "singlev.h"

struct application * app = NULL;

static wchar_t *_application_python_args[] = {L"./singlev", L""};

static void _application_on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents);
static void _application_on_cmdq_ready(struct ev_loop *loop, ev_async *w, int revents);

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

	// Create command queueu
	this->cmd_q = cmd_q_new(128, _application_on_cmdq_ready);
	this->cmd_q->data = this;
	cmd_q_start(this->cmd_q, EV_DEFAULT);

	// Register SIGINT signal handler
	ev_signal_init(&this->_SIGINT, _application_on_terminating_signal, SIGINT);
	ev_signal_start(EV_DEFAULT, &this->_SIGINT);
	this->_SIGINT.data = this;

	// Register SIGTERM signal handler
	ev_signal_init(&this->_SIGTERM, _application_on_terminating_signal, SIGTERM);
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

	cmd_q_stop(this->cmd_q, EV_DEFAULT);

	// Finalize Python interpreter
	Py_Finalize();

	this->run_phase = app_run_phase_EXIT;

	ev_signal_stop(EV_DEFAULT, &this->_SIGINT);
	ev_signal_stop(EV_DEFAULT, &this->_SIGTERM);

	cmd_q_delete(this->cmd_q);

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


static void _application_on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents)
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
	bool keep_running=true;

	// Start event loop
	while(keep_running)
	{
		keep_running = ev_run(EV_DEFAULT, 0);
	}
}

///

static void _application_on_cmdq_ready(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd_q * cmd_q = w->data;
	struct cmd cmd;

	while (true)
	{
		int qsize = cmd_q_get(cmd_q, &cmd);
		if (qsize < 0) break;
		//struct application * this = cmd_q->data;

		printf("Got command %d - %d\n", cmd.id, qsize);

		if (qsize == 0) break;
	}

	printf("Done\n");
}

///

void application_listen(struct application * this, const char * host, const char * port, int backlog)
{
	int rc, i;
	bool failure = false;

	// Resolve address/port into IPv4/IPv6 address infos
	//TODO: Async DNS resolve (how to?)
	struct addrinfo req, *ans;
	req.ai_family = AF_UNSPEC; // Both IPv4 and IPv6 if available
	req.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_V4MAPPED;
	req.ai_socktype = SOCK_STREAM;
	req.ai_protocol = IPPROTO_TCP;

	if (strcmp(host, "*") == 0) host = NULL;

	rc = getaddrinfo(host, port, &req, &ans);
	if (rc != 0)
	{
		LOG_ERROR("Failed to resolve listen address: (%d) %s", rc, gai_strerror(rc));
		return;
	}

	size_t count = 0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next)
	{
		count += 1;
	}

	int listen_socket[count];
	for (i=0; i<count; i++) listen_socket[i] = -1;

	i=0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next, i++)
	{
		char hoststr[NI_MAXHOST];
		char portstr[NI_MAXSERV];
		rc = getnameinfo(rp->ai_addr, sizeof(struct sockaddr), hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
		if (rc != 0)
		{
			LOG_WARN("Problem occured when resolving listen socket: %s",gai_strerror(rc));
			strcpy(hoststr, "?");
			strcpy(portstr, "?");
		}

		// Create socket
		listen_socket[i] = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (listen_socket[i] < 0)
		{
			LOG_ERRNO(errno, "Failed creating listen socket");
			failure = true;
			goto end_freeaddrinfo;
		}

		// Set reuse address option
		int on = 1;
		rc = setsockopt(listen_socket[i], SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
		if (rc == -1)
		{
			LOG_ERRNO(errno, "Failed when setting option SO_REUSEADDR to listen socket");
			failure = true;
			goto end_freeaddrinfo;
		}

		// Bind socket
		rc = bind(listen_socket[i], rp->ai_addr, rp->ai_addrlen);
		if (rc != 0)
		{
			LOG_ERRNO(errno, "Failed when binding listen socket to %s %s", hoststr, portstr);
			failure = true;
			goto end_freeaddrinfo;
		}

		// Start listening
		rc = listen(listen_socket[i], backlog);
		if (rc != 0)
		{
			LOG_ERRNO(errno, "Listening on %s %s", hoststr, portstr);
			failure = true;
			goto end_freeaddrinfo;
		}
	}

/*	i=0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next, i++)
	{
		struct io_thread_listening_socket * new_sockobj = malloc(sizeof(struct io_thread_listening_socket));
		new_sockobj->next = NULL;
		ev_io_init(&new_sockobj->watcher, _on_io_thread_accept, listen_socket[i], EV_READ);
		new_sockobj->io_thread = this;
		new_sockobj->watcher.data = new_sockobj;
		new_sockobj->domain = rp->ai_family;
		new_sockobj->type = rp->ai_socktype;
		new_sockobj->protocol = rp->ai_protocol;
		memcpy(&new_sockobj->address, rp->ai_addr, rp->ai_addrlen);
		new_sockobj->address_len = rp->ai_addrlen;

		watcher_cmd_q_put(this->cmd_q, iot_cmd_IO_THREAD_LISTEN, new_sockobj);
	}*/

end_freeaddrinfo:
	freeaddrinfo(ans);

	if (failure)
	{
		LOG_WARN("Issue when preparing listen sockets, no listen socket created");
		for (i=0; i<count; i++)
		{
			if (listen_socket[i] >= 0) close(listen_socket[i]);
		}
	}

}
