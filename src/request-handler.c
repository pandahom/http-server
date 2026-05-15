#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "request-handler.h"
#include "ds/ht.h"
//
//static void handle_request_line(http_parser_t *p, const char *raw_msg, size_t len) {
//
//}

static char *supported_methods[1] = {
        "GET"
};

int request_state_handler(http_parser_t *parser, const char *raw_msg, size_t len) {
    parser->req.headers = ht_init();

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
                    char *method_or_path = (parser->req.method[0] == '\0') ? parser->req.method : parser->req.path;

                    strncpy(method_or_path, parser->token, parser->token_len);
                    parser->token_len = 0;

                } else if (chr == '\r') {
                    parser->token[parser->token_len] = '\0';
                    strncpy(parser->req.version, parser->token, parser->token_len);
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
                    if (ht_insert(parser->req.headers, parser->current_header_name, parser->token) == -1) {
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
                break;
        }
    }
    return 0;
}