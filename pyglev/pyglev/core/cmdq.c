#include "pyglev.h"
///

struct cmd_q * cmd_q_new(unsigned int size, void (*callback)(struct ev_loop *loop, ev_async *watcher, int revents))
{
	assert(size > 0);

	struct cmd_q * this = malloc(sizeof(struct cmd_q) + size * sizeof(struct cmd));
	if (this == NULL) return NULL;

	this->size = size;
	this->head = 0;
	this->tail = 0;
	this->data = NULL;

	// Initialize mutex
	pthread_mutex_init(&this->mutex, NULL);

	// Prepare asynch watcher for application command queue
	ev_async_init(&this->watcher, callback);
	this->watcher.data = this;

	return this;
}


void cmd_q_delete(struct cmd_q * this)
{
	assert(!ev_is_active(&this->watcher));

	pthread_mutex_destroy(&this->mutex);

//TODO: This ...
/*	if (this->head != this->tail)
	{
		LOG_WARN("There is some content left in a command queue.");
	} */

	free(this);
}

///

void cmd_q_start(struct cmd_q * this, struct ev_loop *loop)
{
	ev_async_start(loop, &this->watcher);
	ev_unref(loop);
}

void cmd_q_stop(struct cmd_q * this, struct ev_loop *loop)
{
	ev_ref(loop);
	ev_async_stop(loop, &this->watcher);
}

///

bool cmd_q_put(struct cmd_q * this, struct ev_loop *loop, int cmd_id, PyObject * subject)
{
	if (!ev_is_active(&this->watcher)) return false;

	struct cmd new_cmd = {.id = cmd_id, .subject = subject};

	// Critical (synchronised) section
	pthread_mutex_lock(&this->mutex);

	unsigned int pos = this->head;
	this->head += 1;
	if (this->head == this->size) this->head = 0;

	if (this->head == this->tail)
	{
		this->head = pos;
		pthread_mutex_unlock(&this->mutex);
		return false;
	}

	this->queue[pos] = new_cmd;

	pthread_mutex_unlock(&this->mutex);

	ev_async_send(loop, &this->watcher);
	return true;
}


int cmd_q_get(struct cmd_q * this, struct cmd * out_cmd)
{
	// Critical (synchronised) section
	pthread_mutex_lock(&this->mutex);

	if (this->head == this->tail) // Command queue is empty
	{
		pthread_mutex_unlock(&this->mutex);	
		return -1;
	}

	*out_cmd = this->queue[this->tail];

	this->tail += 1;
	if (this->tail == this->size) this->tail = 0;

	int size;
	if (this->tail > this->head)
	{
		size = this->head + (this->size - this->tail);
	} else {
		size = this->head - this->tail;
	}	

	pthread_mutex_unlock(&this->mutex);

	return size;
}
