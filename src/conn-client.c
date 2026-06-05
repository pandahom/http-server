#include "conn-client.h"
#include "request-handler.h"
#include "response-build.h"
#include <pthread.h>
static size_t compute_response_size(http_resp_t *resp);

void receive_msg(client_ctx_t *conn_ctx) {
    ssize_t received_bytes = 0;

    conn_ctx->sm.event_trigger = CONN_EVENT_REQ_RECEIVED;

    received_bytes = recv(conn_ctx->fd, conn_ctx->received_msg, MAX_RECEIVE_BYTES, 0);
    if (received_bytes == -1) {
        ERR_LOG("recv() returns -1");
        conn_ctx->sm.event_trigger = CONN_EVENT_ERROR;
        return;
    }
    conn_ctx->received_bytes = received_bytes;
}

void parse_request(client_ctx_t *conn) {
    conn->parser = (void*) calloc(1, sizeof(http_parser_t));
    if (!conn->parser) {
        ERR_LOG("Could not allocate memory for parser");
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
    http_request_t *req  = &pr->req;
    http_resp_t    *resp = conn->response;

    rv = validate_http_version(req->version);

    if (rv != 0) {
        handle_unsupported_version(&resp);
        goto done;
    }

    if (strcmp(req->method, "GET") == 0) {
        rv = handle_get_req(&resp, req);
    } else if (strcmp(req->method, "POST") == 0) {
    } else if (strcmp(req->method, "HEAD") == 0) {
    } else {
        handle_unsupported_method(&resp, req->method);
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
    size_t sent = 0;
    while (sent < offset) {
        ssize_t rv = send(conn_ctx->fd, buf + sent, offset - sent, 0);
        if (rv < 0) goto cleanup;
        sent += rv;
    }

    // send body separately — no copy into buf
    sent = 0;
    while (sent < resp->body_len) {
        ssize_t rv = send(conn_ctx->fd, resp->body + sent, resp->body_len - sent, 0);
        if (rv < 0) goto cleanup;
        sent += rv;
    }

    conn_ctx->sm.event_trigger = CONN_EVENT_RESP_SENT;

cleanup:
    free(buf);
}

void destroy_connection(client_ctx_t *conn_ctx) {
    if (!conn_ctx)
        return;

    pthread_mutex_lock(&mutex);
    num_active_clients--;
    if (num_active_clients == 0)
        pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    if (conn_ctx->parser) {
        ht_destroy(&conn_ctx->parser->req.headers);
        free(conn_ctx->parser);
    }
    if (conn_ctx->response) {
        ll_destroy(conn_ctx->response->headers, header_t , h, free(h->name), free(h->value));
        free(conn_ctx->response->body);
        free(conn_ctx->response);
    }

    close(conn_ctx->fd);
    free(conn_ctx);
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

    size += 3 + resp->body_len; // 2 for \r\n (Body Part) and 1 for \0 that sprintf add

    return size;
}