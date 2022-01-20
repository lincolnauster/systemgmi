/* A thread-safe multi-producer single-consumer queue. The implementation of
 * this is pretty lazy - we're just sticking a queue in a mutex and adding some
 * blocking logic when it's empty. */

struct queue;

struct queue *queue_new(void);
void queue_free(struct queue *);
void queue_enqueue(struct queue *, void *);

/* Dequeue something from the queue. Blocks if nothing is available at the
 * moment, and returns when something is added. */
void *queue_dequeue(struct queue *);
