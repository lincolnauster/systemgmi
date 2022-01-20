/* Concurrent serving interfaces.
 *
 * Functions here wrap POSIX threads to provide a 'good enough' threading API
 * that keeps client code clean and introduces only a limited amount of
 * complexity.
 *
 * On threader_init(), worker threads are spawned (one for each available CPU).
 * threader_queue() may be used to queue a function for execution on those
 * threads. threader_close() will block until threads have finished all
 * previously assigned work, then close them, free all resources related to
 * them, and return. */

struct work {
	void (*routine)(void *);
	void (*cleanup)(struct work *);
	void *args;
};

/* Initialize the threadpool. */
void threader_init(void);

/* Tell threads to stop their work and terminated. This routine blocks until
 * all threads are done. */
void threader_close(void);

void threader_queue(struct work *);
