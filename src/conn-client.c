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
    }
    conn_ctx->received_bytes = received_bytes;
}

void parse_request(client_ctx_t *conn) {
    conn->parser = (void*) calloc(1, sizeof(http_parser_t));
    if (!conn->parser) {
        ERR_LOG("Could not allocate memory for parser");
        return;
    }
    int rv = request_state_handler(conn->parser, conn->received_msg, conn->received_bytes);
//    list_or_ht_show(NULL, conn->parser->req.header_list);
    if (rv == 0) {
        conn->sm.event_trigger = CONN_EVENT_REQ_PARSED;
    } else
        conn->sm.event_trigger = CONN_EVENT_ERROR;
}

int process_request(client_ctx_t *conn) {
    int rv = 0;
    http_parser_t *pr = conn->parser;
    http_request_t *req = &pr->req;

    rv = validate_http_version(req->version);

    if (rv != 0) {
        handle_unsupported_version(&conn->response);
        goto done;
    }

    if (strcmp(req->method, "GET") == 0) {
        rv = handle_get_req(req);
    } else if (strcmp(req->method, "POST") == 0) {
    } else if (strcmp(req->method, "HEAD") == 0) {
    } else {
//        rv = handle_unsupported_method()
    }

done:
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

    // Content-Length + end of headers
    n = snprintf(buf + offset, total - offset,
                 HEADER_CONTENT_LENGTH ": %zu\r\n\r\n", resp->body_len);
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
    pthread_mutex_lock(&mutex);
    num_active_clients--;
    if (num_active_clients == 0)
        pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    ht_destroy(&conn_ctx->parser->req.headers);
    close(conn_ctx->fd);
    free(conn_ctx->parser);
    ll_destroy(conn_ctx->response->headers, node_t , h);
    free(conn_ctx->response);

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

    // content-length header
    char tmp[64];
    size += snprintf(tmp, sizeof(tmp),
        "Content-Length: %zu\r\n",
        strlen(resp->body)
    );

    size += 3 + strlen(resp->body); // 2 for \r\n and 1 for \0 that sprintf add

    return size;
}