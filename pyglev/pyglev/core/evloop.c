#include "pyglev.h" 

static void _event_loop_on_break_signal(struct ev_loop *loop, ev_signal *w, int revents);
static void _event_loop_on_cmdq_ready(struct ev_loop *loop, ev_async *w, int revents);
static void _event_loop_release(struct ev_loop *loop);
static void _event_loop_acquire(struct ev_loop *loop);

///

static PyObject * event_loop_ctor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	// Parse Python arguments
	int die_on_signal = 1;
	unsigned int cmd_q_size = 128;
	unsigned int flags = EVFLAG_AUTO;
	static char *kwlist[] = {"flags", "die_on_signal", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Ip:new", kwlist,
		&flags,
		&die_on_signal
	)) return NULL;


	// Create self
	struct event_loop *self = (struct event_loop *)type->tp_alloc(type, 0);
	if (self == NULL) return NULL;


	// Create event loop
	self->loop = ev_loop_new(flags);
	if (self->loop == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "failed to create event loop");
		Py_DECREF(self);
		return NULL;
    }
	ev_set_userdata(self->loop, self);

	// This automagically manipulates Python GIL to release it when loop waiting/sleeping phase is entered and acquired when exited
	ev_set_loop_release_cb (self->loop, _event_loop_release, _event_loop_acquire);


	// Create command queue
	self->cmd_q = cmd_q_new(cmd_q_size, _event_loop_on_cmdq_ready);
	self->cmd_q->data = self;
	cmd_q_start(self->cmd_q, self->loop);


	// Optionally install break signals
	if (die_on_signal)
	{
		// Create and register SIGINT signal handler
		ev_signal_init(&self->SIGINT_watcher, _event_loop_on_break_signal, SIGINT);
		ev_signal_start(self->loop, &self->SIGINT_watcher);

		// Create and register SIGTERM signal handler
		ev_signal_init(&self->SIGTERM_watcher, _event_loop_on_break_signal, SIGTERM);
		ev_signal_start(self->loop, &self->SIGTERM_watcher);
	} else {
		// When signals should not be installed, initite only common part of watcher (to survive future calls)
		ev_init(&self->SIGINT_watcher, _event_loop_on_break_signal);
		ev_init(&self->SIGTERM_watcher, _event_loop_on_break_signal);
	}

	return (PyObject *)self;
}

static void event_loop_dtor(struct event_loop * self)
{
	if (self->loop)
	{
		if ev_is_active(&self->SIGINT_watcher) ev_signal_stop(self->loop, &self->SIGINT_watcher);
		if ev_is_active(&self->SIGTERM_watcher) ev_signal_stop(self->loop, &self->SIGTERM_watcher);

		cmd_q_stop(self->cmd_q, self->loop);

		ev_loop_destroy(self->loop);
		self->loop = NULL;
    }

    if (self->cmd_q)
    {
		cmd_q_delete(self->cmd_q);
	}

	Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject * event_loop_run(struct event_loop *self)
{
	bool keep_running=true;

	// Start event loop
	while(keep_running)
	{
		keep_running = ev_run(self->loop, 0);
	}

	Py_RETURN_NONE;
}


static void _event_loop_break(struct event_loop * self)
/*
This function initiates process of event loop exit.
It should inform all subcomponents about its termination intention.

Actual exit is controlled by event loop reference count.
Whenever this count reaches zero, event loop exits.
Therefore subcomponents should 'manipulate' this reference count in order to prevent premature exit.

It is 'protected' function, should not be called from 'external' object.
*/
{
	if ev_is_active(&self->SIGINT_watcher) ev_signal_stop(self->loop, &self->SIGINT_watcher);
	if ev_is_active(&self->SIGTERM_watcher) ev_signal_stop(self->loop, &self->SIGTERM_watcher);
}


static void _event_loop_on_break_signal(struct ev_loop *loop, ev_signal *w, int revents)
{
	if (w->signum == SIGINT)
	{
		printf("\n");
	}

	struct event_loop * self = ev_userdata(loop);
	_event_loop_break(self);
}


static void _event_loop_on_cmdq_ready(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd_q * cmd_q = w->data;
	struct cmd cmd;

	while (true)
	{
		int qsize = cmd_q_get(cmd_q, &cmd);
		if (qsize < 0) break;
		//struct event_loop * this = cmd_q->data;

		printf("Got command %d - %d\n", cmd.id, qsize);

		if (qsize == 0) break; // This was last command in the queue, quit and save one mutex lock
	}

	printf("Done\n");
}


static PyObject * event_loop_listen(struct event_loop *self, PyObject *args)
{
	const char *hostname;
	const char *port;
	int backlog;

	if (!PyArg_ParseTuple(args, "ssi", &hostname, &port, &backlog)) return NULL;

	//TODO: This ...
	printf("Listen on %s %s %d\n", hostname, port, backlog);
	cmd_q_put(self->cmd_q, self->loop, 2, NULL);

	Py_RETURN_NONE;
}


/// Helpers that manipulates Python GIL prior and after event loop enters waiting/sleeping phase

static void _event_loop_release(struct ev_loop *loop)
{
    struct event_loop *self = ev_userdata(loop);
    self->tstate = PyEval_SaveThread();
}

static void _event_loop_acquire(struct ev_loop *loop)
{
    struct event_loop *self = ev_userdata(loop);
    PyEval_RestoreThread(self->tstate);
}


/// --- Python type definition follows

static PyMethodDef event_loop_methods[] = {
	{"run", (PyCFunction)event_loop_run, METH_NOARGS, "Enter event loop."},
	{"listen", (PyCFunction)event_loop_listen, METH_VARARGS, "Start listening on given host interface and port."},
    {NULL}  /* Sentinel */
};

PyTypeObject pyglev_core_event_loop_type = 
{
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyglev.core.event_loop",  /* tp_name */
	sizeof(struct event_loop), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)event_loop_dtor, /* tp_dealloc */
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
	Py_TPFLAGS_DEFAULT,        /* tp_flags */
	0,                         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    event_loop_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    event_loop_ctor,           /* tp_new */
};
