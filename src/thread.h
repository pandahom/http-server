#ifndef THREAD_H
#define THREAD_H
#include <stdlib.h>
#include <arpa/inet.h>

#define MAX_WORKER_NUM  15

typedef struct worker_s worker_t;
typedef struct thread_pool_s thread_pool_t;

typedef struct {
    int fd;
    struct sockaddr_storage address;
} conn_job_arg_t;


thread_pool_t *thread_pool_create(size_t worker_num);
int thread_pool_start(thread_pool_t *pool);
int thread_pool_add_new_connection(thread_pool_t *pool, void *data);
void thread_pool_destroy(thread_pool_t *pool);
#endif
