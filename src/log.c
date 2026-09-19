#include "log.h"
#include "ds/queue.h"
#include <stdarg.h>

#define LOG_MAX_QUEUE_ENTRY_NUM 512

#define DEFAULT_STR_SIZE 256

#define COLOR_RESET  "\033[0m"
#define COLOR_BLACK  "\033[30m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_RED    "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE   "\033[34m"

extern _Thread_local uint8_t g_worker_id;
static struct {
    FILE *fp;
    struct {
        int file:1;
        int func:1;
        int line:1;
        int level:1;
    } opt; 

    const char *log_file;
    pthread_t tid;
    queue_t *queue;
    bool debug_mode;
} logger_ctx;

typedef struct {
    log_level_e level;
    struct timespec ts;
    uint8_t caller_worker_id;

    const char *func;
    const char *file;
    int line;
    char msg[DEFAULT_STR_SIZE];
} log_entry_t;

static void *logger_thread(void *);
static const char *get_level_str(log_level_e level);
static const char *get_color_by_level(log_level_e level);
static char* timespec_to_readable_format(char *buf, struct timespec *ts);

static char* timespec_to_readable_format(char *buf, struct timespec *ts) {
    struct tm *tm_info = localtime(&ts->tv_sec);
    strftime(buf, 64, "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

static const char *get_color_by_level(log_level_e level) {
    switch (level) {
        case LEVEL_INFO:
            return COLOR_GREEN;
        case LEVEL_ERROR:
            return COLOR_RED;
        case LEVEL_DEBUG:
            return COLOR_YELLOW;
        case LEVEL_WARN:
            return COLOR_BLUE;
    }
    return COLOR_RESET;
}

static const char *get_level_str(log_level_e level) {
    const char *levels[] = {
        [LEVEL_INFO]  = "INFO",
        [LEVEL_ERROR] = "ERROR",
        [LEVEL_WARN]  = "WARN",
        [LEVEL_DEBUG] = "DEBUG"
    };
    
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
        if (level == i)
            return levels[i];
        
    return "UNKNOWN";
}

int log_init(bool dbug, const char *log_file) {
    logger_ctx.queue = queue_create(LOG_MAX_QUEUE_ENTRY_NUM, sizeof(log_entry_t));
    if (!logger_ctx.queue)
        return FAIL;
    
    if (log_file) {
        logger_ctx.log_file = log_file;
        logger_ctx.fp = fopen(logger_ctx.log_file, "a");
    }
    logger_ctx.debug_mode = dbug;
    pthread_create(&logger_ctx.tid, NULL, logger_thread, NULL);
    return OK;
}

void log_final(void) {
    queue_close(logger_ctx.queue);
    if (logger_ctx.fp)
        fclose(logger_ctx.fp);
    pthread_join(logger_ctx.tid, NULL);
}

void log_write(log_level_e level, const char *file, const char *func, int line, const char *fmt, ...) {
    if (level == LEVEL_DEBUG && !logger_ctx.debug_mode)
        return;

    log_entry_t entry = {
        .caller_worker_id = g_worker_id,
        .file             = file,
        .func             = func,
        .line             = line,
        .level            = level
    };
    va_list ap;

    clock_gettime(CLOCK_REALTIME, &entry.ts);

    va_start(ap, fmt);

    vsnprintf(entry.msg, DEFAULT_STR_SIZE, fmt, ap);
    queue_push(logger_ctx.queue, &entry, sizeof(entry));

    va_end(ap);
}

static void *logger_thread(void *data) {
    (void) data;

    log_entry_t out = {0};
    const char *level_name   = NULL;
    const char *level_color  = NULL;
    char time_str_buf[64];
    int n = 0;

    while (queue_pop(logger_ctx.queue, &out, sizeof(out)) != 1) {
        char line[DEFAULT_STR_SIZE + 20] = {0};
        level_name = get_level_str(out.level);
        level_color =  get_color_by_level(out.level);

        n = snprintf(line, sizeof(line), "%s %s[%s]%s %s::%s:%d:th(%u)  %s\n", 
                timespec_to_readable_format(time_str_buf, &out.ts), 
                level_color, 
                level_name,
                COLOR_RESET, 
                out.file, 
                out.func, 
                out.line, out.caller_worker_id,
                out.msg); 

        if (logger_ctx.fp) fwrite(line, sizeof(char), n, logger_ctx.fp);
        fwrite(line, sizeof(char), n, stdout);
    }

    return NULL;
}

#undef COLOR_RESET
#undef COLOR_BLACK
#undef COLOR_RED
#undef COLOR_GREEN
#undef COLOR_YELLOW
#undef COLOR_BLUE
#undef LOG_MAX_QUEUE_ENTRY_NUM
#undef DEFAULT_STR_SIZE
