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

	size_t socket_count;	
};

// Scheduler shortcuts
void _listen_cmd_xschedule_01(struct listen_cmd *, struct event_loop * event_loop);

extern PyTypeObject pyglev_core_listen_cmd_type;

#endif //PYGLEV_LSTNCMD_H_
