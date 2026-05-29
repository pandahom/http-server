#include "conn-client.h"
#include <pthread.h>

void receive_msg(client_ctx_t *conn_ctx) {
    ssize_t received_bytes = 0;

    conn_ctx->sm.event_trigger = CONN_EVENT_REQ_RECEIVED;

    received_bytes = recv(conn_ctx->fd, conn_ctx->received_msg, MAX_RECEIVE_BYTES, 0);
    if (received_bytes == -1) {
        ERR_LOG("recv() returns -1");
        conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
    }
    conn_ctx->received_bytes = received_bytes;
}

void parse_request(client_ctx_t *conn) {
    int rv = request_state_handler(&conn->parser, conn->received_msg, conn->received_bytes);
    if (rv == 0) {
        conn->sm.event_trigger = CONN_EVENT_REQ_PARSED;
    } else
        conn->sm.event_trigger = CONN_EVENT_ERROR;
}


void send_msg(client_ctx_t *conn_ctx, char *msg) {
    (void)msg;
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

    ht_destroy(&conn_ctx->parser->req.headers);
    close(conn_ctx->fd);
    free(conn_ctx);
}