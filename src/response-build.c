#include "response-build.h"
#include "common.h"
#include "static-response-bodies/http_400.h"
#include "static-response-bodies/http_404.h"
#include "static-response-bodies/http_500.h"
#include "static-response-bodies/http_505.h"
#include "static-response-bodies/serve_dir.h"

static const struct {
    http_code_e status_code;
    const char *status_phrase;
} http_status[] = {
        {STATUS_OK,                         HTTP_STATUS_OK},
        {STATUS_Created,                    HTTP_STATUS_CREATED},
        {STATUS_No_Content,                 HTTP_STATUS_NO_CONTENT},
        {STATUS_Moved_Permanently,          HTTP_STATUS_MOVED_PERMANENTLY},
        {STATUS_Bad_Request,                HTTP_STATUS_BAD_REQUEST},
        {STATUS_Forbidden,                  HTTP_STATUS_FORBIDDEN},
        {STATUS_Not_Found,                  HTTP_STATUS_NOT_FOUND},
        {STATUS_Not_Implemented,            HTTP_STATUS_NOT_IMPLEMENTED},
        {STATUS_HTTP_Version_Not_Supported, HTTP_STATUS_VERSION_NOT_SUPPORTED}
};

static const char* get_status_message(http_code_e code);
static http_resp_t *http_response_init(void);
static int http_response_add_header(http_resp_t *resp, char *name, char *value);

static const char* get_status_message(http_code_e code) {
    size_t n = sizeof(http_status) / sizeof(http_status[0]);

    for (size_t i = 0; i < n; ++i) {
        if (code == http_status[i].status_code) {
            return http_status[i].status_phrase;
        }
    }
    return NULL;
}

static http_resp_t *http_response_init(void) {
    return (http_resp_t *) calloc(1, sizeof(http_resp_t));
}

static int http_response_add_header(http_resp_t *resp, char *name, char *value) {
    header_t *new_header = (header_t *) calloc(1, sizeof(header_t));
    if (!new_header) {
        ERR_LOG("Could not allocate for header_t");
        return 1;
    }
    new_header->name = name;
    new_header->value = value;
    // Note: Content-Type will not be added here.

    ll_add_node(&resp->headers, new_header);
    return 0;
}

#define http_response_add_body(res_ptr, _body) \
    (res_ptr)->body = (_body)

int build_http_response(http_resp_t** resp, http_code_e code, const char *version) {
    http_resp_t *new_response = http_response_init();
    if (!new_response) {
        ERR_LOG("Could not allocate for http_resp_t");
        return 1;
    }
    *resp = new_response;

    const char *phrase = get_status_message(code);
    if (phrase == NULL) {
        ERR_LOG("No such status exist");
        return 1;
    }

    new_response->phrase = phrase;
    new_response->status_code = code;
    new_response->version = version;

    switch (code) {
        case STATUS_OK:
            break;
        case STATUS_Not_Implemented:
            http_response_add_header(new_response, "Connection", "Close");
            http_response_add_header(new_response, "Content-Type", "text/html");
            http_response_add_body(new_response, BODY_505);

            break;
        case STATUS_HTTP_Version_Not_Supported:
            http_response_add_header(new_response, "Connection", "Close");
            http_response_add_header(new_response, "Content-Type", "text/html");
            http_response_add_body(new_response, BODY_505);
            new_response->body_len = BODY_505_LEN;

//            ll_destroy(new_response->headers, header_t, header);

            break;
        default:
            break;
    }

    return 0;
}