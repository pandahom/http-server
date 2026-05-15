#ifndef CONN_CLIENT_H
#define CONN_CLIENT_H
#include "connection-sm.h"
#include "request-handler.h"

typedef struct {
    struct sockaddr_storage address;
    int fd;
    char received_msg[MAX_RECEIVE_BYTES];
    ssize_t received_bytes;

    struct {
        char ip[MAX_ADDR_LEN];
        uint16_t port;
    } readable_format;

    conn_sm_t sm;
    http_parser_t parser;
} client_ctx_t;

void receive_msg(client_ctx_t *conn);
void parse_request(client_ctx_t *conn);
void send_msg(client_ctx_t *conn, char *msg);
void destroy_connection(client_ctx_t *conn);

#endif