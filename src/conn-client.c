#include "conn-client.h"

void *handle_conn_states(void *arg) {
    client_ctx_t *conn_ctx = (client_ctx_t *) arg;
    if (conn_ctx == NULL)
        return NULL;

    switch (conn_ctx->sm.current_state) {
        case CONN_STATE_ACCEPTED:

            break;
        case CONN_STATE_REQ_PARSING:
            break;
        case CONN_STATE_RESP_CONSTRUCTING:
            break;
        case CONN_STATE_RESP_SENDING:
            break;
        case CONN_STATE_CLOSED:
            break;
        case CONN_STATE_ERROR:
            break;
        default:
            break;
    }
//    state_transit(&conn_ctx->sm);
    return NULL;
}