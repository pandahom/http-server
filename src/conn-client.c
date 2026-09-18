#include "conn-client.h"
#include "common.h"
#include "request-handler.h"
#include "response-build.h"
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <sys/sendfile.h>

struct client_ctx_s {
    struct sockaddr_storage address;
    int fd;
    char received_msg[MAX_RECEIVE_BYTES];
    ssize_t received_bytes;

    conn_sm_t sm;
    struct http_parser_s *parser;
    struct http_resp_s *response;
};

static size_t compute_response_size(http_resp_t *resp);
static int send_headers(client_ctx_t *conn_ctx, char *buf, size_t total_size);
static int send_file_response_body(client_ctx_t *conn_ctx, int file_fd, off_t file_size);
static int send_mem_response_body(client_ctx_t *conn_ctx, char *buffer, size_t response_size);

client_ctx_t *client_ctx_alloc(void) {
    client_ctx_t *res = NULL;
    res = (client_ctx_t *) calloc(1, sizeof(client_ctx_t));
    if (res == NULL) {
        ERR_LOG("calloc()");
        return res;
    }

    res->parser = http_parser_alloc();
    if (res->parser == NULL) {
        ERR_LOG("http_parser_alloc()");
        return res;
    }

    res->response = http_response_alloc();
    if (res->response == NULL) {
        ERR_LOG("http_response_alloc()");
        return res;
    }
    return res;
}

void client_ctx_free(client_ctx_t *conn) {
    if (!conn)
        return;
    if (conn->response) 
        free(conn->response);
    if (conn->parser) 
        free(conn->parser);
    free(conn);
}

void client_ctx_reset(client_ctx_t *conn) {
    http_parser_reset(conn->parser);
    memset(conn->response, 0, sizeof(http_resp_t));
    memset(conn, 0, offsetof(client_ctx_t, parser)); // memset zero upto parser part (we must not zero parser or response pointers)
}

void client_ctx_assign_fd_and_address(client_ctx_t *conn, int fd, struct sockaddr_storage *address) {
    conn->fd = fd;
    if (address)
        memcpy(&conn->address, address, sizeof(struct sockaddr_storage));
}

conn_sm_t *client_ctx_sm(client_ctx_t *conn) {
    return &conn->sm;
}

void receive_msg(client_ctx_t *conn_ctx) {
    ssize_t received_bytes = 0;

    conn_ctx->sm.event_trigger = CONN_EVENT_REQ_RECEIVED;

    received_bytes = recv(conn_ctx->fd, conn_ctx->received_msg, MAX_RECEIVE_BYTES, 0);
    if (received_bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            ERR_LOG("recv() timed out after %d seconds", CLIENT_RECV_TIMEOUT_SEC);
        else
            ERR_LOG("recv() returns -1");
        conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
        return;
    }
    conn_ctx->received_bytes = received_bytes;
}

void parse_request(client_ctx_t *conn) {
    if (!conn->parser) {
        ERR_LOG("Could not allocate memory for parser %lu", pthread_self());
        conn->sm.event_trigger = CONN_EVENT_ERROR;
        return;
    }
    int rv = request_state_handler(conn->parser, conn->received_msg, conn->received_bytes);
    if (rv == 0) {
        conn->sm.event_trigger = CONN_EVENT_REQ_PARSED;
    } else
        conn->sm.event_trigger = CONN_EVENT_ERROR;
}

int process_request(client_ctx_t *conn) {
    int            rv    = 0;
    http_parser_t  *pr   = conn->parser;
    http_request_t *req  = http_parser_get_request(pr);
    http_resp_t    *resp = conn->response;

    rv = validate_http_version(req->version);

    if (rv != 0) {
        handle_unsupported_version(resp);
        goto done;
    }

    if (strcmp(req->method, "GET") == 0) {
        rv = handle_get_req(resp, req);
    } else if (strcmp(req->method, "POST") == 0) {
    } else if (strcmp(req->method, "HEAD") == 0) {
        rv = handle_head_req(resp, req);
    } else {
        handle_unsupported_method(resp, req->method);
    }

done:
    conn->response = resp;
    conn->sm.event_trigger = CONN_EVENT_RESP_BUILT;

    return 0;
}
void send_msg(client_ctx_t *conn_ctx) {
    http_resp_t *resp = conn_ctx->response;
    char        *buf  = NULL;
    size_t       total = compute_response_size(resp);
    size_t       offset = 0;
    int          rv = 0;
    int          n;

    buf = malloc(total);
    if (!buf)
        goto cleanup;

    // status line
    n = snprintf(buf + offset, total - offset,
                 "%s %s\r\n", resp->version, resp->phrase);
    if (n < 0) goto cleanup;
    offset += n;

    // headers
    node_t *tm = NULL;
    ll_for_each(resp->headers, tm, tm) {
        header_t *h = (header_t *)tm;
        n = snprintf(buf + offset, total - offset,
                     "%s: %s\r\n", h->name, h->value);
        if (n < 0) goto cleanup;
        offset += n;
    }

    n = snprintf(buf + offset, total - offset, HEADER_CRLF);
    if (n < 0) goto cleanup;
    offset += n;

    // send headers
    rv = send_headers(conn_ctx, buf, offset);
    if (rv == FAIL) {
        conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
        goto cleanup;
    }


    // send body
    switch (resp->body_type) {
        case BODY_TYPE_MEM:
            rv = send_mem_response_body(conn_ctx, resp->body.mem.data, resp->body.mem.len);
            if (rv == FAIL) {
                ERR_LOG("Sending response body buffer failed");
                conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
                goto cleanup;
            }
            break;
        case BODY_TYPE_FILE:
            /*
             I could Use read and write/send to send data but as i have read,
              it comes up with additional user space copy which is unnecessary
              when we can transfer files directly in a zero copy way
            */
            rv = send_file_response_body(conn_ctx, resp->body.file.fd, resp->body.file.len);
            if (rv == FAIL) {
                ERR_LOG("Sending response file failed");
                conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
                goto cleanup;
            }
            break;
        default:
            break;
    }

    conn_ctx->sm.event_trigger = CONN_EVENT_RESP_SENT;

cleanup:
    free(buf);
}

void release_connection_resources(client_ctx_t *conn_ctx) {
    if (!conn_ctx)
        return;

    if (conn_ctx->parser) {
        http_request_t *req = http_parser_get_request(conn_ctx->parser); 
        ht_destroy(&req->headers);
    }

    if (conn_ctx->response) {
        ll_destroy(conn_ctx->response->headers, header_t , h, free(h->name), free(h->value));
        conn_ctx->response->headers = NULL;
        switch (conn_ctx->response->body_type) {
            case BODY_TYPE_MEM:
                free(conn_ctx->response->body.mem.data);
                break;
            case BODY_TYPE_FILE:
                close(conn_ctx->response->body.file.fd);
                break;
            case BODY_TYPE_NONE:
                break;
            default:
                ERR_LOG("Unknown Body Type");
                break;
        }
    }

    close(conn_ctx->fd);
}

static size_t compute_response_size(http_resp_t *resp) {
    size_t size = 0;

    // status line: "HTTP/1.1 200 OK\r\n"
    size += strlen(resp->version) + 1 + strlen(resp->phrase) + 2;

    // headers
    node_t *tm = NULL;
    ll_for_each(resp->headers, tm, tm) {
        header_t *h = (header_t *)tm;
        size += strlen(h->name) + 2 + strlen(h->value) + 2;
        // "Key: Value\r\n"
    }

    size += 3; // 2 for \r\n (Body Part) and 1 for \0 that sprintf add

    return size;
}

static int send_headers(client_ctx_t *conn_ctx, char *buf, size_t total_size) {
    size_t sent = 0;
    ssize_t rv = 0;

    while (sent < total_size) {
        rv = send(conn_ctx->fd, buf + sent, total_size - sent, 0);
        if (rv < 0 && (errno == EPIPE || errno == ECONNRESET)) {
                return FAIL;
        }
        sent += rv;
    }
    return OK;
}

static int send_file_response_body(client_ctx_t *conn_ctx, int file_fd, off_t file_size) {
    off_t file_offset = 0;
    while (file_offset < file_size) {
        off_t remaining = file_size - file_offset;

        ssize_t rv = sendfile(conn_ctx->fd, file_fd, &file_offset, remaining);
        if (rv < 0) {
            return FAIL;
        }
    }
    return OK;
}

static int send_mem_response_body(client_ctx_t *conn_ctx, char *buffer, size_t response_size) {
    size_t sent = 0;
    ssize_t rv  = 0;
    while (sent < response_size) {
        size_t remaining = response_size - sent;
        rv = send(conn_ctx->fd, buffer + sent, remaining, 0);
        if (rv < 0) {
            return FAIL;
        }
        sent += rv;
    }
    return OK;
}

void send_service_unavailable(client_ctx_t *conn) {
    receive_msg(conn);
    handle_service_unavailable(conn->response);
    send_msg(conn);
}
