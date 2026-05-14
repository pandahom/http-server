#include "conn-client.h"
#include <pthread.h>

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
            pthread_mutex_lock(&mutex);
            num_active_clients--;
            if (num_active_clients == 0)
                pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
            break;
        case CONN_STATE_ERROR:
            break;
        default:
            break;
    }
//    state_transit(&conn_ctx->sm);
    return NULL;
}