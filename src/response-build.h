#ifndef RESPONSE_BUILD_H
#define RESPONSE_BUILD_H
#include "ds/ht.h"

#define HTTP_VERSION(v) "HTTP/"#v

/*
 *        Status-Code    = "200"   ; OK
                      | "201"   ; Created
                      | "202"   ; Accepted
                      | "204"   ; No Content
                      | "301"   ; Moved Permanently
                      | "302"   ; Moved Temporarily
                      | "304"   ; Not Modified
                      | "400"   ; Bad Request
                      | "401"   ; Unauthorized
                      | "403"   ; Forbidden
                      | "404"   ; Not Found
                      | "500"   ; Internal Server Error
                      | "501"   ; Not Implemented
                      | "502"   ; Bad Gateway
                      | "503"   ; Service Unavailable
 * */
//HTTP/1.0 400 Bad Request
//Content-Type: text/html; charset=UTF-8

#define HTTP_STATUS_OK                      "200 OK"
#define HTTP_STATUS_CREATED                 "201 Created"
#define HTTP_STATUS_NO_CONTENT              "204 No Content"
#define HTTP_STATUS_MOVED_PERMANENTLY       "301 Moved Permanently"
#define HTTP_STATUS_BAD_REQUEST             "400 Bad Request"
#define HTTP_STATUS_FORBIDDEN               "403 Forbidden"
#define HTTP_STATUS_NOT_FOUND               "404 Not Found"
#define HTTP_STATUS_NOT_IMPLEMENTED         "501 Not Implemented"
#define HTTP_STATUS_VERSION_NOT_SUPPORTED   "505 HTTP Version Not Supported"

#define HEADER_CONTENT_TYPE "Content-Type"
#define HEADER_CONTENT_LENGTH "Content-Length"
#define HEADER_CONNECTION "Connection"

#define HEADER_CONTENT_VALUE_TYPE_HTML    "text/html"
#define HEADER_CONTENT_VALUE_TYPE_TEXT    "text/plain"
#define HEADER_CONTENT_VALUE_TYPE_JSON    "application/json"
#define HEADER_CONNECTION_VALUE_CLOSE     "Close"
#define HEADER_CRLF                 "\r\n"

#define DOC_ROOT "/tmp/miz"

typedef enum {
    STATUS_OK = 200,
    STATUS_Created = 201,
    STATUS_No_Content = 204,
    STATUS_Moved_Permanently = 301,
    STATUS_Bad_Request = 400,
    STATUS_Forbidden = 403,
    STATUS_Not_Found = 404,
    STATUS_Not_Implemented = 501,
    STATUS_HTTP_Version_Not_Supported = 505
} http_code_e;

typedef struct {
    node_t next;
    char *name;
    char *value;
} header_t;

typedef struct http_resp_s {
    // Status line
    const char *version;
    http_code_e      status_code;
    const char    *phrase;

    // Headers
    list_t *headers;

    // Body
    char *body;
    size_t   body_len; // content-length
} http_resp_t;

int build_http_response_default_page(http_resp_t** resp, http_code_e code, const char* version, ...);
void send_response(http_resp_t* resp);
#endif