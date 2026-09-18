#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct queue_s queue_t;

queue_t *queue_create(size_t max_num, size_t elem_size);
int queue_push(queue_t *q, void *data, size_t elem_size);
int queue_pop(queue_t *q, void* out, size_t elem_size);
void queue_close(queue_t *q);
void queue_free(queue_t *q);
#endif
