#ifndef GREENEV_CMDQ_H_
#define GREENEV_CMDQ_H_

#define CMD_Q_DEPTH 200

struct cmd
{
	int id;
	void * arg;
};

struct cmd_q
{
	struct cmd q[CMD_Q_DEPTH];
	int q_pos;
	pthread_mutex_t q_mtx;
	struct ev_async q_watcher;

	struct ev_loop *loop;

	void (*cmdcallback)(void *, struct cmd);
	void * callbackarg;
};


void cmd_q_ctor(struct cmd_q *, struct ev_loop *loop, void (*cmdcallback)(void *, struct cmd), void * callbackarg);
void cmd_q_dtor(struct cmd_q *);

// This method is callable from other threads
void cmd_q_insert(struct cmd_q * this, int cmd_id, void * arg);

#endif //GREENEV_CMDQ_H_
