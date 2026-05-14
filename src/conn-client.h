#ifndef CONN_CLIENT_H
#define CONN_CLIENT_H
#include "connection-sm.h"

extern uint16_t num_active_clients;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;

typedef struct {
    struct sockaddr_storage address;
    int fd;
    char received_msg[MAX_RECEIVE_BYTES];

    struct {
        char ip[MAX_ADDR_LEN];
        uint16_t port;
    } readable_format;

    conn_sm_t sm;
} client_ctx_t;

void receive_msg(client_ctx_t *client);
void *handle_conn_states(void *arg);

// TODO
//#define state_transit( sm_ptr) \
//    (sm_ptr)->current_state = transition_table[(sm_ptr)->current_state][(sm_ptr)->event_trigger]

#endif