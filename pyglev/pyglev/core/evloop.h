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
};

extern PyTypeObject pyglev_core_event_loop_type;

#endif //PYGLEV_EVLOOP_H_
