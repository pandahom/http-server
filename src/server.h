#ifndef SERVER_H
#define SERVER_H

#include "server-sm.h"
#include "conn-client.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "thread.h"

typedef struct {
    struct sockaddr_storage address;
    int fd;
    int backlog;

    thread_pool_t *th_pool;
    srv_sm_t sm;
} server_ctx_t;

void construct_server(server_ctx_t  *server, in_port_t port, const char *ip_address, int backlog);
client_ctx_t *accept_connection(server_ctx_t *server);
void add_client_to_waiting_list(client_ctx_t *new_con, server_ctx_t *server);

#endif
