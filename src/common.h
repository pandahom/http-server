#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>

#define OK 0
#define FAIL -1


#define MAX_ADDR_LEN INET6_ADDRSTRLEN
#define MAX_RECEIVE_BYTES   (5 * 1024)
#define CLIENT_RECV_TIMEOUT_SEC 30

#endif
