#include "greenev.h"

const size_t io_buffer_slot_size = 1024*16;

///

struct io_buffer * io_buffer_new()
{
	const unsigned int slot_count = 1024*8;

	size_t alloc_size = sizeof(struct io_buffer) + io_buffer_slot_size*slot_count;
	LOG_DEBUG("Allocating %0.2f Mbytes for IO buffer", alloc_size / 1048576.);
	
	struct io_buffer * this = malloc(alloc_size);
	this->slot_count = slot_count;
	this->free_slots_head = slot_count;
	this->free_slots_tail = 0;
	this->data = &this->free_slots[slot_count];
	for (unsigned int i = 0; i < this->slot_count; i++)
	{
		this->free_slots[i] = this->data + (io_buffer_slot_size * i);
	}

	// Initialize mutex
	pthread_mutex_init(&this->mtx, NULL);

	return this;
}

void io_buffer_delete(struct io_buffer * this)
{
	pthread_mutex_destroy(&this->mtx);

	free(this);
}

///

bool io_buffer_empty(struct io_buffer * this)
{
        return (this->free_slots_tail == this->free_slots_head);
}

unsigned int io_buffer_get(struct io_buffer * this, unsigned int slot_count, struct iovec * iovec)
{
	unsigned int i;

	// Critical (synchronised) section
	pthread_mutex_lock(&this->mtx);

	for (i = 0; i < slot_count; i++)
	{
		if (io_buffer_empty(this)) break;

		iovec[i].iov_base = this->free_slots[this->free_slots_tail];
		iovec[i].iov_len = io_buffer_slot_size;

		this->free_slots_tail += 1;
        if (this->free_slots_tail == this->slot_count) this->free_slots_tail = 0;
	}

	pthread_mutex_unlock(&this->mtx);

	return i;
}

void io_buffer_put(struct io_buffer * this, unsigned int slot_count, struct iovec * iovec)
{
	unsigned int i;

	// Critical (synchronised) section
	pthread_mutex_lock(&this->mtx);

	for (i = 0; i < slot_count; i++)
	{
		this->free_slots_head += 1;
        if (this->free_slots_head == this->slot_count) this->free_slots_head = 0;

        assert(this->free_slots_head != this->free_slots_tail); // This should never happen

		this->free_slots[this->free_slots_head] = iovec[i].iov_base;
	}

	pthread_mutex_unlock(&this->mtx);

}
