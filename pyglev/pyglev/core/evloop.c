#include "pyglev.h" 

static void _event_loop_on_break_signal(struct ev_loop *loop, ev_signal *w, int revents);
static void _event_loop_on_cmdq_ready(struct ev_loop *loop, ev_async *w, int revents);

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
	self->listen_commands = NULL;

	// Create event loop
	self->loop = ev_loop_new(flags);
	if (self->loop == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "failed to create event loop");
		Py_DECREF(self);
		return NULL;
    }
	ev_set_userdata(self->loop, self);


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


static int event_loop_tp_clear(struct event_loop *self)
{
	Py_CLEAR(self->on_error);
    return 0;
}

static int event_loop_tp_traverse(struct event_loop *self, visitproc visit, void *arg)
{
    Py_VISIT(self->on_error);
    return 0;
}


static void event_loop_dtor(struct event_loop * self)
{
	printf("DEBUG: event_loop_dtor()\n");

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

	event_loop_tp_clear(self);
	Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject * event_loop_run(struct event_loop *self)
{
	bool keep_running=true;

	// Release Python GIL
	self->tstate = PyEval_SaveThread();

	// Start event loop
	while(keep_running)
	{
		keep_running = ev_run(self->loop, 0);
	}

	// Acquire Python GIL
	PyEval_RestoreThread(self->tstate);

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

	for (struct listen_cmd * listen_cmd = self->listen_commands; listen_cmd!=NULL; self->listen_commands=listen_cmd=listen_cmd->next)
	{
		_listen_cmd_close(listen_cmd, self);
	}
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
	struct event_loop * self = ev_userdata(loop);

	while (true)
	{
		int qsize = cmd_q_get(cmd_q, &cmd);
		if (qsize < 0) break;
		//struct event_loop * this = cmd_q->data;

		switch (cmd.id)
		{
			case pyglev_evloop_cmd_CALLABLE:
			{
				// Acquire Python GIL
				PyEval_RestoreThread(self->tstate);

				PyObject * args = PyTuple_New(0);

				PyObject * res = PyObject_CallObject(cmd.subject, args);
				if (res == NULL)
				{
					PyErr_Print();
				} else {
					Py_DECREF(res); // Throw result away
				}

				Py_DECREF(args);

				// We don't need to own reference to cmd.subject anymore
				Py_DECREF(cmd.subject);

				// Release Python GIL
				self->tstate = PyEval_SaveThread();
			}
			break;


			case pyglev_evloop_cmd_LISTEN_CMD_01:
			{
				struct listen_cmd * listen_cmd = (struct listen_cmd *)cmd.subject;
				
				// Add it to my listen commands list
				listen_cmd->next = self->listen_commands;
				self->listen_commands = listen_cmd;

				// Execute first step in xschedule
				_listen_cmd_xschedule_01(listen_cmd, self);
			}
			break;

			default:
				PyEval_RestoreThread(self->tstate);

				// Set exception
				PyObject * msg = PyUnicode_FromFormat("Unknown command id: %d", cmd.id);
				PyErr_SetObject(PyExc_RuntimeError, msg);
				Py_DECREF(msg);

				// Print exception
				PyErr_WriteUnraisable(NULL);

				self->tstate = PyEval_SaveThread();
		}

		if (qsize == 0) break; // This was last command in the queue, quit and save one mutex lock
	}

}


static PyObject * event_loop_schedule(struct event_loop *self, PyObject *args)
{
	PyObject * callable;

	if (!PyArg_ParseTuple(args, "O:schedule", &callable)) return NULL;

	bool ret = cmd_q_put(self->cmd_q, self->loop, pyglev_evloop_cmd_CALLABLE, callable);
	if (!ret)
	{
		PyErr_SetString(PyExc_RuntimeError, "Command queue is refusing new commands");
		return NULL;
	}

	Py_INCREF(callable);

	Py_RETURN_NONE;
}


static PyObject * event_loop_xschedule(struct event_loop *self, PyObject *args)
{
	PyObject * callable;

	if (!PyArg_ParseTuple(args, "O:xschedule", &callable)) return NULL;

	int cmdid = -1;
	if (PyObject_TypeCheck(callable, &pyglev_core_listen_cmd_type))
	{
		cmdid = pyglev_evloop_cmd_LISTEN_CMD_01;
	} else {
		PyErr_SetString(PyExc_RuntimeError, "There is no extended scheduling for this object");
		return NULL;		
	}

	assert(cmdid != -1);

	bool ret = cmd_q_put(self->cmd_q, self->loop, cmdid, callable);
	if (!ret)
	{
		PyErr_SetString(PyExc_RuntimeError, "Command queue is refusing new commands");
		return NULL;
	}

	Py_INCREF(callable);

	Py_RETURN_NONE;
}

/// --- Python type definition follows

static PyMethodDef event_loop_methods[] = {
	{"run", (PyCFunction)event_loop_run, METH_NOARGS, "Enter event loop."},
	{"schedule", (PyCFunction)event_loop_schedule, METH_VARARGS, "Add callable to command queue."},
	{"_xschedule", (PyCFunction)event_loop_xschedule, METH_VARARGS, "Add 'special' object to command queue"},
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
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,        /* tp_flags */
	0,                         /* tp_doc */
    (traverseproc)event_loop_tp_traverse,    /* tp_traverse */
    (inquiry)event_loop_tp_clear,       /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    event_loop_methods,        /* tp_methods */
    event_loop_members,        /* tp_members */
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
