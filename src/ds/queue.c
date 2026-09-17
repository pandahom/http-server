#include "queue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#define QUEUE_FIND_TAIL(tail, queue_ptr) (tail) = ((queue_ptr)->head + (queue_ptr)->len) % (queue_ptr)->cap
#define QUEUE_REVALUATE_HEAD(queue_ptr) (queue_ptr)->head = ((queue_ptr)->head + 1) % (queue_ptr)->cap
#define QUEUE_IS_FULL(queue_ptr) ((queue_ptr)->len == (queue_ptr)->cap)

struct queue_s {
    void **items;
    int len;
    int cap;
    int head;
    bool closed;

    pthread_mutex_t lock;
    pthread_cond_t  cond;  // signaled on push, broadcast on close
};

queue_t *queue_create(size_t max_num) {
    queue_t *q = calloc(1, sizeof(queue_t));
    if (!q) return NULL;

    q->items = calloc(max_num, sizeof(void*));
    if (!q->items) {
        free(q);
        return NULL;
    }

    q->cap = max_num;
    q->len = 0;
    q->head = 0;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);

    return q;
}

int queue_push(queue_t *q, void *data) {
    pthread_mutex_lock(&q->lock);
    size_t tail = 0;

    if (q->closed || QUEUE_IS_FULL(q)) {
        pthread_mutex_unlock(&q->lock);
        return 1;
    }

    QUEUE_FIND_TAIL(tail, q);
    q->items[tail] = data; 
    q->len++;

    pthread_mutex_unlock(&q->lock);
    pthread_cond_signal(&q->cond);

    return 0;
}

void* queue_pop(queue_t *q) {
    void *popped = NULL;
    pthread_mutex_lock(&q->lock);
    
    while (q->len == 0 && !q->closed)
        pthread_cond_wait(&q->cond, &q->lock);

    if (q->len == 0) {
        // Queue is closed and empty so no more work
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    
    popped = q->items[q->head];

    QUEUE_REVALUATE_HEAD(q);
    q->len--;

    pthread_mutex_unlock(&q->lock);
    return popped;
}

void queue_close(queue_t *q) {
    pthread_mutex_lock(&q->lock);
    q->closed = true;
    pthread_mutex_unlock(&q->lock);
    pthread_cond_broadcast(&q->cond);
}

void queue_free(queue_t *q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->cond);
    free(q->items);
    free(q);
}

#undef QUEUE_FIND_TAIL
#undef QUEUE_REVALUATE_HEAD
#undef QUEUE_IS_FULL
