#include "conn-client.h"
#include <pthread.h>


#define state_transit( sm_ptr) \
    (sm_ptr)->current_state = transition_table_client[(sm_ptr)->current_state][(sm_ptr)->event_trigger]

static const conn_state_e transition_table_client[CONN_STATE_COUNT][CONN_EVENT_COUNT] = {
        [CONN_STATE_ACCEPTED] = {
                [CONN_EVENT_REQ_RECEIVED] = CONN_STATE_REQ_PARSING
        },

        [CONN_STATE_REQ_PARSING] = {
                [CONN_EVENT_REQ_PARSED] = CONN_STATE_RESP_CONSTRUCTING,
        },

        [CONN_STATE_RESP_CONSTRUCTING] = {
                [CONN_EVENT_RESP_BUILT] = CONN_STATE_RESP_SENDING,
        },

        [CONN_STATE_RESP_SENDING] = {
                [CONN_EVENT_RESP_SENT] = CONN_STATE_CLOSED
        }
};

void *handle_conn_states(void *arg) {
    client_ctx_t *conn_ctx = (client_ctx_t *) arg;
    if (conn_ctx == NULL)
        return NULL;

    while (true) {
        switch (conn_ctx->sm.current_state) {
            case CONN_STATE_ACCEPTED:
                receive_msg(conn_ctx);
                break;
            case CONN_STATE_REQ_PARSING:
                parse_request(conn_ctx);
                break;
            case CONN_STATE_RESP_CONSTRUCTING:
                process_request(conn_ctx);
                break;
            case CONN_STATE_RESP_SENDING:
                send_msg(conn_ctx);
                break;
            case CONN_STATE_CLOSED:
                destroy_connection(conn_ctx);
                goto return_val;
            case CONN_STATE_ERROR:
                ERR_LOG("Encounter error on handling connection");
                destroy_connection(conn_ctx);
                goto return_val;
            default:
                break;
        }
        state_transit(&conn_ctx->sm);
    }
return_val:
    return NULL;
}
