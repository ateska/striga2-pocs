#ifndef GREENEV_PYTHREAD_H_
#define GREENEV_PYTHREAD_H_

struct py_thread
{
	pthread_t thread;

};


void py_thread_ctor(struct py_thread *);
void py_thread_dtor(struct py_thread *);


#endif //GREENEV_PYTHREAD_H_