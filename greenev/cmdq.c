#include "greenev.h"

static void _on_cmd_q_async(struct ev_loop *loop, ev_async *w, int revents);

///

void cmd_q_ctor(struct cmd_q * this, struct ev_loop *loop, void (*cmdcallback)(void *, struct cmd), void * callbackarg)
{
	this->q_pos = 0;
	this->loop = loop;

	this->cmdcallback = cmdcallback;
	this->callbackarg = callbackarg;

	// Initialize mutex
	pthread_mutex_init(&this->q_mtx, NULL);

	// Prepare asynch watcher for application command queue
	ev_async_init(&this->q_watcher, _on_cmd_q_async);
	this->q_watcher.data = this;
	ev_async_start(loop, &this->q_watcher);

	// Don't count this watcher into loop exit blockers
	ev_unref(loop);
}

void cmd_q_dtor(struct cmd_q * this)
{
	ev_async_stop(this->loop, &this->q_watcher);

	pthread_mutex_destroy(&this->q_mtx);
}

///

void cmd_q_insert(struct cmd_q * this, int cmd_id, void * arg)
{
	struct cmd new_cmd = {.id = cmd_id, .arg = arg};

	// Critical (synchronised) section
	pthread_mutex_lock(&this->q_mtx);
	this->q[this->q_pos++] = new_cmd;
	pthread_mutex_unlock(&this->q_mtx);

	ev_async_send(this->loop, &this->q_watcher);
}

static void _on_cmd_q_async(struct ev_loop *loop, ev_async *w, int revents)
{
	struct cmd cmd;
	struct cmd_q * this = w->data;

	for(;;)
	{
		// Critical (synchronised) section
		pthread_mutex_lock(&this->q_mtx);
		if (this->q_pos == 0)
		{
			pthread_mutex_unlock(&this->q_mtx);	
			break;
		}
		cmd = this->q[--this->q_pos];
		pthread_mutex_unlock(&this->q_mtx);

		this->cmdcallback(this->callbackarg, cmd);
	}

}