#ifndef PYGLEV_ESTSOCK_H_
#define PYGLEV_ESTSOCK_H_

struct est_socket
{
	struct est_socket * next; // Allow to form single-linked list

	PyObject * protocol;

	ev_io watcher;
};

struct est_socket * est_socket_new(struct event_loop * loop, PyObject *protocol, int socket);
void est_socket_delete(struct est_socket *);

#endif //PYGLEV_ESTSOCK_H_
