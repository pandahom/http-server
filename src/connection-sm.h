#ifndef REQ_HANDLER_H
#define REQ_HANDLER_H

#include "common.h"
typedef enum {
    CONN_STATE_ERROR,
    CONN_STATE_ACCEPTED,
    CONN_STATE_REQ_PARSING,
    CONN_STATE_RESP_CONSTRUCTING,
    CONN_STATE_RESP_SENDING,
    CONN_STATE_CLOSED,
    CONN_STATE_COUNT
} conn_state_e;

typedef enum {
    CONN_EVENT_ERROR,
    CONN_EVENT_REQ_RECEIVED,
    CONN_EVENT_REQ_PARSED,
    CONN_EVENT_RESP_BUILT,
    CONN_EVENT_RESP_SENT,
    CONN_EVENT_COUNT
} conn_event_e;

typedef struct {
    conn_state_e current_state;
    conn_event_e event_trigger;
} conn_sm_t;

extern const conn_state_e transition_table_cn[CONN_STATE_COUNT][CONN_EVENT_COUNT];

#endif