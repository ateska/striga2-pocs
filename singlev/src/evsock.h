#ifndef SINGLEV_EVSOCKETS_H_
#define SINGLEV_EVSOCKETS_H_

struct listening_ev_socket
{
	struct listening_ev_socket * next;
	struct ev_io watcher;

	int domain;
	int type;
	int protocol;

	struct sockaddr_storage address;
	socklen_t address_len;
};

struct established_ev_socket
{
	struct established_ev_socket * next;
	struct ev_io watcher;

	int domain;
	int type;
	int protocol;

	struct sockaddr_storage address;
	socklen_t address_len;
};

#endif //SINGLEV_EVSOCKETS_H_
