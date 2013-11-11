#include "greenev.h"

#include <netdb.h>
#include <sys/uio.h> // readv()

static void * _io_thread_start(void *);
static void _io_thread_cleanup(void * arg);
static void _on_iot_cmd(void * arg, struct cmd cmd);
static void _on_io_thread_accept(struct ev_loop *loop, struct ev_io *watcher, int revents);
static void _on_io_thread_readwrite(struct ev_loop *loop, struct ev_io *watcher, int revents);

///

void io_thread_ctor(struct io_thread * this)
{
	int rc;

	this->listening = NULL;
	this->established = NULL;

	// Create thread-specific event loop
	this->loop = ev_loop_new(EVFLAG_NOSIGMASK);
	if (this->loop == NULL)
	{
		LOG_ERROR("Failed to create IO thread event loop");
		abort();
	}

	this->cmd_q = cmd_q_new(this->loop, 256, _on_iot_cmd, this);

	// Create IO buffer (just temporary location of this code)
	this->tmp_io_buf = io_buffer_new();

	// Prepare creation of IO thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	// Temporarily block all signals - to ensure that newly created thread inherites this ...
	sigset_t new_set, orig_set;
	sigfillset(&new_set);
	rc = pthread_sigmask(SIG_SETMASK, &new_set, &orig_set);
	if (rc != 0)
	{
		LOG_ERROR("Failed to set sigmask for io thread: %d", rc);
		abort();
	}

	// Create thread
	rc = pthread_create(&this->thread, &attr, _io_thread_start, this);
	if (rc != 0)
	{
		LOG_ERROR("Failed to create io thread: %d", rc);
		abort();
	}

	// Restore original signal mask
	rc = pthread_sigmask(SIG_SETMASK, &orig_set, NULL);
	if (rc != 0)
	{
		LOG_ERROR("Failed to set sigmask for io thread (2): %d", rc);
		abort();
	}

	pthread_attr_destroy(&attr);

}

void io_thread_dtor(struct io_thread * this)
{
	cmd_q_delete(this->cmd_q);
	io_buffer_delete(this->tmp_io_buf);
}

///

void io_thread_die(struct io_thread * this)
{
	// Insert DIE command in my queue
	cmd_q_insert(this->cmd_q, iot_cmd_IO_THREAD_DIE, NULL);
}

///

static void * _io_thread_start(void * arg)
{
	struct io_thread * this = arg;

	pthread_cleanup_push(_io_thread_cleanup, arg);  	
	cmd_q_start(this->cmd_q);

	// Add artifical reference to prevent loop exit
	ev_ref(this->loop);

	// Start event loop
	for(bool keep_running=true; keep_running;)
	{
		keep_running = ev_run(this->loop, 0);
	}

	pthread_cleanup_pop(1);

	// Exit thread
	pthread_exit(NULL);
	return NULL;
}

static void _io_thread_cleanup(void * arg)
{
	struct io_thread * this = arg;
	cmd_q_stop(this->cmd_q);

	if (app != NULL) application_command(app, app_cmd_IO_THREAD_EXIT, arg);
}

///

static void _on_iot_cmd(void * arg, struct cmd cmd)
{
	struct io_thread * this = arg;

	switch (cmd.id)
	{

		case iot_cmd_IO_THREAD_DIE:
			{
				// Close all listening sockets
				for (struct io_thread_listening_socket * sockobj = this->listening; sockobj; sockobj = sockobj->next)
				{
					close(sockobj->watcher.fd);
					ev_io_stop(this->loop, &sockobj->watcher);
					this->listening = sockobj->next; // Remove for a list
					free(sockobj);
				}

				// Close all established sockets
				for (struct io_thread_established_socket * sockobj = this->established; sockobj; sockobj = sockobj->next)
				{
					close(sockobj->watcher.fd);
					ev_io_stop(this->loop, &sockobj->watcher);
					this->established = sockobj->next; // Remove for a list
					free(sockobj);
				}

				ev_unref(this->loop); // This should release artifical reference count for our loop and eventually exit iothread loop
			}
			break;


		case iot_cmd_IO_THREAD_LISTEN:
		// There is prepared listen socket, add that into our loop
			{
				struct io_thread_listening_socket * sockobj = cmd.arg;
				sockobj->next = this->listening;
				this->listening = sockobj;
				ev_io_start(this->loop, &sockobj->watcher);
			}
			break;


		case iot_cmd_IO_THREAD_ESTABLISH:
		// There is prepared established (connected) socket, add that into our loop
			{
				struct io_thread_established_socket * sockobj = cmd.arg;
				sockobj->next = this->established;
				this->established = sockobj;

				//TODO: Callback to Python code [SOCKET ESTABLISHED]
				//TODO: Get initial state of socket watcher (READ and/or write) - probably from a callback or some preset
				ev_io_set(&sockobj->watcher, sockobj->watcher.fd, EV_READ);
				ev_io_start(this->loop, &sockobj->watcher);
			}
			break;		

		default:
			LOG_WARNING("Unknown IO thread command '%d'", cmd.id);
	}

}

///

void io_thread_sockobj_close(struct io_thread * this, struct io_thread_established_socket * sockobj)
{
	assert(this == sockobj->io_thread);

	close(sockobj->watcher.fd);
	ev_io_stop(this->loop, &sockobj->watcher);

	//TODO: Callback to Python code [SOCKET CLOSED]

	struct io_thread_established_socket ** prev = &this->established;
	for (struct io_thread_established_socket * i = this->established; i; prev = &i->next, i = i->next)
	{
		if (i == sockobj)
		{
			*prev = sockobj->next;
			sockobj->next = NULL;
			free(sockobj);
			sockobj= NULL;
			break;
		}
	}
	assert(sockobj == NULL);

}

static void _on_io_thread_readwrite(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	const unsigned int wanted_iovcnt = 4;

	struct io_thread_established_socket * sockobj = watcher->data;
	struct io_thread * this = sockobj->io_thread;

	if (revents & EV_READ)
	{
		struct iovec iovec[wanted_iovcnt];
		int iovcnt = io_buffer_get(this->tmp_io_buf, wanted_iovcnt, iovec);
		if (iovcnt == 0)
		{
			LOG_WARNING("IO buffer is full.");
			return;
		}

		ssize_t size = readv(watcher->fd, iovec, iovcnt);

		if (size < 0) // ERROR ON SOCKET
		{
			LOG_ERRNO(errno, "Error on the socket (closing)");
			io_thread_sockobj_close(this, sockobj);
			io_buffer_put(this->tmp_io_buf, iovcnt, iovec); // Return IO buffers
			return;
		} else if (size == 0) // SOCKET CLOSED
		{
			io_thread_sockobj_close(this, sockobj);
			io_buffer_put(this->tmp_io_buf, iovcnt, iovec); // Return IO buffers
			return;
		} else {  // SOCKET READ DETECTED
			int used_iovcnt = 1+(	(size-1) / io_buffer_slot_size);

			if (used_iovcnt == wanted_iovcnt)
			{
				// This is kind of important information for fine-tuning
				LOG_DEBUG("Read exhausted available IO buffers");
			} else if (used_iovcnt < iovcnt)
			{
				// Return unused IO buffers
				io_buffer_put(this->tmp_io_buf, iovcnt-used_iovcnt, &iovec[used_iovcnt]);
			}

			//TODO: Callback to Python code [SOCKET READ] with iovec and used_iovcnt
		}
	}

	if (revents & EV_WRITE)
	{
			//TODO: Callback to Python code [SOCKET WRITE]
	}
}

///

void io_thread_listen(struct io_thread * this, const char * host, const char * port, int backlog)
{
	//TODO: This whole function can be executed in DETACHED thread (to remove eventual blocking when resolving DNS etc.)

	int rc, i;
	bool failure = false;

	// Resolve address/port into IPv4/IPv6 address infos
	struct addrinfo req, *ans;
	req.ai_family = AF_UNSPEC; // Both IPv4 and IPv6 if available
	req.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV | AI_V4MAPPED;
	req.ai_socktype = SOCK_STREAM;
	req.ai_protocol = IPPROTO_TCP;

	if (strcmp(host, "*") == 0) host = NULL;

	rc = getaddrinfo(host, port, &req, &ans);
	if (rc != 0)
	{
		LOG_ERROR("Failed to resolve listen address: (%d) %s", rc, gai_strerror(rc));
		return;
	}

	size_t count = 0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next)
	{
		count += 1;
	}

	int listen_socket[count];
	for (i=0; i<count; i++) listen_socket[i] = -1;

	i=0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next, i++)
	{
		char hoststr[NI_MAXHOST];
		char portstr[NI_MAXSERV];
		rc = getnameinfo(rp->ai_addr, sizeof(struct sockaddr), hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
		if (rc != 0)
		{
			LOG_WARNING("Problem occured when resolving listen socket: %s",gai_strerror(rc));
			strcpy(hoststr, "?");
			strcpy(portstr, "?");
		}

		// Create socket
		listen_socket[i] = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (listen_socket[i] < 0)
		{
			LOG_ERRNO(errno, "Failed creating listen socket");
			failure = true;
			goto end_freeaddrinfo;
		}

		// Set reuse address option
		int on = 1;
		rc = setsockopt(listen_socket[i], SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
		if (rc == -1)
		{
			LOG_ERRNO(errno, "Failed when setting option SO_REUSEADDR to listen socket");
			failure = true;
			goto end_freeaddrinfo;
		}

		// Bind socket
		rc = bind(listen_socket[i], rp->ai_addr, rp->ai_addrlen);
		if (rc != 0)
		{
			LOG_ERRNO(errno, "Failed when binding listen socket to %s %s", hoststr, portstr);
			failure = true;
			goto end_freeaddrinfo;
		}

		// Start listening
		rc = listen(listen_socket[i], backlog);
		if (rc != 0)
		{
			LOG_ERRNO(errno, "Listening on %s %s", hoststr, portstr);
			failure = true;
			goto end_freeaddrinfo;
		}
	}

	i=0;
	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next, i++)
	{
		struct io_thread_listening_socket * new_sockobj = malloc(sizeof(struct io_thread_listening_socket));
		new_sockobj->next = NULL;
		ev_io_init(&new_sockobj->watcher, _on_io_thread_accept, listen_socket[i], EV_READ);
		new_sockobj->io_thread = this;
		new_sockobj->watcher.data = new_sockobj;
		new_sockobj->domain = rp->ai_family;
		new_sockobj->type = rp->ai_socktype;
		new_sockobj->protocol = rp->ai_protocol;
		memcpy(&new_sockobj->address, rp->ai_addr, rp->ai_addrlen);
		new_sockobj->address_len = rp->ai_addrlen;

		cmd_q_insert(this->cmd_q, iot_cmd_IO_THREAD_LISTEN, new_sockobj);
	}

end_freeaddrinfo:
	freeaddrinfo(ans);

	if (failure)
	{
		LOG_WARNING("Issue when preparing listen sockets, no listen socket created");
		for (i=0; i<count; i++)
		{
			if (listen_socket[i] >= 0) close(listen_socket[i]);
		}
	}

}


static void _on_io_thread_accept(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	struct io_thread_listening_socket * listening_sockobj = watcher->data;

	if (revents & EV_ERROR)
	{
		LOG_ERROR("Listen socket (accept) got invalid event");
		return;
	}

	if (revents & EV_READ)
	{
		struct sockaddr_storage client_addr;
		socklen_t client_len = sizeof(struct sockaddr_storage);

		// Accept client request
		int client_socket = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
		if (client_socket < 0)
		{
			LOG_ERRNO(errno, "Accept on listening socket");
			return;
		}

		if (app == NULL)
		{
			close(client_socket);
			return;
		}

		// Add new socket object
		struct io_thread_established_socket * new_sockobj = malloc(sizeof(struct io_thread_established_socket));
		new_sockobj->next = listening_sockobj->io_thread->established;
		listening_sockobj->io_thread->established = new_sockobj;
		ev_io_init(&new_sockobj->watcher, _on_io_thread_readwrite, client_socket, 0);
		new_sockobj->io_thread = listening_sockobj->io_thread;
		new_sockobj->watcher.data = new_sockobj;
		new_sockobj->domain = listening_sockobj->domain;
		new_sockobj->type = listening_sockobj->type;
		new_sockobj->protocol = listening_sockobj->protocol;
		memcpy(&new_sockobj->address, &client_addr, client_len);
		new_sockobj->address_len = client_len;

		//TODO: Callback to Python code [SOCKET ESTABLISHED]
		//TODO: Get initial state of socket watcher (READ and/or write) - probably from a callback or some preset
		ev_io_set(&new_sockobj->watcher, new_sockobj->watcher.fd, EV_READ);
		ev_io_start(listening_sockobj->io_thread->loop, &new_sockobj->watcher);
	}

}

///

void io_thread_connect(struct io_thread * this, const char * host, const char * port)
{
	//TODO: This whole function can be executed in DETACHED thread (to remove eventual blocking when resolving DNS etc.)

	int rc;

	// Resolve address/port into IPv4/IPv6 address infos
	struct addrinfo req, *ans;
	req.ai_family = AF_UNSPEC; // Both IPv4 and IPv6 if available
	req.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV | AI_V4MAPPED;
	req.ai_socktype = SOCK_STREAM;
	req.ai_protocol = IPPROTO_TCP;

	rc = getaddrinfo(host, port, &req, &ans);
	if (rc != 0)
	{
		LOG_ERROR("Failed to resolve connect address: (%d) %s", rc, gai_strerror(rc));
		return;
	}

	int connect_socket = -1;
	int connect_errno = 0;
	char hoststr[NI_MAXHOST];
	char portstr[NI_MAXSERV];
	strcpy(hoststr, "?");
	strcpy(portstr, "?");

	for (struct addrinfo *rp = ans; rp != NULL; rp = rp->ai_next)
	{
		rc = getnameinfo(rp->ai_addr, sizeof(struct sockaddr), hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
		if (rc != 0)
		{
		
			LOG_WARNING("Problem occured when resolving connect socket: %s",gai_strerror(rc));
			strcpy(hoststr, "?");
			strcpy(portstr, "?");
		}

		// Create socket
		connect_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (connect_socket < 0)
		{
			connect_errno = errno;
			continue;
		}

		// Set reuse address option
		int on = 1;
		rc = setsockopt(connect_socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on));
		if (rc == -1)
		{
			connect_errno = errno;
			close(connect_socket);
			connect_socket = -1;
			continue;
		}

		// Connect socket
		rc = connect(connect_socket, rp->ai_addr, rp->ai_addrlen);
		if (rc == -1)
		{
			connect_errno = errno;
        	close(connect_socket);
        	connect_socket = -1;
        	continue;
        }

		// Add new socket object
		struct io_thread_established_socket * new_sockobj = malloc(sizeof(struct io_thread_established_socket));
		new_sockobj->next =  NULL;
		ev_io_init(&new_sockobj->watcher, _on_io_thread_readwrite, connect_socket, 0);
		new_sockobj->io_thread = this;
		new_sockobj->watcher.data = new_sockobj;
		new_sockobj->domain = rp->ai_family;
		new_sockobj->type = rp->ai_socktype;
		new_sockobj->protocol = rp->ai_protocol;
		memcpy(&new_sockobj->address, rp->ai_addr, rp->ai_addrlen);
		new_sockobj->address_len = rp->ai_addrlen;

		cmd_q_insert(this->cmd_q, iot_cmd_IO_THREAD_ESTABLISH, new_sockobj);

		break;
	}

	if (connect_errno != 0)
	{
		LOG_ERRNO(connect_errno, "Error when establishing connection to %s %s", hoststr, portstr);
	}

	freeaddrinfo(ans);
}
