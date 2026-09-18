#include "server.h"
#include "common.h"
#include "thread.h"
#include <errno.h>
#include <pthread.h>

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
        goto on_error;
    }

    rv = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &reuse,  (socklen_t)sizeof(reuse));
    if (rv == -1) {
        ERR_LOG("setsockopt()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        goto on_error;
    }

    rv = bind(server->fd, (struct sockaddr *)&server->address, sizeof(server->address));
    if (rv == -1) {
        ERR_LOG("bind()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        goto on_error;
    }

    rv = listen(server->fd, server->backlog);
    if (rv == -1) {
        ERR_LOG("listen()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        goto on_error;
    }

    server->opaque = client_ctx_alloc();
    if (server->opaque == NULL) {
        ERR_LOG("calloc()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        goto on_error;
    }

    server->th_pool = thread_pool_create(MAX_WORKER_NUM);
    if (!server->th_pool){
        ERR_LOG("thread_pool_create()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        goto on_error;
    }

    rv = thread_pool_start(server->th_pool);
    if (rv != OK) {
        ERR_LOG("thread_pool_create()");
        server->sm.event_trigger = SRV_EVENT_ERROR;
        goto on_error;
    }

    printf("Server listening on %s:%u ....\n", ip_address, port);
    return;

on_error:
    ERR_LOG("Could not construct server");
}

void accept_connection(conn_job_arg_t *arg, server_ctx_t *server) {
    char ip_buf[MAX_ADDR_LEN];
    uint16_t port; 
    socklen_t client_addr_len = sizeof(arg->address);

    server->sm.event_trigger = SRV_EVENT_CONNECTION_RECEIVED;

    arg->fd = accept(server->fd, (struct sockaddr *) &arg->address, &client_addr_len);

    if (arg->fd == -1) {
        ERR_LOG("accept()");
        if (errno == EINVAL)
            server->sm.event_trigger = SRV_EVENT_ERROR; 
        else
            server->sm.event_trigger = SRV_EVENT_RESET; 
        return;
    }

    if (inet_ntop(arg->address.ss_family, get_ip(&arg->address), ip_buf, MAX_ADDR_LEN) == NULL) {
        ERR_LOG("inet_ntop()");
    }

    port = ntohs(get_port(&arg->address));

    printf("Received connection from %s:%u\n", ip_buf, port);
}

void add_client_to_waiting_list(conn_job_arg_t *arg, server_ctx_t *server) {
    int rv;
    rv = thread_pool_add_new_connection(server->th_pool, arg);
    client_ctx_t *tmp_conn = NULL;

    if (rv != OK) {
        tmp_conn = server->opaque;
        tmp_conn->fd = arg->fd;
        send_service_unavailable(tmp_conn);
        destroy_connection(tmp_conn);
    }
    
    server->sm.event_trigger = SRV_EVENT_RESET;
}
