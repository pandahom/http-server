#include "server.h"
#include <pthread.h>

const srv_state_e transition_table[SRV_STATE_COUNT][SRV_EVENT_COUNT] = {
    [SRV_STATE_INIT] = {
        [SRV_EVENT_CONSTRUCTED] = SRV_STATE_LISTENING
    },

    [SRV_STATE_LISTENING] = {
        [SRV_EVENT_CONNECTION_RECEIVED] = SRV_STATE_ACCEPTED,
    },
    [SRV_STATE_ACCEPTED] = {
            [SRV_EVENT_RESET] = SRV_STATE_LISTENING,
    }
};

static int on_error_server(server_ctx_t *server) {
    close(server->fd);
    printf("Terminating Program (Waiting for threads to be finished) ....\n");

    pthread_mutex_lock(&mutex);

    while (num_active_clients > 0)
        pthread_cond_wait(&cond, &mutex);

    pthread_mutex_unlock(&mutex);

    return FAIL;
}

int handle_srv_states(void) {
    int ret = OK;
    server_ctx_t server   = {
            .sm = { .current_state = SRV_STATE_INIT},
    };
    srv_sm_t *server_sm = &server.sm;
    client_ctx_t *new_con  = NULL;


    char *ip_address = "127.0.0.4";
    uint16_t port    = 8080;

    while (true) {
        switch (server_sm->current_state) {
            case SRV_STATE_INIT:
                construct_server(&server, port, ip_address, 5);
                break;
            case SRV_STATE_LISTENING:
                new_con = accept_connection(&server);
                break;
            case SRV_STATE_ACCEPTED:
                init_client_connection_thread(new_con, &server);
                break;
            case SRV_STATE_ERROR:
                ret = on_error_server(&server);
                goto done;
            default:
                break;
        }
        state_transit(server_sm);
    }
done:
    return ret;
}