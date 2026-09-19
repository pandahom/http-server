#ifndef LOG_H
#define LOG_H

#include <time.h>
#include <pthread.h>
#include "common.h"

#define LOG_ERROR(fmt, ...) log_write(LEVEL_ERROR, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_write(LEVEL_INFO,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_write(LEVEL_WARN,  __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log_write(LEVEL_DEBUG, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

typedef enum {
    LEVEL_INFO,
    LEVEL_ERROR,
    LEVEL_WARN,
    LEVEL_DEBUG
} log_level_e;


void log_write(log_level_e level, const char *file, const char *func, int line, const char *fmt, ...);
int log_init(bool dbug, const char *log_file);
void log_final(void);

#endif
