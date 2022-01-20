#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

#include "../log/log.h"
#include "threading.h"
#include "queue.h"

#define MAX_WORKER_THREADS 32

struct worker_thread {
	struct queue *queued;
	pthread_t thread;
};

static struct worker_thread thread_pool[MAX_WORKER_THREADS];
static long thread_pool_size;
_Atomic unsigned int sched_idx = 0;

static void make_thread(int);
static void free_thread(int);

static void run_thread(struct worker_thread *);

static long num_cpus(void);

void
threader_init(void)
{
	thread_pool_size = num_cpus();

	for (int i = 0; i < thread_pool_size; i++)
		make_thread(i);

	log_debug("Threads launched.");
}

void
threader_close(void)
{
	for (int i = 0; i < thread_pool_size; i++)
		free_thread(i);
}

void
threader_queue(struct work *work)
{
	// This is our scheduler. It's a very simple round-robin-ish scheduler:
	// the first time this is called, we assign to thread 0, then thread 1,
	// then 2, until looping back around. This isn't necessarily optimal,
	// but, to be fair, this whole arrangement is slightly overkill for a
	// gemini server.
	//
	// Note that we count on unsigned integer overflow when serving large
	// numbers of requests.
	struct worker_thread *wt = &thread_pool[
		atomic_fetch_add(&sched_idx, 1) % thread_pool_size
	];

	queue_enqueue(wt->queued, work);
}

/* Initialize thread_pool[idx]. The caller is responsible for verifying idx is
 * in bounds. */
static void
make_thread(int idx)
{
	struct worker_thread *wt = &thread_pool[idx];
	wt->queued = queue_new();
	pthread_create(&wt->thread, NULL, (void *(*)(void *)) run_thread, wt);
}

/* Clean up all the resources associated with thrad_pool[idx]. The caller is
 * responsible for verifying idx is in bounds. */
static void
free_thread(int idx)
{
	struct worker_thread *wt = &thread_pool[idx];
	queue_enqueue(wt->queued, NULL);
	pthread_join(wt->thread, NULL);
	queue_free(wt->queued);
}

/* While non-NULL work is available in the queue, perform it. When NULL is
 * received, shut down cleanly. */
static void
run_thread(struct worker_thread *self)
{
	struct work *w;
	while ((w = (struct work *) queue_dequeue(self->queued))) {
		(w->routine)(w->args);
		(w->cleanup)(w);
	}
}

static long
num_cpus(void)
{
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	int ncpus_i = ncpus > MAX_WORKER_THREADS
	            ? MAX_WORKER_THREADS
	            : (int) ncpus;

        char log_msg[] = "Thread pool will use .. CPUs.";
        sprintf(log_msg, "Thread pool will use %d CPUs.", ncpus_i);

        log_debug(log_msg);

	return ncpus_i;
}
