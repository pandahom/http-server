#include "server-sm.h"
#include "common.h"
#include "request-handler.h"
#include "log.h"
#include <signal.h>
#include <sys/stat.h>

#define DEFAULT_IP "127.0.0.4"
#define DEFAULT_PORT 8080

                                            
static void print_usage(const char *program_name) {
#define DEFAULT         "\033[0m"
#define BOLD         "\033[1m"
    fprintf(stderr, BOLD "Usage: %s [-i ip_address] [-p port] [-d document_root]\n", program_name);
#undef BOLD
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
                    LOG_ERROR("Invalid IP address: %s", optarg);
                    return FAIL;
                }
                ip_address = optarg;
                break;
            case 'p':
                if (parse_port(optarg, &port) == FAIL) {
                    LOG_ERROR("Invalid port: %s", optarg);
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
                LOG_ERROR("Unknown Option: -%c",optopt);
                print_usage(program_name);
                return FAIL;
            case ':':
                LOG_ERROR("Missing Argument for: -%c", optopt);
                print_usage(program_name);
                return FAIL;
        }
    }

    if (optind < argc) {
        LOG_ERROR("Unexpected argument: %s", argv[optind]);
        print_usage(program_name);
        return FAIL;
    }

    if (!validate_document_root(document_root)) {
        LOG_ERROR("Invalid document root: %s", document_root);
        print_usage(program_name);
        return FAIL;
    }


    signal(SIGPIPE, SIG_IGN);

    log_ctx_init(true, NULL, LOG_OPT_ALL); // TODO: user must provide arguments
    set_document_root(document_root);
    handle_srv_states(ip_address, port);

    log_ctx_final();
    return 0;
}
