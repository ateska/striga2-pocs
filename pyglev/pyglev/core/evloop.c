#include "pyglev.h" 

static void event_loop_on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents);

///

static PyObject * event_loop_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	unsigned int flags = EVFLAG_AUTO;
	static char *kwlist[] = {"flags", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|I:new", kwlist,
		&flags
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

	// Create and register SIGINT signal handler
	ev_signal_init(&self->SIGINT_watcher, event_loop_on_terminating_signal, SIGINT);
	ev_signal_start(self->loop, &self->SIGINT_watcher);
	self->SIGINT_watcher.data = self;

	// Create and register SIGTERM signal handler
	ev_signal_init(&self->SIGTERM_watcher, event_loop_on_terminating_signal, SIGTERM);
	ev_signal_start(self->loop, &self->SIGTERM_watcher);
	self->SIGTERM_watcher.data = self;


	return (PyObject *)self;
}

static void event_loop_dealloc(struct event_loop * self)
{
	if (self->loop)
	{
		ev_signal_stop(self->loop, &self->SIGINT_watcher);
		ev_signal_stop(self->loop, &self->SIGTERM_watcher);

		ev_loop_destroy(self->loop);
		self->loop = NULL;
    }
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


static void _event_loop_die(struct event_loop * this)
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
//	if (this->run_phase == app_run_phase_DYING) return;
//	if (this->run_phase == app_run_phase_EXIT) return;

//	assert(this->run_phase == app_run_phase_RUN);
//	this->run_phase = app_run_phase_DYING;

	ev_signal_stop(this->loop, &this->SIGINT_watcher);
	ev_signal_stop(this->loop, &this->SIGTERM_watcher);	
}


static void event_loop_on_terminating_signal(struct ev_loop *loop, ev_signal *w, int revents)
{
	if (w->signum == SIGINT)
	{
		printf("\n");
	}

	struct event_loop * self = w->data;
	_event_loop_die(self);
}

/// --- Python type definition follows

static PyMethodDef event_loop_methods[] = {
	{"run", (PyCFunction)event_loop_run, METH_NOARGS, "Enter event loop."},
    {NULL}  /* Sentinel */
};

PyTypeObject pyglev_core_event_loop_type = 
{
	PyVarObject_HEAD_INIT(NULL, 0)
	"pyglev.core.event_loop",  /* tp_name */
	sizeof(struct event_loop), /* tp_basicsize */
	0,                         /* tp_itemsize */
	(destructor)event_loop_dealloc, /* tp_dealloc */
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
    event_loop_new,            /* tp_new */
};
