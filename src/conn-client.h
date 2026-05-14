#ifndef CONN_CLIENT_H
#define CONN_CLIENT_H
#include "connection-sm.h"

typedef struct {
    struct sockaddr_storage address;
    int fd;
    char received_msg[MAX_RECEIVE_BYTES];

    struct {
        char ip[MAX_ADDR_LEN];
        uint16_t port;
    } readable_format;

    conn_sm_t sm;
} client_ctx_t;

void receive_msg(client_ctx_t *conn);
void send_msg(client_ctx_t *conn);
void destroy_connection(client_ctx_t *conn);

#endif