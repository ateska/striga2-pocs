#ifndef PYGLEV_EVLOOP_H_
#define PYGLEV_EVLOOP_H_

struct event_loop
{
/* Public Methods:
	run()
	schedule()
	_xschedule()
*/

	PyObject_HEAD

	struct ev_loop * loop;
	PyThreadState * tstate;

	struct cmd_q * cmd_q;

	ev_signal SIGINT_watcher;
	ev_signal SIGTERM_watcher;

	PyObject * on_error;

	struct listen_cmd * listen_commands;
};

// Used to propagate error into application
// PROTECTED / should be only called from thread that owns event_loop / will acquire GIL
void _event_loop_error(struct event_loop *, PyObject * subject, int error_type, int error_code, const char * restrict format, ...);

extern PyTypeObject pyglev_core_event_loop_type;


#endif //PYGLEV_EVLOOP_H_
