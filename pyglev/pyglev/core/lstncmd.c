#include "pyglev.h" 

static void _on_listen_socket_io(struct ev_loop *loop, struct ev_io *watcher, int revents);
///

static PyObject * listen_cmd_ctor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	char *host;
	char *port;
	int backlog = 20;
	PyObject *on_accept = NULL;

	static char *kwlist[] = {"host", "port", "on_accept", "backlog", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|Oi:__new__", kwlist,
		&host, &port,
		&on_accept,
		&backlog
	)) return NULL;

	// Create self
	struct listen_cmd *self = (struct listen_cmd *)type->tp_alloc(type, 0);
	if (self == NULL) return NULL;


	// Copy values into members
	if (strcmp(host, "*") == 0) self->host = NULL;
	else self->host = strdup(host);
	self->port = strdup(port);
	self->backlog = backlog;
	self->status = listen_cmd_status_INIT;
	self->watchers = NULL;

	self->on_accept = on_accept;
	Py_XINCREF(on_accept);

	return (PyObject *)self;
}

static int listen_cmd_tp_clear(struct listen_cmd *self)
{
	Py_CLEAR(self->on_accept);
    return 0;
}

static int listen_cmd_tp_traverse(struct listen_cmd *self, visitproc visit, void *arg)
{
	Py_VISIT(self->on_accept);
    return 0;
}


static void listen_cmd_dtor(struct listen_cmd * self)
{
	if (self->watchers)
	{
		for (int i=0; i<self->watchers_count; i++)
		{
			if (ev_is_active(&self->watchers[i])) 
				fprintf(stderr, "Watchers are active when listen command destructs!\n");
		}
		free(self->watchers);
	}

	if (self->host) free(self->host);
	free(self->port);

	listen_cmd_tp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);

	printf("DEBUG: listen_cmd_dtor()\n");
}

///

void _listen_cmd_xschedule_01(struct listen_cmd * self, struct event_loop * event_loop)
{
	// In this step, we resolve given hostname and port into IPs
	
	int rc, i;
	bool failure = false;

	self->status = listen_cmd_status_RESOLVING;

	//TODO: Asynchronous resolve
	// Resolve address/port into IPv4/IPv6 address infos
	struct addrinfo req, *ans;
	req.ai_family = AF_UNSPEC; // Both IPv4 and IPv6 if available
	req.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_V4MAPPED;
	req.ai_socktype = SOCK_STREAM;
	req.ai_protocol = IPPROTO_TCP;

	rc = getaddrinfo(self->host, self->port, &req, &ans);
	if (rc != 0)
	{
		_event_loop_error(event_loop, 
			(PyObject *) self, pyglev_error_type_EAI, rc, 
			"resolving listen address %s (port %s): %s", 
			self->host, self->port, gai_strerror(rc)
		);
		return;
	}


	self->watchers_count = 0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next)
		self->watchers_count += 1;


	int listen_socket[self->watchers_count];
	for (i=0; i<self->watchers_count; i++) listen_socket[i] = -1;


	i=0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next, i++)
	{
		char hoststr[NI_MAXHOST];
		char portstr[NI_MAXSERV];
		rc = getnameinfo(rp->ai_addr, sizeof(struct sockaddr), hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
		if (rc != 0)
		{
			_event_loop_error(event_loop, 
				(PyObject *) self, pyglev_error_type_EAI, rc, 
				"resolving listen address: %s", gai_strerror(rc)
			);
			strcpy(hoststr, "?");
			strcpy(portstr, "?");
		}

		// Create socket
		listen_socket[i] = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (listen_socket[i] < 0)
		{
			_event_loop_error(event_loop, 
				(PyObject *) self, pyglev_error_type_ERRNO, errno, 
				"creating listen socket"
			);
			failure = true;
			goto end_freeaddrinfo;
		}

		// Set reuse address option
		int on = 1;
		rc = setsockopt(listen_socket[i], SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
		if (rc == -1)
		{
			_event_loop_error(event_loop, 
				(PyObject *) self, pyglev_error_type_ERRNO, errno, 
				"setting SO_REUSEADDR to listen socket"
			);
			failure = true;
			goto end_freeaddrinfo;
		}

		// Bind socket
		rc = bind(listen_socket[i], rp->ai_addr, rp->ai_addrlen);
		if (rc != 0)
		{
			_event_loop_error(event_loop, 
				(PyObject *) self, pyglev_error_type_ERRNO, errno, 
				"binding listen socket to %s %s", hoststr, portstr
			);
			failure = true;
			goto end_freeaddrinfo;
		}

		// Start listening
		rc = listen(listen_socket[i], self->backlog);
		if (rc != 0)
		{
			_event_loop_error(event_loop, 
				(PyObject *) self, pyglev_error_type_ERRNO, errno, 
				"listening on socket %s %s", hoststr, portstr
			);
			failure = true;
			goto end_freeaddrinfo;
		}
	}

	self->watchers = calloc(self->watchers_count, sizeof(ev_io));
	i=0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next, i++)
	{
		ev_io_init(&self->watchers[i], _on_listen_socket_io, listen_socket[i], EV_READ);
		self->watchers[i].data = self;
		// new_sockobj->domain = rp->ai_family;
		// new_sockobj->type = rp->ai_socktype;
		// new_sockobj->protocol = rp->ai_protocol;

		// memcpy(&new_sockobj->address, rp->ai_addr, rp->ai_addrlen);
		// new_sockobj->address_len = rp->ai_addrlen;

		ev_io_start(event_loop->loop, &self->watchers[i]);
	}

	self->status = listen_cmd_status_LISTENING;

end_freeaddrinfo:
	freeaddrinfo(ans);

	if (failure)
	{
		for (i=0; i<self->watchers_count; i++)
		{
			if (listen_socket[i] >= 0) close(listen_socket[i]);
		}
	}
}

void _listen_cmd_close(struct listen_cmd * self, struct event_loop * event_loop)
{
	int res;

	if ((self->status == listen_cmd_status_INIT) || (self->status == listen_cmd_status_RESOLVING))
	{
		self->status = listen_cmd_status_CLOSED;
		return; // Do nothing when not initialized
	}

	if (self->status != listen_cmd_status_LISTENING)
		fprintf(stderr, "Unexpected status '%c' in listen command\n", self->status);

	if (self->watchers)
	{
		for (int i=0; i<self->watchers_count; i++)
		{
			ev_io_stop(event_loop->loop, &self->watchers[i]);
			res = close(self->watchers[i].fd);
			if (res != 0) fprintf(stderr, "Error when closing listen socket!\n");
		}
	}	

	self->status = listen_cmd_status_CLOSED;
}

///

static void _on_listen_socket_accept(struct listen_cmd * self, struct event_loop * loop, int listen_fd)
{
	struct sockaddr_storage client_addr;
	socklen_t client_len = sizeof(struct sockaddr_storage);

	// Accept client request
	int client_socket = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_socket < 0)
	{
		_event_loop_error(loop,
			(PyObject *)self, 
			pyglev_error_type_ERRNO, errno,
			"accepting on listen socket"
		);
		return;
	}

	if (self->status != listen_cmd_status_LISTENING)
	{
		_event_loop_error(loop,
			(PyObject *)self, 
			pyglev_error_type_GENER, pyglev_general_error_code_LISTEN_ERR,
			"not LISTENING"
		);
		goto exit_close;
	}

	// Acquire Python GIL
	PyEval_RestoreThread(loop->tstate);

	//TODO: Pass following information to the green thread
	// client_addr, client_len
	// listening_sockobj->domain
	// listening_sockobj->type
	// listening_sockobj->protocol

	PyObject * protocol = PyObject_CallFunctionObjArgs(self->on_accept, self, NULL);
	if (protocol == NULL)
	{
		PyErr_Print();
		goto exit_release_gil;
	}

	// If 'False' or None is retrieved, silently close a socket
	if ((PyObject_Not(protocol) == 1)||(protocol == Py_None))
	{
		goto exit_remove_protocol;	
	}

	if (PyGen_Check(protocol) != true)
	{
		PyObject* x = PyUnicode_FromFormat("%R", protocol);
		PySys_WriteStderr("%s is not generator\n", PyUnicode_AsUTF8(x));
		Py_DECREF(x);

		goto exit_remove_protocol;	
	}

	struct est_socket * est_socket = est_socket_new(loop, protocol, client_socket);
	if (est_socket == NULL)
	{
		PyErr_Print();
		goto exit_remove_protocol;
	}

	Py_DECREF(protocol);

	// Add newly established socket into event loop list
	est_socket->next = loop->established_sockets;
	loop->established_sockets = est_socket;

	// Release Python GIL
	loop->tstate = PyEval_SaveThread();

	return;

exit_remove_protocol:
	Py_DECREF(protocol);

exit_release_gil:
	// Release Python GIL
	loop->tstate = PyEval_SaveThread();

exit_close:
	close(client_socket);

	return;
}

static void _on_listen_socket_io(struct ev_loop *ev_loop, struct ev_io *watcher, int revents)
{
	struct listen_cmd * self = watcher->data;
	struct event_loop * loop = ev_userdata(ev_loop);

	if (revents & EV_ERROR)
	{
		_event_loop_error(loop,
			(PyObject *)self, 
			pyglev_error_type_GENER, pyglev_general_error_code_LISTEN_ERR,
			"listening"
		);
		return;
	}

	if (revents & EV_READ) 
		_on_listen_socket_accept(self, loop, watcher->fd);

}

/// --- Python type definition follows

static PyMethodDef listen_cmd_methods[] = {
    {NULL}  /* Sentinel */
};

static PyMemberDef listen_cmd_members[] = {
	{"host", T_STRING, offsetof(struct listen_cmd, host), 1, "Host name used for listening"},
	{"port", T_STRING, offsetof(struct listen_cmd, port), 1, "Port name used for listening"},
	{"backlog", T_INT, offsetof(struct listen_cmd, backlog), 1, "Size of listen backlog"},
	{"status", T_CHAR, offsetof(struct listen_cmd, status), 1, "Status of listen command"},	
	{"on_accept", T_OBJECT_EX, offsetof(struct listen_cmd, on_accept), 0, "This event is triggered when new connection is accepted."},
	{NULL}  /* Sentinel */
};

PyTypeObject pyglev_core_listen_cmd_type = 
{
	PyObject_HEAD_INIT(NULL)
	"pyglev.core.listen_cmd",  /* tp_name */
	sizeof(struct listen_cmd), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)listen_cmd_dtor,   /* tp_dealloc */
	0,                         /* tp_print */
	0,                         /* tp_getattr */
	0,                         /* tp_setattr */
	0,                         /* tp_reserved */
	0,                         /* tp_repr */
	0,                         /* tp_as_number */
	0,                         /* tp_as_sequence */
	0,                         /* tp_as_mapping */
	0,                         /* tp_hash  */
	0,                         /* tp_call */
	0,                         /* tp_str */
	0,                         /* tp_getattro */
	0,                         /* tp_setattro */
	0,                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
	0,                         /* tp_doc */
    (traverseproc)listen_cmd_tp_traverse,    /* tp_traverse */
    (inquiry)listen_cmd_tp_clear,            /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    listen_cmd_methods,        /* tp_methods */
    listen_cmd_members,        /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    listen_cmd_ctor,           /* tp_new */
};
