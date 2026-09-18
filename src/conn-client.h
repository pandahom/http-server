#ifndef CONN_CLIENT_H
#define CONN_CLIENT_H
#include "connection-sm.h"

struct http_parser_s;
struct http_resp_s;

typedef struct client_ctx_s client_ctx_t;
struct client_ctx_s {
    struct sockaddr_storage address;
    int fd;
    char received_msg[MAX_RECEIVE_BYTES];
    ssize_t received_bytes;

    conn_sm_t sm;
    struct http_parser_s *parser;
    struct http_resp_s *response;
};

void receive_msg(client_ctx_t *conn);
void parse_request(client_ctx_t *conn);
int process_request(client_ctx_t *conn);
void send_msg(client_ctx_t *conn);
void send_service_unavailable(client_ctx_t *conn);
void release_connection_resources(client_ctx_t *conn_ctx);
client_ctx_t *client_ctx_alloc(void);
void client_ctx_free(client_ctx_t *conn);
void client_ctx_assign_fd_and_address(client_ctx_t *conn, int fd, struct sockaddr_storage *address);
void client_ctx_reset(client_ctx_t *conn);
#endif
