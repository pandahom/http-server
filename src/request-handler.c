#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "request-handler.h"
#include "response-build.h"
#include "ds/ht.h"

static const char *DOC_ROOT = NULL;

void set_document_root(const char *doc_root) {
    DOC_ROOT = doc_root;
}

const char* get_document_root(void) {
    return DOC_ROOT;
}

#define BUILD_RESPONSE_DEFAULT_PAGE(response_ptr, code, version, ...) \
    do {\
        if (!is_head_req)\
            build_http_response_default_page(response_ptr, code, version, __VA_ARGS__);\
        else \
            build_http_response_default_page_headers(response_ptr, code, version, __VA_ARGS__);\
    } while(0)

#define BUILD_RESPONSE_FILE(response_ptr, code, version, ...) \
        do {\
            if (!is_head_req)\
                rv = build_http_response_file(response_ptr, code, version, __VA_ARGS__);\
            else\
                rv = build_http_response_file_headers(response_ptr, code, version, __VA_ARGS__);\
        } while(0)

static int handle_get_head_req(struct http_resp_s **resp, http_request_t *req, bool is_head_req) {
    int rv = 0;

    char path[PATH_LEN * 2] = {0};
    struct stat st = {0};

    // TODO: Normalize/Decode Path
    sprintf(path, "%s%s", get_document_root(), DOC_ROOT[strlen(DOC_ROOT) - 1] == '/' ? &req->path[1]: req->path);

    if (strcmp(req->path, "/") == 0) {
        BUILD_RESPONSE_DEFAULT_PAGE(resp, STATUS_OK, HTTP_VERSION(1.0), path, req->path);
    } else {
        if (stat(path, &st) == -1) {
            BUILD_RESPONSE_DEFAULT_PAGE(resp, STATUS_Not_Found, HTTP_VERSION(1.0), &req->path[1]);
        } else {
            if (S_ISDIR(st.st_mode)){
                BUILD_RESPONSE_DEFAULT_PAGE(resp, STATUS_OK, HTTP_VERSION(1.0), path ,&req->path[1]);
            } else {
                BUILD_RESPONSE_FILE(resp, STATUS_OK, HTTP_VERSION(1.0), path);
            }
        }
    }
    return rv;
}

int validate_http_version(char *version) {
    int rv = 1;
    if (!strcmp(version, "HTTP/1.1") || !strcmp(version, "HTTP/1.0"))
        rv = 0;

    return rv;
}

void handle_unsupported_version(struct http_resp_s **resp) {
    build_http_response_default_page(resp, STATUS_HTTP_Version_Not_Supported, HTTP_VERSION(1.0), NULL);
}

void handle_unsupported_method(struct http_resp_s **resp, const char *method) {
    build_http_response_default_page(resp, STATUS_Not_Implemented, HTTP_VERSION(1.0), method);
}

int handle_get_req(struct http_resp_s **resp, http_request_t *req)  {
    return handle_get_head_req(resp, req, false);
}

int handle_head_req(struct http_resp_s **resp, http_request_t *req)  {
    return handle_get_head_req(resp, req, true);
}

int request_state_handler(http_parser_t *parser, const char *raw_msg, size_t len) {
    http_request_t *req = &parser->req;

    for (size_t i = 0; i < len; ++i) {
        char chr = raw_msg[i];

        switch (parser->state) {
            case MSG_STATE_REQUEST_LINE:
                if (chr == ' ') {
                    if (parser->token_len == 0) {
                        fprintf(stderr, "malformed request: leading space\n");
                        parser->state = MSG_STATE_ERROR;
                        return -1;
                    }

                    parser->token[parser->token_len] = '\0';
                    char *method_or_path = (req->method[0] == '\0') ? req->method : req->path;

                    strncpy(method_or_path, parser->token, parser->token_len);
                    parser->token_len = 0;

                } else if (chr == '\r') {
                    parser->token[parser->token_len] = '\0';
                    strncpy(req->version, parser->token, parser->token_len);
                    parser->token_len = 0;

                } else if (chr == '\n') {
                    parser->state = MSG_STATE_HEADER_NAME;

                } else {
                    parser->token[parser->token_len++] = chr;
                }
                break;

            case MSG_STATE_HEADER_NAME:
                if (chr == ':') {
                    parser->token[parser->token_len] = '\0';
                    strncpy(parser->current_header_name, parser->token, parser->token_len + 1);


                    parser->state = MSG_STATE_HEADER_VALUE;
                    parser->token_len = 0;

                } else if (chr == '\r') {
                    parser->state = MSG_STATE_BODY;
                    
                } else if (chr == '\n') {
                    // skip 
                } else {
                    parser->token[parser->token_len++] = chr;
                }
                break;

            case MSG_STATE_HEADER_VALUE:
                if (chr == ' ' && parser->token_len == 0) {
                    // skip
                } else if (chr == '\r') {
                    parser->token[parser->token_len] = '\0';
                    if (ht_insert(&req->headers, parser->current_header_name, parser->token) == -1) {
                        parser->state = MSG_STATE_ERROR;
                    } else {
                        parser->state = MSG_STATE_HEADER_NAME;
                        parser->token_len = 0;
                    }
                } else {
                    parser->token[parser->token_len++] = chr;
                }
                break;

            case MSG_STATE_BODY:
                return 0;
                if (chr == '\n' && parser->token_len == 0)
                {
                    // skip
                }
                break;
            case MSG_STATE_DONE:
                break;
            case MSG_STATE_ERROR:
                break;
            default:
                fprintf(stderr, "unknown parser state: %d\n", parser->state);
                parser->state = MSG_STATE_ERROR;
                return 1;
        }
    }
    return 0;
}