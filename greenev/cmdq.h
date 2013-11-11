#ifndef GREENEV_CMDQ_H_
#define GREENEV_CMDQ_H_

struct cmd
{
	int id;
	void * arg;
};

struct watcher_cmd_q
/*
Watcher-based (libev watcher) command queue.
It uses ev_async watcher from libev to synchronize producers and consumers.
This one is useful when processing thread has to handle libev events (e.g. socket or signals)
*/
{
	unsigned int q_size;
	unsigned int q_head;
	unsigned int q_tail;


	pthread_mutex_t q_mtx;
	struct ev_async q_watcher;

	struct ev_loop *loop;

	void (*cmdcallback)(void *, struct cmd);
	void * callbackarg;

	struct cmd q[];
};


struct watcher_cmd_q * watcher_cmd_q_new(struct ev_loop *loop, unsigned int size, void (*cmdcallback)(void *, struct cmd), void * callbackarg);
void watcher_cmd_q_delete(struct watcher_cmd_q *);

void watcher_cmd_q_start(struct watcher_cmd_q *); /* The same sence as libev ev_TYPE_start (loop, ...)*/
void watcher_cmd_q_stop(struct watcher_cmd_q *); /* Is also implicty called by destructor */

// This method is callable from other threads
bool watcher_cmd_q_insert(struct watcher_cmd_q * this, int cmd_id, void * arg);

#endif //GREENEV_CMDQ_H_
