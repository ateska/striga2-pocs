#ifndef GREENEV_IOTHREAD_H_
#define GREENEV_IOTHREAD_H_

struct io_thread
{
	pthread_t thread;
};

void io_thread_ctor(struct io_thread *);
void io_thread_dtor(struct io_thread *);

void io_thread_die(struct io_thread *);

#endif //GREENEV_IOTHREAD_H_
