#ifndef SERVER_SM_H
#define SERVER_SM_H

typedef enum {
    SRV_STATE_ERROR,
    SRV_STATE_INIT,
    SRV_STATE_LISTENING,
    SRV_STATE_ACCEPTED,
    SRV_STATE_COUNT
} srv_state_e;

typedef enum {
    SRV_EVENT_ERROR,
    SRV_EVENT_CONSTRUCTED,
    SRV_EVENT_CONNECTION_RECEIVED,
    SRV_EVENT_RESET,
    SRV_EVENT_COUNT
} srv_event_e;

typedef struct {
    srv_state_e current_state;
    srv_event_e event_trigger;
} srv_sm_t;

int handle_srv_states(const char *ip_address, int port);
#endif