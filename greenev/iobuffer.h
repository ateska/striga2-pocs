#ifndef GREENEV_IOBUFFER_H_
#define GREENEV_IOBUFFER_H_

struct io_buffer
{
	void * data;
	pthread_mutex_t mtx;

	unsigned int slot_count;
	unsigned int free_slots_head;
	unsigned int free_slots_tail;

	void * free_slots[];
};

extern const size_t io_buffer_slot_size;

struct io_buffer * io_buffer_new();
void io_buffer_delete(struct io_buffer *);

bool io_buffer_empty(struct io_buffer * this);

unsigned int io_buffer_get(struct io_buffer *, unsigned int slot_count, struct iovec * iovec);
void io_buffer_put(struct io_buffer *, unsigned int slot_count, struct iovec * iovec);


#endif //GREENEV_IOBUFFER_H_
