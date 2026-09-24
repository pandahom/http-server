#include "thread.h"
#include "conn-client.h"
#include "ds/queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "log.h"

#define MAX_ACCEPTING_CONN 1024

typedef struct client_ctx_s client_ctx_t;

typedef struct thread_pool_s {
    worker_t *workers;
    
    size_t worker_num;
    size_t running_workers_num;
    queue_t *queue;

} thread_pool_t;

struct worker_s {
    pthread_t tid;
    thread_pool_t *pool; // pointer back to its pool
    void *data;
};

static void *worker_run(void* arg);

thread_pool_t *thread_pool_create(size_t worker_num) {
    thread_pool_t *pool = calloc(1, sizeof(thread_pool_t));
    if (!pool)
        return NULL;

    pool->workers = calloc(worker_num, sizeof(worker_t));
    if (!pool->workers) {
        free(pool);
        return NULL;
    }
    pool->worker_num = worker_num;
    pool->queue = queue_create(MAX_ACCEPTING_CONN, sizeof(conn_job_arg_t));
    if (!pool->queue) {
        free(pool->workers);
        free(pool);
        return NULL;
    }

    return pool;
}

int thread_pool_start(thread_pool_t *pool) {
    worker_t *worker = NULL;
    int rv = 0;

    for (size_t i = 0; i < pool->worker_num; ++i) {
        worker = &pool->workers[i];
        worker->pool = pool;
        worker->data = (void*) client_ctx_alloc();
        if (worker->data == NULL) {
            break;
        }

        rv = pthread_create(&worker->tid, NULL, worker_run, worker);
        if (rv != 0)  {
            LOG_ERROR("Could not create worker [%zu]", i + 1);
            break;
        }
        pool->running_workers_num = i + 1;
    }

    if (pool->worker_num != pool->running_workers_num) {
        return FAIL; 
    }
    return OK;
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) 
        return;

    worker_t *worker = NULL;

    queue_close(pool->queue);

    for (size_t i = 0; i < pool->running_workers_num ; ++i) {
        worker = &pool->workers[i];
        pthread_join(worker->tid, NULL);
        client_ctx_free(worker->data);
    }

    queue_free(pool->queue);
    free(pool->workers);
    free(pool);
}

int thread_pool_add_new_connection(thread_pool_t *pool, void *data) {
    return queue_push(pool->queue, data, sizeof(conn_job_arg_t));
}

static void *worker_run(void* arg) {
    worker_t *w = arg;
    client_ctx_t *conn = w->data;
    conn_job_arg_t out = {0};
    int rv = OK;

    while (queue_pop(w->pool->queue, &out, sizeof(conn_job_arg_t)) == OK) {
        client_ctx_assign_fd_and_address(conn, out.fd, &out.address);
        rv = handle_conn_states(conn);
        if (rv == FAIL) {

        }
        client_ctx_reset(conn);
    }
    
    return NULL;
}
