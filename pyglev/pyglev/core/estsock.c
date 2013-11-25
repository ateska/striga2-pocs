#include "pyglev.h" 

static void _on_est_socket_io(struct ev_loop *loop, struct ev_io *watcher, int revents);
static bool _est_socket_preconfig(struct est_socket * this, struct event_loop * loop, PyObject* progress_config);

///

struct est_socket * est_socket_new(struct event_loop * loop, PyObject *protocol, int socket)
{
	struct est_socket * this = malloc(sizeof(struct est_socket));
	if (this == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "est. socket memory allocation failed");
		return NULL;
	}

	this->protocol = protocol;
	Py_XINCREF(protocol);

	ev_io_init(&this->watcher, _on_est_socket_io, socket, 0);
	this->watcher.data = this;

	// Make first step in protocol
	PyObject* config = PyIter_Next(this->protocol);
	if (config == NULL)
	{
		Py_XDECREF(this->protocol);
		this->protocol = NULL;
		return NULL;
	}

	bool res = _est_socket_preconfig(this, loop, config);
	if (!res)
	{
		Py_DECREF(config);
		Py_XDECREF(this->protocol);
		this->protocol = NULL;
		return NULL;		
	}

	Py_DECREF(config);

	return this;
}


void est_socket_delete(struct est_socket * this)
{
	if (ev_is_active(&this->watcher))
		fprintf(stderr, "Watcher is active when established socket destructs!\n");

	Py_XDECREF(this->protocol);

	free(this);

	printf("DEBUG: est_socket_delete()\n");
}

///

// Configure watcher based on yield() configuration
//GIL has to be acquired when entering this
static bool _est_socket_preconfig(struct est_socket * this, struct event_loop * loop, PyObject* config)
{
	if (config == NULL)
	{
		printf("Protocol wants us to close socket!\n");
		return false;
	}
	int event_mask = PyLong_AsLong(config);

	if ((event_mask < 0) && PyErr_Occurred())
		return false;

	//TODO: Check that even_mask contains only EV_READ (or other known events)
	ev_io_stop(loop->loop, &this->watcher);
	ev_io_set(&this->watcher, this->watcher.fd, event_mask);
	ev_io_start(loop->loop, &this->watcher);

	return true;
}
///

static void _on_est_socket_read(struct est_socket * this, struct event_loop * loop, char * buffer, ssize_t size)
{
	// Acquire Python GIL
	PyEval_RestoreThread(loop->tstate);

	// Send data to protocol
	PyObject * config = PyObject_CallMethod(this->protocol,"send","(l)", size);
	bool res = _est_socket_preconfig(this, loop, config);
	if (!res)
	{
		PyErr_Print();
		Py_XDECREF(config);
		goto exit_release_gil;
		//TODO: Close socket ...
	}

	Py_DECREF(config);

	// Release Python GIL
exit_release_gil:
	loop->tstate = PyEval_SaveThread();

	return;
}

static void _on_est_socket_read_error()
{
	printf("Socket error %d!\n", errno);
}

static void _on_est_socket_closed(struct est_socket * this, struct event_loop * loop)
{
	//TODO: this is short cut
	ev_io_stop(loop->loop, &this->watcher);
	printf("Socket closed\n");
}

static void _on_est_socket_write()
{
	printf("Socket ready to write\n");
}


static void _on_est_socket_io(struct ev_loop *ev_loop, struct ev_io *watcher, int revents)
{
	struct est_socket * this = watcher->data;
	struct event_loop * loop = ev_userdata(ev_loop);

	char buffer[4096];

	if (revents & EV_READ)
	{
		ssize_t size_read = read(watcher->fd, buffer, sizeof(buffer));
		if (size_read == 0)
		{
			_on_est_socket_closed(this, loop);
			return;
		} else if (size_read < 0)
		{
			_on_est_socket_read_error();
			return;
		} else {
			_on_est_socket_read(this, loop, buffer, size_read);
		} 
	}

	if (revents & EV_WRITE)
	{
		_on_est_socket_write();
	}

	if (revents & EV_ERROR)
	{
		fprintf(stderr, "Socket is in error state\n");
	}
}

