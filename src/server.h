#ifndef SERVER_H
#define SERVER_H

#include "server-sm.h"
#include "conn-client.h"
#include <arpa/inet.h>
#include <sys/socket.h>

typedef struct {
    struct sockaddr_storage address;
    int fd;
    int backlog;

    srv_sm_t sm;
} server_ctx_t;

void construct_server(server_ctx_t  *server, in_port_t port, char *ip_address, int backlog);
void *get_ip(struct sockaddr_storage *addr);
uint16_t get_port(struct sockaddr_storage *addr);
client_ctx_t *accept_connection(server_ctx_t *server);
void init_client_connection_thread(client_ctx_t *new_con, server_ctx_t *server);

#endif