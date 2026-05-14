#include "conn-client.h"
#include <pthread.h>

void receive_msg(client_ctx_t *conn_ctx) {
    ssize_t received_bytes = 0;
    conn_ctx->sm.event_trigger = CONN_EVENT_REQ_RECEIVED;
    received_bytes = recv(conn_ctx->fd, conn_ctx->received_msg, MAX_RECEIVE_BYTES, 0);
    if (received_bytes == -1) {
        ERR_LOG("recv() returns -1");
        conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
        close(conn_ctx->fd);
    }
    printf("%s\n", conn_ctx->received_msg);
    fflush(stdout);
}

void send_msg(client_ctx_t *conn_ctx) {
    char buf[12] = "Hello World";
    send(conn_ctx->fd, buf, sizeof buf, 0);
    conn_ctx->sm.event_trigger = CONN_EVENT_RESP_SENT;
    fflush(stdout);
}

void destroy_connection(client_ctx_t *conn_ctx) {
    pthread_mutex_lock(&mutex);
    num_active_clients--;
    if (num_active_clients == 0)
        pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    close(conn_ctx->fd);
    free(conn_ctx);
}