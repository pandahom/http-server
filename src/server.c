#include "server.h"
#include <pthread.h>

uint16_t num_active_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static struct sockaddr_storage populate_server_address(in_port_t port, const char *ip_address);
static uint16_t get_port(struct sockaddr_storage *addr);
static void *get_ip(struct sockaddr_storage *addr);


static uint16_t get_port(struct sockaddr_storage *addr) {
    return
            addr->ss_family == AF_INET               ?
            ((struct sockaddr_in *)  addr)->sin_port :
            ((struct sockaddr_in6 *) addr)->sin6_port;
}

static void *get_ip(struct sockaddr_storage *addr) {
    return
            addr->ss_family == AF_INET                          ?
            (void *) &((struct sockaddr_in  *)addr)->sin_addr   :
            (void *) &((struct sockaddr_in6 *)addr)->sin6_addr  ;
}

static struct sockaddr_storage populate_server_address(in_port_t port, const char *ip_address) {
    struct sockaddr_storage address = {0};
    struct sockaddr_in      *v4     = (struct sockaddr_in *) &address;
    struct sockaddr_in6     *v6     = (struct sockaddr_in6 *) &address;

    if (inet_pton(AF_INET, ip_address, &v4->sin_addr.s_addr) == 1) {

        v4->sin_port = htons(port);
        v4->sin_family = AF_INET;

    } else if (inet_pton(AF_INET6, ip_address, &v6->sin6_addr) == 1) {

        v6->sin6_port = htons(port);
        v6->sin6_family = AF_INET6;
    } else {
        ERR_LOG("RIDI");
        exit(FAIL);
    }

    return address;
}

void construct_server(server_ctx_t  *server, in_port_t port, const char *ip_address, int backlog) {
    int rv                            = OK;
    int reuse                           = 1;

    server->address = populate_server_address(port, ip_address);
    server->backlog = backlog;

    server->sm.event_trigger = SRV_EVENT_CONSTRUCTED;


    server->fd = socket(server->address.ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (server->fd == -1) {
        ERR_LOG("socket()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
    }

    rv = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &reuse,  (socklen_t)sizeof(reuse));
    if (rv == -1) {
        ERR_LOG("setsockopt()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
    }

    rv = bind(server->fd, (struct sockaddr *)&server->address, sizeof(server->address));
    if (rv == -1) {
        ERR_LOG("bind()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
    }

    rv = listen(server->fd, server->backlog);
    if (rv == -1) {
        ERR_LOG("listen()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
    }

    printf("Server listening on %s:%u ....\n", ip_address, port);
}

client_ctx_t *accept_connection(server_ctx_t *server) {
    client_ctx_t *conn_ctx = (client_ctx_t *) calloc(1, sizeof(client_ctx_t));


    if (!conn_ctx) {
        server->sm.event_trigger = SRV_EVENT_ERROR;
        return NULL;
    }

    socklen_t client_addr_len = sizeof(conn_ctx->address);

    server->sm.event_trigger = SRV_EVENT_CONNECTION_RECEIVED;

    conn_ctx->fd = accept(server->fd, (struct sockaddr *) &conn_ctx->address, &client_addr_len);

    if (conn_ctx->fd == -1) {
        ERR_LOG("accept()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        free(conn_ctx);
        return NULL;
    }

    if (inet_ntop(conn_ctx->address.ss_family,
                  get_ip(&conn_ctx->address),
                  conn_ctx->readable_format.ip, MAX_ADDR_LEN) == NULL) {
        ERR_LOG("inet_ntop()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        free(conn_ctx);
        return NULL;
    }

    conn_ctx->readable_format.port = ntohs(get_port(&conn_ctx->address));

    printf("Received connection from %s:%u\n", conn_ctx->readable_format.ip, conn_ctx->readable_format.port);

    return conn_ctx;
}

void init_client_connection_thread(client_ctx_t *new_con, server_ctx_t *server) {
    int rv = OK;
    pthread_t new_th;

    new_con->sm.current_state = CONN_STATE_ACCEPTED;
    rv = pthread_create(&new_th, NULL, handle_conn_states, new_con);

    if (rv != OK) {
        close(new_con->fd);
        free(new_con);
        ERR_LOG("Could not create a new thread.");
        return;
    }

    rv = pthread_detach(new_th);
    if (rv != OK) {
        close(new_con->fd);
        free(new_con);
        ERR_LOG("Could not set detach on new thread.");
        return;
    }

    pthread_mutex_lock(&mutex);
    num_active_clients++;
    pthread_mutex_unlock(&mutex);

    server->sm.event_trigger = SRV_EVENT_RESET;
}