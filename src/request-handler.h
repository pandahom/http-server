#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "ds/ht.h"

#define METHOD_LEN           16
#define PATH_LEN             2048
#define VERSION_LEN          16
#define MAX_HEADERS_NUM      50
#define HEADER_VAL_LEN  1024
#define HEADER_NAME_LEN 256

enum parse_e {
    MSG_STATE_REQUEST_LINE,
    MSG_STATE_LEADING_SPACE,
    MSG_STATE_HEADER_NAME,
    MSG_STATE_HEADER_VALUE,
    MSG_STATE_BODY,
    MSG_STATE_DONE,
    MSG_STATE_ERROR
};

typedef struct http_request_s { 
    // Request Line
    char method[METHOD_LEN];
    char path[PATH_LEN];
    char version[VERSION_LEN];

    // Headers
    ht_t headers;

    char *body;
} http_request_t;
 
typedef struct http_parser_s http_parser_t;

struct http_resp_s;

void set_document_root(const char *doc_root);
const char* get_document_root(void);
int validate_http_version(char *version);
void handle_unsupported_version(struct http_resp_s *resp);
void handle_unsupported_method(struct http_resp_s *resp, const char *method);
void handle_service_unavailable(struct http_resp_s *resp);
int handle_get_req(struct http_resp_s *resp, http_request_t *req);
int handle_head_req(struct http_resp_s *resp, http_request_t *req);
int request_state_handler(http_parser_t  *parser, const char *raw_msg, size_t len);
http_parser_t *http_parser_alloc(void);
void http_parser_reset(http_parser_t *parser);
http_request_t *http_parser_get_request(http_parser_t *pr);
#endif
