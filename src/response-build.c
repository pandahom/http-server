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
static http_resp_t *http_response_init(void);

static int populate_status_line(http_resp_t *resp, http_code_e code, const char *version);
static const char *get_content_type(const char *path);
static const char* get_status_message(http_code_e code);

static ssize_t http_response_compute_content_length(const char *format, ...);
static ssize_t http_response_add_body(http_resp_t *resp, const char *format, ...);

static int http_response_add_header(http_resp_t *resp, char *name, char *value);
static int populate_entity_headers(http_resp_t *resp, const char *content_type, size_t content_len);

static int build_http_response_default_page_impl(http_resp_t** resp, http_code_e code, const char* version,
    bool include_body, va_list ap);
static int build_http_response_file_impl(http_resp_t** resp, http_code_e code, const char* version, const char *path, bool include_body);

static const char* get_status_message(http_code_e code) {
    size_t n = sizeof(http_status) / sizeof(http_status[0]);

    for (size_t i = 0; i < n; ++i) {
        if (code == http_status[i].status_code) {
            return http_status[i].status_phrase;
        }
    }
    return NULL;
}

static int populate_status_line(http_resp_t *resp, http_code_e code, const char *version) {
    const char *phrase = get_status_message(code);
    if (phrase == NULL) {
        ERR_LOG("No such status exist");
        return FAIL;
    }

    resp->phrase = phrase;
    resp->status_code = code;
    resp->version = version;

    return OK;
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
    http_resp_t *new_response = (http_resp_t *) calloc(1, sizeof(http_resp_t));

    if (new_response == NULL)
        return NULL;

    new_response->body_type = BODY_TYPE_NONE;
    return new_response;
}

static int populate_entity_headers(http_resp_t *resp, const char *content_type, size_t content_len) {
    char content_len_str[20] = {0};

    snprintf(content_len_str, sizeof(content_len_str), "%zu", content_len);

    if (http_response_add_header(resp, HEADER_CONNECTION, HEADER_CONNECTION_VALUE_CLOSE) != 0)
        return FAIL;
    if (http_response_add_header(resp, HEADER_CONTENT_TYPE, (char *) content_type) != 0)
        return FAIL;
    if (http_response_add_header(resp, HEADER_CONTENT_LENGTH, content_len_str) != 0)
        return FAIL;

    return OK;
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

static ssize_t http_response_compute_content_length(const char *format, ...) {
    va_list ap;
    ssize_t content_length = 0;
    va_start(ap, format);
    content_length = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    return content_length;
}

static ssize_t http_response_add_body(http_resp_t *resp, const char *format, ...) {
    va_list ap, copy_ap;
    va_start(ap, format);
    va_copy(copy_ap, ap);

    ssize_t body_len = vsnprintf(NULL, 0, format, copy_ap);
    if (body_len < 0) {
        ERR_LOG("Could not compute response body length");
        va_end(copy_ap);
        va_end(ap);
        return FAIL;
    }
    va_end(copy_ap);

    resp->body.mem.data = (char *) calloc((size_t) body_len + 1, sizeof(char));
    if (!resp->body.mem.data) {
        ERR_LOG("Could not allocate for body");
        va_end(ap);
        return FAIL;
    }

    vsnprintf(resp->body.mem.data, body_len + 1, format, ap);

    va_end(ap);
    resp->body_type = BODY_TYPE_MEM;
    return body_len;
}

static int build_http_response_file_impl(http_resp_t** resp, http_code_e code, const char* version, const char *path, bool include_body) {
    http_resp_t *new_response = NULL;
    int fd;
    struct stat st = {0};
    int rv = OK;
    const char *content_type = NULL;

    new_response = http_response_init();
    if (!new_response) {
        return FAIL;
    }
    *resp = new_response;

    rv = stat(path, &st);
    if (rv == FAIL) {
        ERR_LOG("FATA ERROR ");
        return FAIL;
    }

    rv = populate_status_line(new_response, code, version);
    if (rv == FAIL) {
        ERR_LOG("Populating response status line");
        return FAIL;
    }

    content_type = get_content_type(path);

    rv = populate_entity_headers(new_response, content_type, st.st_size);
    if (rv == FAIL) {
        ERR_LOG("Populating entity headers");
        return FAIL;
    }

    if (!include_body) // This is a HEAD request
        return OK;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        build_http_response_default_page(resp, STATUS_Not_Found, version, path);
        ERR_LOG("FATA ERROR");
        return FAIL;
    }

    new_response->body.file.len = st.st_size;
    new_response->body.file.fd = fd;
    new_response->body_type = BODY_TYPE_FILE;

    return OK;
}

int build_http_response_file_headers(http_resp_t** resp, http_code_e code, const char* version, const char *path) {
    return build_http_response_file_impl(resp, code, version, path, false);
}

int build_http_response_file(http_resp_t** resp, http_code_e code, const char* version, const char *path) {
    return build_http_response_file_impl(resp, code, version, path, true);
}

static int build_http_response_default_page_impl(http_resp_t** resp, http_code_e code, const char* version,
    bool include_body, va_list ap) {

    int rv = 0;
    char *arg = NULL, *arg2 = NULL;
    arg = va_arg(ap, char *);
    ssize_t content_length = 0;

    http_resp_t *new_response = http_response_init();
    if (!new_response) {
        ERR_LOG("Could not allocate for http_resp_t");
        return FAIL;
    }
    *resp = new_response;

    rv = populate_status_line(new_response, code, version);
    if (rv == FAIL)
        return FAIL;

    switch (code) {
     // TODO: Handle Return Value on ERROR
        case STATUS_OK:
            arg2 = va_arg(ap, char *);

            char elemtns[4096] = {0};
            list_dir(arg,elemtns);

            if (include_body) {
                content_length =  http_response_add_body(new_response, DEFAULT_PAGE, arg, arg2,  elemtns);
                new_response->body.mem.len = content_length;
            } else
                content_length = http_response_compute_content_length(DEFAULT_PAGE, arg, arg2,  elemtns);

            if (content_length < 0)
                return FAIL;

            rv = populate_entity_headers(new_response, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML,  content_length);
            if (rv == FAIL)
                return FAIL;

            break;
        case STATUS_Not_Found:
            if (include_body) {
                content_length = http_response_add_body(new_response, BODY_404, arg);
                new_response->body.mem.len = content_length;
            } else
                content_length = http_response_add_body(new_response, BODY_404, arg);

            if (content_length < 0)
                return FAIL;

            rv = populate_entity_headers(new_response, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML,  content_length);
            if (rv == FAIL)
                return FAIL;

            break;
        case STATUS_Not_Implemented:
            if (include_body) {
                content_length = http_response_add_body(new_response, BODY_501, (char *) arg);
                new_response->body.mem.len = content_length;
            } else
                content_length = http_response_add_body(new_response, BODY_501, (char *) arg);;


            if (content_length < 0)
                return FAIL;

            rv = populate_entity_headers(new_response, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML,  content_length);
            if (rv == FAIL)
                return FAIL;

            break;
        case STATUS_HTTP_Version_Not_Supported:
            if (include_body) {
                content_length = http_response_add_body(new_response, BODY_505);
                new_response->body.mem.len = content_length;
            } else
                content_length = http_response_add_body(new_response, BODY_505);

            if (content_length < 0)
                return FAIL;

            rv = populate_entity_headers(new_response, HEADER_CONTENT_VALUE_TYPE_TEXT_HTML,  content_length);
            if (rv == FAIL)
                return FAIL;

            break;
        default:
            break;
    }

    return 0;
}

int build_http_response_default_page(http_resp_t** resp, http_code_e code, const char* version, ...) {
    int rv = 0;
    va_list ap;
    va_start(ap, version);

    rv = build_http_response_default_page_impl(resp, code, version, true, ap);

    va_end(ap);
    return rv;
}

int build_http_response_default_page_headers(http_resp_t** resp, http_code_e code, const char* version, ...) {
    int rv = 0;
    va_list ap;
    va_start(ap, version);

    rv = build_http_response_default_page_impl(resp, code, version, false, ap);

    va_end(ap);
    return rv;
}