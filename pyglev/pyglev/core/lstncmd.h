#ifndef PYGLEV_LSTNCMD_H_
#define PYGLEV_LSTNCMD_H_

struct listen_cmd
{
/* Public Methods:
	...
*/
	PyObject_HEAD

	char *host;
	char *port;
	int backlog;

	char status;
	size_t socket_count;
};

// Scheduler shortcuts
void _listen_cmd_xschedule_01(struct listen_cmd *, struct event_loop * event_loop);

enum listen_cmd_statuses
{
	listen_cmd_status_INIT = 'I',
	listen_cmd_status_LISTENING = 'L',
};

extern PyTypeObject pyglev_core_listen_cmd_type;

#endif //PYGLEV_LSTNCMD_H_
