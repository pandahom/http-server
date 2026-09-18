#include "queue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_FIND_TAIL(tail, queue_ptr) (tail) = ((queue_ptr)->head + (queue_ptr)->len) % (queue_ptr)->cap
#define QUEUE_REVALUATE_HEAD(queue_ptr) (queue_ptr)->head = ((queue_ptr)->head + 1) % (queue_ptr)->cap
#define QUEUE_IS_FULL(queue_ptr) ((queue_ptr)->len == (queue_ptr)->cap)

struct queue_s {
    void *items;
    int len;
    int cap;
    int head;
    bool closed;

    pthread_mutex_t lock;
    pthread_cond_t  cond;  // signaled on push, broadcast on close
};

queue_t *queue_create(size_t max_num, size_t elem_size) {
    queue_t *q = calloc(1, sizeof(queue_t));
    if (!q) return NULL;

    q->items = calloc(max_num, elem_size);
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

int queue_push(queue_t *q, void *data, size_t elem_size) {
    pthread_mutex_lock(&q->lock);
    size_t tail = 0;

    if (q->closed || QUEUE_IS_FULL(q)) {
        pthread_mutex_unlock(&q->lock);
        return 1;
    }

    QUEUE_FIND_TAIL(tail, q);
    memcpy((char *)q->items + tail * elem_size, data, elem_size);
    q->len++;

    pthread_mutex_unlock(&q->lock);
    pthread_cond_signal(&q->cond);

    return 0;
}

int queue_pop(queue_t *q, void* out, size_t elem_size) {
    pthread_mutex_lock(&q->lock);
    
    while (q->len == 0 && !q->closed)
        pthread_cond_wait(&q->cond, &q->lock);

    if (q->len == 0) {
        // Queue is closed and empty so no more work
        pthread_mutex_unlock(&q->lock);
        return 1;
    }
    
    memcpy(out, (char *)q->items + q->head * elem_size, elem_size);
    QUEUE_REVALUATE_HEAD(q);
    q->len--;

    pthread_mutex_unlock(&q->lock);
    return 0;
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
