#include "server.h"
#include "log.h"
#include <pthread.h>

#define state_transit(sm_ptr)\
    (sm_ptr)->current_state = transition_table_server[(sm_ptr)->current_state][(sm_ptr)->event_trigger]

static const srv_state_e transition_table_server[SRV_STATE_COUNT][SRV_EVENT_COUNT] = {
    [SRV_STATE_INIT] = {
        [SRV_EVENT_CONSTRUCTED] = SRV_STATE_LISTENING
    },

    [SRV_STATE_LISTENING] = {
        [SRV_EVENT_RESET] = SRV_STATE_LISTENING,
        [SRV_EVENT_CONNECTION_RECEIVED] = SRV_STATE_ACCEPTED,
        [SRV_EVENT_SHUTDOWN] = SRV_STATE_SHUTTING_DOWN,
    },
    [SRV_STATE_ACCEPTED] = {
            [SRV_EVENT_RESET] = SRV_STATE_LISTENING,
    }
};

static void on_error_or_shutdown_server(server_ctx_t *server, srv_state_e state) {
    close(server->fd);
    if (state == SRV_STATE_ERROR)
        LOG_ERROR("Terminating Program (Waiting for threads to be finished)");
    else if (state == SRV_STATE_SHUTTING_DOWN)
        LOG_INFO("Shutdown requested, waiting for in-flight requests to finish");

    thread_pool_destroy(server->th_pool);
    client_ctx_free(server->opaque);
}

int handle_srv_states(const char *ip_address, int port) {
    int ret = OK;
    server_ctx_t server   = {
            .sm = { .current_state = SRV_STATE_INIT},
    };
    srv_sm_t *server_sm = &server.sm;
    conn_job_arg_t arg = {0};

    while (true) {
        switch (server_sm->current_state) {
            case SRV_STATE_INIT:
                construct_server(&server, port, ip_address, SOMAXCONN);
                break;
            case SRV_STATE_LISTENING:
                accept_connection(&arg, &server);
                break;
            case SRV_STATE_ACCEPTED:
                add_client_to_waiting_list(&arg, &server);
                break;
            case SRV_STATE_ERROR:
                ret = FAIL;
                on_error_or_shutdown_server(&server, server_sm->current_state);
                goto done;
            case SRV_STATE_SHUTTING_DOWN:
                ret =  OK;
                on_error_or_shutdown_server(&server, server_sm->current_state);;
                goto done;
            default:
                break;
        }
        state_transit(server_sm);
    }
done:
    return ret;
}
