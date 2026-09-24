#ifndef LOG_H
#define LOG_H

#include <time.h>
#include <pthread.h>
#include "common.h"

#define LOG_ERROR(...) log_write(LEVEL_ERROR, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_write(LEVEL_INFO,  __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_write(LEVEL_WARN,  __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_write(LEVEL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__)

#define LOG_OPT_FILE_AND_LINE  (1 << 0)
#define LOG_OPT_FUNC           (1 << 1)
#define LOG_OPT_LEVEL          (1 << 2)
#define LOG_OPT_ALL            (LOG_OPT_LEVEL|LOG_OPT_FUNC|LOG_OPT_FILE_AND_LINE)
typedef enum {
    LEVEL_INFO,
    LEVEL_ERROR,
    LEVEL_WARN,
    LEVEL_DEBUG
} log_level_e;


void log_write(log_level_e level, const char *file, const char *func, int line, const char *fmt, ...);
int log_ctx_init(bool dbug, const char *log_file, uint8_t flag);
void log_ctx_final(void);

#endif
