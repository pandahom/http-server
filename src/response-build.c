#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "path-handler.h"
#include "response-build.h"
#include "common.h"
#include "static-response-bodies/http_400.h"
#include "static-response-bodies/http_404.h"
#include "static-response-bodies/http_500.h"
#include "static-response-bodies/http_505.h"
#include "static-response-bodies/http_501.h"
#include "static-response-bodies/default_dir_list_page.h"

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
static ssize_t http_response_add_body(http_resp_t *resp, const char *format, ...);
static const char *get_content_type(const char *path);

static const char* get_status_message(http_code_e code) {
    size_t n = sizeof(http_status) / sizeof(http_status[0]);

    for (size_t i = 0; i < n; ++i) {
        if (code == http_status[i].status_code) {
            return http_status[i].status_phrase;
        }
    }
    return NULL;
}

const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.'); // STRRCHR return last occurrence

    if (!ext)
        return HEADER_CONTENT_VALUE_TYPE_APP_OCTECT_STREAM;

    if (strcmp(ext, ".html") == 0)
        return HEADER_CONTENT_VALUE_TYPE_TEXT_HTML;

    if (strcmp(ext, ".htm") == 0)
        return HEADER_CONTENT_VALUE_TYPE_TEXT_HTML;

    if (strcmp(ext, ".css") == 0)
        return HEADER_CONTENT_VALUE_TYPE_TEXT_CSS;

    if (strcmp(ext, ".js") == 0)
        return HEADER_CONTENT_VALUE_TYPE_APP_JS;

    if (strcmp(ext, ".json") == 0)
        return HEADER_CONTENT_VALUE_TYPE_APP_JSON;

    if (strcmp(ext, ".png") == 0)
        return HEADER_CONTENT_VALUE_TYPE_IMAGE_PNG;

    if (strcmp(ext, ".jpg") == 0)
        return HEADER_CONTENT_VALUE_TYPE_IMAGE_JPG;

    if (strcmp(ext, ".jpeg") == 0)
        return HEADER_CONTENT_VALUE_TYPE_IMAGE_JPG;

    if (strcmp(ext, ".gif") == 0)
        return HEADER_CONTENT_VALUE_TYPE_IMAGE_GIF;

    if (strcmp(ext, ".pdf") == 0)
        return HEADER_CONTENT_VALUE_TYPE_APP_PDF;

    if (strcmp(ext, ".txt") == 0)
        return HEADER_CONTENT_VALUE_TYPE_TEXT_PLAIN;

    return HEADER_CONTENT_VALUE_TYPE_APP_OCTECT_STREAM;
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
    new_header->name = strdup(name);
    new_header->value = strdup(value);

    ll_add_node(&resp->headers, new_header);
    return 0;
}

static ssize_t http_response_add_body(http_resp_t *resp, const char *format, ...) {
    va_list ap, copy_ap;
    va_start(ap, format);
    va_copy(copy_ap, ap);

    ssize_t body_len = vsnprintf(NULL, 0, format, copy_ap);
    if (body_len < 0) {
        ERR_LOG("Could not compute response body length");
        return 1;
    }
    va_end(copy_ap);

    resp->body.mem.data = (char *) calloc((size_t) body_len + 1, sizeof(char));
    if (!resp->body.mem.data) {
        ERR_LOG("Could not allocate for body");
        return 1;
    }

    vsnprintf(resp->body.mem.data, body_len + 1, format, ap);

    va_end(ap);
    return body_len;
}



int build_http_file_response(http_resp_t** resp, http_code_e code, const char* version, const char *path) {
    http_resp_t *new_response = NULL;
    int fd;
    struct stat st = {0};
    int rv = 0;
    (void)rv;
    char content_len[20] = {0};
    char *content_type = NULL;

    new_response = http_response_init();
    if (!new_response) {
        return 1;
    }
    *resp = new_response;


    if (stat(path, &st) == -1) {
        ERR_LOG("FATA ERROR ");
        return 1;
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        build_http_response_default_page(resp, STATUS_Not_Found, version, path);
        ERR_LOG("FATA ERROR ");
        return 1;
    }


    const char *phrase = get_status_message(code);
    if (phrase == NULL) {
        ERR_LOG("No such status exist");
        return 1;
    }

    new_response->phrase = phrase;
    new_response->status_code = code;
    new_response->version = version;
    new_response->body.file.len = st.st_size;
    new_response->body.file.fd = fd;
    new_response->body_type = BODY_TYPE_FILE;


    content_type = (char *) get_content_type(path);
    snprintf(content_len, 20, "%zu", new_response->body.file.len);

    rv = http_response_add_header(new_response, HEADER_CONNECTION, HEADER_CONNECTION_VALUE_CLOSE);
    rv = http_response_add_header(new_response, HEADER_CONTENT_TYPE, content_type);
    rv = http_response_add_header(new_response, HEADER_CONTENT_LENGTH, content_len);

    return 0;
}

int build_http_response_default_page(http_resp_t** resp, http_code_e code, const char* version, ...) {
    int rv = 0;
    char content_len[10] = {0};
    va_list ap;
    va_start(ap, version);
    char *arg = NULL, *arg2 = NULL;
    arg = va_arg(ap, char *);

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
    new_response->body_type = BODY_TYPE_MEM;
    (void)rv;
    switch (code) {
     // TODO: Handle Return Value on ERROR
        case STATUS_OK:
            arg2 = va_arg(ap, char *);

            char elemtns[4096] = {0};
            list_dir(arg,elemtns);

            new_response->body.mem.len =  http_response_add_body(new_response, DEFAULT_PAGE, arg, arg2,  elemtns);
            rv = http_response_add_header(new_response, HEADER_CONNECTION, HEADER_CONNECTION_VALUE_CLOSE);
            rv = http_response_add_header(new_response, HEADER_CONTENT_TYPE, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML);

            snprintf(content_len, 10, "%zu", new_response->body.mem.len);
            rv = http_response_add_header(new_response, HEADER_CONTENT_LENGTH, content_len);

            break;
        case STATUS_Not_Found:
            new_response->body.mem.len =  http_response_add_body(new_response, BODY_404, arg);
            rv = http_response_add_header(new_response, HEADER_CONNECTION, HEADER_CONNECTION_VALUE_CLOSE);
            rv = http_response_add_header(new_response, HEADER_CONTENT_TYPE, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML);

            snprintf(content_len, 10, "%zu", new_response->body.mem.len);
            rv = http_response_add_header(new_response, HEADER_CONTENT_LENGTH, content_len);

            break;
        case STATUS_Not_Implemented:
            new_response->body.mem.len =  http_response_add_body(new_response, BODY_501, (char *) arg);
            rv = http_response_add_header(new_response, HEADER_CONNECTION, HEADER_CONNECTION_VALUE_CLOSE);
            rv = http_response_add_header(new_response, HEADER_CONTENT_TYPE, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML);

            snprintf(content_len, 10, "%zu", new_response->body.mem.len);
            rv = http_response_add_header(new_response, HEADER_CONTENT_LENGTH, content_len);
            break;
        case STATUS_HTTP_Version_Not_Supported:
            new_response->body.mem.len = http_response_add_body(new_response, BODY_505);

            rv = http_response_add_header(new_response, HEADER_CONNECTION, HEADER_CONNECTION_VALUE_CLOSE);
            rv = http_response_add_header(new_response, HEADER_CONTENT_TYPE, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML);
            snprintf(content_len, 10, "%zu", new_response->body.mem.len);
            rv = http_response_add_header(new_response, HEADER_CONTENT_LENGTH, content_len);

            break;
        default:
            break;
    }
    va_end(ap);

    return 0;
}