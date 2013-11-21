#ifndef PYGLEV_LSTNCMD_H_
#define PYGLEV_LSTNCMD_H_

struct listen_cmd
{
/* Public Methods:
	...
*/
	PyObject_HEAD

	struct listen_cmd * next; // Allow to form single-linked list

	char *host;
	char *port;
	int backlog;

	char status;

	size_t watchers_count;
	ev_io * watchers; /* Allocated array (length is specified by watchers_count) */
};

// Scheduler shortcuts
// PROTECTED / should be called only from event loop that owns this command
void _listen_cmd_xschedule_01(struct listen_cmd *, struct event_loop * event_loop);

// Initiate termination
// PROTECTED / should be called only from event loop that owns this command
void _listen_cmd_close(struct listen_cmd *, struct event_loop * event_loop);

enum listen_cmd_statuses
{
	listen_cmd_status_INIT = 'I',
	listen_cmd_status_RESOLVING = 'R',
	listen_cmd_status_LISTENING = 'L',
	listen_cmd_status_CLOSED = 'C',
};

extern PyTypeObject pyglev_core_listen_cmd_type;

#endif //PYGLEV_LSTNCMD_H_
