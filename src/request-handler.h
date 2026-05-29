#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "ds/ht.h"

#define METHOD_LEN           16
#define PATH_LEN             2048
#define VERSION_LEN          16
#define MAX_HEADERS_NUM      50
#define HEADER_VAL_LEN  1024
#define HEADER_NAME_LEN 256



typedef struct {
    char name[HEADER_NAME_LEN];
    char value[HEADER_VAL_LEN];
} http_header_t;

typedef struct {
    // Request Line
    char method[METHOD_LEN];
    char path[PATH_LEN];
    char version[VERSION_LEN];

    // Headers
    ht_t headers;

    char *body;
} http_request_t;

enum parse_e {
    MSG_STATE_REQUEST_LINE,
    MSG_STATE_LEADING_SPACE,
    MSG_STATE_HEADER_NAME,
    MSG_STATE_HEADER_VALUE,
    MSG_STATE_BODY,
    MSG_STATE_DONE,
    MSG_STATE_ERROR
};

typedef struct http_parser_s {
    enum parse_e state;
    http_request_t req;

    // temporary buffers while building current token
    char   token[1024];
    size_t token_len;

//    // for headers, we need to remember the name
//    // while we go parse the value
    char current_header_name[256];

//    size_t body_received;
} http_parser_t;

struct http_resp_s;

int validate_http_version(char *version);
void handle_unsupported_version(struct http_resp_s **resp);
void handle_unsupported_method(struct http_resp_s **resp);
int handle_get_req(http_request_t *req);
int request_state_handler(http_parser_t  *parser, const char *raw_msg, size_t len);
#endif