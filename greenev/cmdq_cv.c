#include "greenev.h"

// Local forwards
static void _cond_cmd_q_getabstimeout(struct timespec * ts, unsigned int timeout_ms);

///

struct cond_cmd_q * cond_cmd_q_new(unsigned int size)
{
	int err;

	assert(size > 0);

	struct cond_cmd_q * this = malloc(sizeof(struct cond_cmd_q) + size * sizeof(struct cmd));
	if (this == NULL) return NULL;

	this->q_size = size;
	this->q_head = 0;
	this->q_tail = 0;

	// Initialize mutex
	err = pthread_mutex_init(&this->q_mtx, NULL);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_new pthread_mutex_init failed");
		goto end_free_me;
	}

	// Initialize 'not full' conditional variable
	err = pthread_cond_init(&this->q_not_full, NULL);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_new pthread_cond_init (q_not_full) failed");
		goto end_destroy_mtx;
	}

	// Initialize 'not empty' conditional variable
	err = pthread_cond_init(&this->q_not_empty, NULL);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_new pthread_cond_init (q_not_empty) failed");
		goto end_destroy_not_full;
	}

	return this;

end_destroy_not_full:
	pthread_cond_destroy(&this->q_not_full);

end_destroy_mtx:
	pthread_mutex_destroy(&this->q_mtx);

end_free_me:
	free(this);
	return NULL;

}


void cond_cmd_q_delete(struct cond_cmd_q * this)
{
	int err;

	assert(this != NULL);

	err = pthread_mutex_destroy(&this->q_mtx);
	if (err)
	{
		LOG_WARNING("cond_cmd_q_delete pthread_mutex_destroy failed");
	}

	err = pthread_cond_destroy(&this->q_not_full);
	if (err)
	{
		LOG_WARNING("cond_cmd_q_delete pthread_cond_destroy (q_not_full) failed");
	}

	err = pthread_cond_destroy(&this->q_not_empty);
	if (err)
	{
		LOG_WARNING("cond_cmd_q_delete pthread_cond_destroy (q_not_empty) failed");
	}
}

///

bool cond_cmd_q_full(struct cond_cmd_q * this)
{
	if (this->q_tail > 0)
	{
		return (this->q_tail == (this->q_head + 1));
	}

	return (this->q_head == (this->q_size - 1));
}


bool cond_cmd_q_empty(struct cond_cmd_q * this)
{
	return (this->q_tail == this->q_head);
}


unsigned int cond_cmd_q_size(struct cond_cmd_q * this)
{
	if (this->q_tail > this->q_head)
	{
		return (this->q_tail == (this->q_head + 1));
	} else {
		return (this->q_head - this->q_tail);
	}	
}

///

static void _cond_cmd_q_put_internal(struct cond_cmd_q * this, int cmd_id, void * arg)
{
	struct cmd new_cmd = {.id = cmd_id, .arg = arg};

	// Insert item into a buffer
	this->q[this->q_head] = new_cmd;
	if ((this->q_head + 1) == this->q_size) this->q_head = 0;
	else this->q_head += 1;

	// Send 'not empty' event
	pthread_cond_signal(&this->q_not_empty);
}

///

bool cond_cmd_q_put(struct cond_cmd_q * this, int cmd_id, void * arg)
{
	int err;

	assert(this != NULL);
	assert(this->q_head < this->q_size);
	assert(this->q_tail < this->q_size);


	// Lock and conditionally wait for 'not full' event
	err = pthread_mutex_lock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_put pthread_mutex_lock failed");
		return false;
	}


	while (cond_cmd_q_full(this))
	{
		err = pthread_cond_wait(&this->q_not_full, &this->q_mtx);
		if (err)
		{
			LOG_ERROR("cond_cmd_q_put pthread_cond_wait failed");
			pthread_mutex_unlock(&this->q_mtx);
			return false;
		}
	}


	// Insert item into buffer
	_cond_cmd_q_put_internal(this, cmd_id, arg);


	// Unlock buffer
	err = pthread_mutex_unlock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_put pthread_mutex_unlock failed");
		return false;
	}


	return true;
}

///

int cond_cmd_q_timed_put(struct cond_cmd_q * this, int cmd_id, void * arg, unsigned int timeout_ms)
{
	struct timespec ts = {0,0};
	int err;

	assert(this != NULL);
	assert(this->q_head < this->q_size);
	assert(this->q_tail < this->q_size);


	// Lock and conditionally wait for 'not full' event
	err = pthread_mutex_lock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_timed_put pthread_mutex_lock failed");
		return -1;
	}


	while (cond_cmd_q_full(this))
	{
		// When iterating for first time, compute absolute time for timeout
		if (ts.tv_sec == 0) _cond_cmd_q_getabstimeout(&ts, timeout_ms);

		err = pthread_cond_timedwait(&this->q_not_full, &this->q_mtx, &ts);
		if (err != 0)
		{
			int ret = (err == ETIMEDOUT) ? 0 : -1;
			
			// Unlock buffer and return due to timeout condition
			err = pthread_mutex_unlock(&this->q_mtx);
			if (err)
			{
				LOG_ERROR("cond_cmd_q_timed_put pthread_mutex_unlock failed");
				return -1;
			}
			
			return ret;
		}
	}


	// Insert item into buffer
	_cond_cmd_q_put_internal(this, cmd_id, arg);


	// Unlock buffer
	err = pthread_mutex_unlock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_timed_put pthread_mutex_unlock failed");
		return -1;
	}

	return 1;
}

///

static void _cond_cmd_q_get_internal(struct cond_cmd_q * this, struct cmd * cmd_out)
{
	// Get item out of a buffer
	*cmd_out = this->q[this->q_tail];
	if ((this->q_tail + 1) == this->q_size) this->q_tail = 0;
	else this->q_tail += 1;

	// Send 'not full' event
	pthread_cond_signal(&this->q_not_full);
}

///

bool cond_cmd_q_get(struct cond_cmd_q * this, struct cmd * cmd_out)
{
	int err;

	assert(this != NULL);
	assert(this->q_head < this->q_size);
	assert(this->q_tail < this->q_size);


	// Lock and conditionally wait for 'not empty' event
	err = pthread_mutex_lock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_get pthread_mutex_lock failed");
		return false;
	}


	while (cond_cmd_q_empty(this))
	{
		err = pthread_cond_wait(&this->q_not_empty, &this->q_mtx);
		if (err)
		{
			LOG_ERROR("cond_cmd_q_get pthread_cond_wait failed");
			err = pthread_mutex_unlock(&this->q_mtx);
			if (err) LOG_ERROR("cond_cmd_q_get pthread_mutex_unlock failed");
			return false;
		}

	}


	// Extract item from buffer
	_cond_cmd_q_get_internal(this,cmd_out);


	// Unlock buffer
	err = pthread_mutex_unlock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_get pthread_mutex_unlock");
		return false;
	}

	return true;
}

///

int cond_cmd_q_timed_get(struct cond_cmd_q * this, struct cmd * cmd_out, unsigned int timeout_ms)
{
	struct timespec ts = {0, 0};
	int err;

	assert(this != NULL);
	assert(this->q_head < this->q_size);
	assert(this->q_tail < this->q_size);

	
	// Lock and conditionally wait for 'not empty' event
	err = pthread_mutex_lock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_timed_get pthread_mutex_lock failed");
		return -1;
	}


	while (cond_cmd_q_empty(this))
	{
		// When iterating for first time, compute absolute time for timeout
		if (ts.tv_sec == 0) _cond_cmd_q_getabstimeout(&ts, timeout_ms);
	
		err = pthread_cond_timedwait(&this->q_not_empty, &this->q_mtx, &ts);
		if (err)
		{
			int ret = (err == ETIMEDOUT) ? 0 : -1;
			
			// Unlock buffer and return due to timeout condition
			err = pthread_mutex_unlock(&this->q_mtx);
			if (err)
			{
				LOG_ERROR("cond_cmd_q_timed_get pthread_mutex_unlock failed");
				return -1;	
			}
			
			return ret;
		}

	}


	// Extract item from buffer
	_cond_cmd_q_get_internal(this,cmd_out);


	// Unlock buffer
	err = pthread_mutex_unlock(&this->q_mtx);
	if (err)
	{
		LOG_ERROR("cond_cmd_q_timed_get pthread_mutex_unlock failed");
		return -1;
	}

	return 1;
}

///

static void _cond_cmd_q_getabstimeout(struct timespec * ts, unsigned int timeout_ms)
{
	struct timeval now;

	// Prepare absolute time for timeout
	gettimeofday(&now, NULL);
	ts->tv_sec = now.tv_sec;
	ts->tv_nsec = now.tv_usec*1000;

	ts->tv_sec += timeout_ms / 1000;
	ts->tv_nsec += timeout_ms % 1000 * 1000000;

	if (ts->tv_nsec > 999999999)
	{
		ts->tv_sec++;
		ts->tv_nsec = ts->tv_nsec % 1000000000;
	}
}

