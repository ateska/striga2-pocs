#ifndef SINGLEV_CMDQ_H_
#define SINGLEV_CMDQ_H_

struct cmd
{
	int id;
	void * data;
};

struct cmd_q
/*
Watcher-based (libev watcher) command queue.
It uses ev_async watcher from libev to synchronize producers and consumers.
This one is useful when processing thread has to handle libev events (e.g. socket or signals)
*/
{
	unsigned int size;
	unsigned int head;
	unsigned int tail;

	pthread_mutex_t mutex;
	struct ev_async watcher;

	void * data; //This is purely for user use (the same way as watcher->data)

	struct cmd queue[];
};


struct cmd_q * cmd_q_new(unsigned int size, void (*callback)(struct ev_loop *loop, ev_async *watcher, int revents));
void cmd_q_delete(struct cmd_q *);

void cmd_q_start(struct cmd_q *, struct ev_loop *loop); /* The same sence as libev ev_TYPE_start (loop, ...)*/
void cmd_q_stop(struct cmd_q *, struct ev_loop *loop); /* Is also implicty called by destructor */

// This method is callable from other threads
bool cmd_q_put(struct cmd_q *, struct ev_loop *loop, int cmd_id, void * data);

int cmd_q_get(struct cmd_q *, struct cmd * out_cmd);

///

#endif //SINGLEV_CMDQ_H_
