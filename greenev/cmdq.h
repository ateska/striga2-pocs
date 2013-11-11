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
bool watcher_cmd_q_put(struct watcher_cmd_q *, int cmd_id, void * arg);

///

struct cond_cmd_q
/*
Command queue based on Posix conditional variable.

Synchronization is done purely via Posix condition variables.
Therefore this is the only element that can feed thread event loop.
*/
{
	unsigned int q_size;
	unsigned int q_head;
	unsigned int q_tail;

	pthread_mutex_t q_mtx;
	pthread_cond_t q_not_full;
	pthread_cond_t q_not_empty;

	struct cmd q[];
};

struct cond_cmd_q * cond_cmd_q_new(unsigned int size);
void cond_cmd_q_delete(struct cond_cmd_q *);

unsigned int cond_cmd_q_size(struct cond_cmd_q *);
bool cond_cmd_q_empty(struct cond_cmd_q *);
bool cond_cmd_q_full(struct cond_cmd_q *);

bool cond_cmd_q_put(struct cond_cmd_q *, int cmd_id, void * arg);
int cond_cmd_q_timed_put(struct cond_cmd_q * this, int cmd_id, void * arg, unsigned int timeout_ms);
/*
Return values:
-1 - error
0 - timeout occured (item is not inserted)
1 - item is successfully inserted into queue
*/

bool cond_cmd_q_get(struct cond_cmd_q *, struct cmd * cmd_out);
int cond_cmd_q_timed_get(struct cond_cmd_q *, struct cmd * cmd_out, unsigned int timeout_ms);
/*
Return values:
-1 - error
0 - timeout occured (item is not inserted)
1 - item is successfully inserted into queue
*/

#endif //GREENEV_CMDQ_H_
