#ifndef GREENEV_IOTHREAD_H_
#define GREENEV_IOTHREAD_H_

struct io_thread
{
	pthread_t thread;
	struct ev_loop * loop;

	// Command queue
	struct cmd_q cmd_q;
};

void io_thread_ctor(struct io_thread *);
void io_thread_dtor(struct io_thread *);

// This method is callable from other threads
static inline void io_thread_command(struct io_thread * this, int iot_cmd_id, void * arg)
{
	return cmd_q_insert(&this->cmd_q, iot_cmd_id, arg);
}

void io_thread_die(struct io_thread *);

enum iot_cmd_ids {
	iot_cmd_IO_THREAD_DIE = 1,
};

#endif //GREENEV_IOTHREAD_H_
