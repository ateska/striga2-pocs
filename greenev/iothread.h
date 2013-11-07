#ifndef GREENEV_IOTHREAD_H_
#define GREENEV_IOTHREAD_H_

struct io_thread_listening_socket
{
	struct io_thread_listening_socket * next;
	struct io_thread * io_thread;
	struct ev_io watcher;

	int domain;
	int type;
	int protocol;

	struct sockaddr_storage address;
	socklen_t address_len;
};

struct io_thread_established_socket
{
	struct io_thread_established_socket * next;
	struct io_thread * io_thread;
	struct ev_io watcher;

	int domain;
	int type;
	int protocol;

	struct sockaddr_storage address;
	socklen_t address_len;
};


struct io_thread
{
	pthread_t thread;
	struct ev_loop * loop;

	// Connections ...
	struct io_thread_listening_socket * listening;
	struct io_thread_established_socket * established;

	// Command queue
	struct cmd_q cmd_q;
};

void io_thread_ctor(struct io_thread *);
void io_thread_dtor(struct io_thread *);

// This method is callable from other threads
void io_thread_die(struct io_thread *);

// This methid is callable from other threads
void io_thread_listen(struct io_thread *, const char * host, const char * port, int backlog);

enum iot_cmd_ids {
	iot_cmd_IO_THREAD_DIE = 1,
	iot_cmd_IO_THREAD_LISTEN = 2,
};

#endif //GREENEV_IOTHREAD_H_
