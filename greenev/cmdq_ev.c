/*

Watcher-based (libev) command queue class implementation

*/
#include "greenev.h"

static void _on_watcher_cmd_q_async(struct ev_loop *loop, ev_async *w, int revents);

///

struct watcher_cmd_q * watcher_cmd_q_new(struct ev_loop *loop, unsigned int size, void (*cmdcallback)(void *, struct cmd), void * callbackarg)
{
	assert(size > 0);

	struct watcher_cmd_q * this = malloc(sizeof(struct watcher_cmd_q) + size * sizeof(struct cmd));
	if (this == NULL) return NULL;

	this->q_size = size;
	this->q_head = 0;
	this->q_tail = 0;

	this->loop = loop;

	this->cmdcallback = cmdcallback;
	this->callbackarg = callbackarg;

	// Initialize mutex
	pthread_mutex_init(&this->q_mtx, NULL);

	// Prepare asynch watcher for application command queue
	ev_async_init(&this->q_watcher, _on_watcher_cmd_q_async);
	this->q_watcher.data = this;

	return this;
}

void watcher_cmd_q_delete(struct watcher_cmd_q * this)
{
	watcher_cmd_q_stop(this);
	pthread_mutex_destroy(&this->q_mtx);

	if (this->q_head != this->q_tail)
	{
		LOG_WARNING("There is some content left in a command quueue.");
	}

	free(this);
}

///

void watcher_cmd_q_start(struct watcher_cmd_q * this)
{
	ev_async_start(this->loop, &this->q_watcher);

	// Don't count this watcher into loop exit blockers
	ev_unref(this->loop);
}

void watcher_cmd_q_stop(struct watcher_cmd_q * this)
{
	ev_async_stop(this->loop, &this->q_watcher);
}

///

bool watcher_cmd_q_put(struct watcher_cmd_q * this, int cmd_id, void * arg)
{
	struct cmd new_cmd = {.id = cmd_id, .arg = arg};

	// Critical (synchronised) section
	pthread_mutex_lock(&this->q_mtx);

	unsigned int q_pos = this->q_head;
	this->q_head += 1;
	if (this->q_head == this->q_size) this->q_head = 0;

	if (this->q_head == this->q_tail)
	{
		this->q_head = q_pos;
		pthread_mutex_unlock(&this->q_mtx);
		return false;
	}

	this->q[q_pos] = new_cmd;

	pthread_mutex_unlock(&this->q_mtx);

	ev_async_send(this->loop, &this->q_watcher);
	return true;
}


static void _on_watcher_cmd_q_async(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd cmd;
	struct watcher_cmd_q * this = w->data;

	for(;;)
	{
		// Critical (synchronised) section
		pthread_mutex_lock(&this->q_mtx);

		if (this->q_head == this->q_tail) // Command queue is empty
		{
			pthread_mutex_unlock(&this->q_mtx);	
			break;
		}

		cmd = this->q[this->q_tail];

		this->q_tail += 1;
		if (this->q_tail == this->q_size) this->q_tail = 0;

		pthread_mutex_unlock(&this->q_mtx);

		this->cmdcallback(this->callbackarg, cmd);
	}

}