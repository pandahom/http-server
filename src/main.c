#include "server-sm.h"
#include "common.h"
#include "request-handler.h"
#include <signal.h>
#include <sys/stat.h>

#define DEFAULT_IP "127.0.0.4"
#define DEFAULT_PORT 8080

static void print_usage(const char *program_name) {
    fprintf(stderr, BOLD "Usage: %s [-i ip_address] [-p port] [-d document_root]\n", program_name);
    fprintf(stderr, "  -i ip_address    IP address to listen on (default: %s)\n", DEFAULT_IP);
    fprintf(stderr, "  -p port          TCP port to listen on (default: %d)\n", DEFAULT_PORT);
    fprintf(stderr, "  -d document_root Directory to serve files from (default: Working Directory)\n");
    fprintf(stderr, "  -h               Show help message\n" DEFAULT);
}

static int validate_ip_address(const char *value) {
    struct in_addr addr_v4;
    struct in6_addr addr_v6;

    return inet_pton(AF_INET, value, &addr_v4) == 1 ||
           inet_pton(AF_INET6, value, &addr_v6) == 1;
}

static int validate_document_root(const char *path) {
    struct stat st = {0};

    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int parse_port(const char *value, uint16_t *port) {
    char *end = NULL;
    unsigned long parsed_port = 0;

    parsed_port = strtoul(value, &end, 10);

    if (*end != '\0' || parsed_port == 0 || parsed_port > UINT16_MAX)
        return FAIL;

    *port = (uint16_t) parsed_port;
    return OK;
}

int main(int argc, char **argv) {
    const char *program_name = argv[0];
    const char *ip_address = DEFAULT_IP;
    uint16_t port = DEFAULT_PORT;
    const char *document_root = "."; // default DOCUMENT-ROOT is Working Directory
    int opt = 0;

    while ((opt = getopt(argc, argv, ":i:p:d:h")) != -1) {
        switch (opt) {
            case 'i':
                if (!validate_ip_address(optarg)) {
                    ERR_LOG("Invalid IP address: %s", optarg);
                    return FAIL;
                }
                ip_address = optarg;
                break;
            case 'p':
                if (parse_port(optarg, &port) == FAIL) {
                    ERR_LOG("Invalid port: %s", optarg);
                    return FAIL;
                }
                break;
            case 'd':
                document_root = optarg;
                break;
            case 'h':
                print_usage(program_name);
                return OK;
            case '?':
                ERR_LOG("Unknown Option: -%c",optopt);
                print_usage(program_name);
                return FAIL;
            case ':':
                ERR_LOG("Missing Argument for: -%c", optopt);
                print_usage(program_name);
                return FAIL;
        }
    }

    if (optind < argc) {
        ERR_LOG("Unexpected argument: %s", argv[optind]);
        print_usage(program_name);
        return FAIL;
    }

    if (!validate_document_root(document_root)) {
        ERR_LOG("Invalid document root: %s", document_root);
        print_usage(program_name);
        return FAIL;
    }

    set_document_root(document_root);

    signal(SIGPIPE, SIG_IGN);
    return handle_srv_states(ip_address, port);
}