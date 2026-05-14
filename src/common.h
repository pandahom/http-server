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
#define FAIL 1

#define RED          "\033[1;31m"
#define YELLOW       "\033[1;33m"
#define CYAN         "\033[1;36m"
#define DEFAULT      "\033[0m"

extern uint16_t num_active_clients;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;


#define MAX_ADDR_LEN INET6_ADDRSTRLEN
#define MAX_RECEIVE_BYTES   (5 * 1024)
#define DEFAULT_STR_SIZE 1024

#define ERR_LOG(msg)  do {                             \
    printf( YELLOW "%s" CYAN "::" DEFAULT , __FILE__);   \
    printf( YELLOW "%s" CYAN "::" DEFAULT, __func__);  \
    printf( YELLOW "%d  " DEFAULT , __LINE__ );        \
    printf( RED "[ERROR]\t"  DEFAULT);                  \
    printf( RED msg"\n" DEFAULT); } while(0)



#endif
