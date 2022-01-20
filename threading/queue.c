#include <pthread.h>
#include <stdlib.h>

#include "queue.h"

// a complete guess, no benchmarks yet, but, given the realistic load the server
// is going to experience, anything greater than 1 is *fine*.
#define INIT_QUEUE_LEN 8

struct queue {
	pthread_mutex_t mutex;
	pthread_cond_t queued;

	void **elems;
	unsigned int cap;
	unsigned int len;
};

struct queue *
queue_new(void)
{
	struct queue *q = malloc(sizeof(struct queue));
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->queued, NULL);
	q->elems = calloc(sizeof(void *), INIT_QUEUE_LEN);
	q->cap = INIT_QUEUE_LEN;
	q->len = 0;
	return q;
}

void
queue_free(struct queue *q)
{
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->queued);
	free(q->elems);
	free(q);
}

void
queue_enqueue(struct queue *q, void *e)
{
	pthread_mutex_lock(&q->mutex);

	if (q->len >= q->cap) {
		q->cap *= 2;
		q->elems = realloc(q, q->cap);
	}

	q->elems[q->len++] = e;

	pthread_cond_signal(&q->queued);
	pthread_mutex_unlock(&q->mutex);
}

void *
queue_dequeue(struct queue *q)
{
	void *ret;

	pthread_mutex_lock(&q->mutex);
	while (q->len == 0) pthread_cond_wait(&q->queued, &q->mutex);

	ret = q->elems[--q->len];

	pthread_mutex_unlock(&q->mutex);
	return ret;
}
