#include "connection-sm.h"
const conn_state_e transition_table_cn[CONN_STATE_COUNT][CONN_EVENT_COUNT] = {
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