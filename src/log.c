#include "log.h"
#include "ds/queue.h"
#include <stdarg.h>

#define LOG_MAX_QUEUE_ENTRY_NUM 512

#define DEFAULT_STR_SIZE 1024

#define COLOR_RESET  "\033[0m"
#define COLOR_BLACK  "\033[1;30m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_RED    "\033[1;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE   "\033[1;34m"
#define COLOR_GRAY   "\033[1;37m"

extern _Thread_local uint8_t g_worker_id;
static struct {
    FILE *fp;
    union {
        struct {
            uint8_t file_line:1;
            uint8_t func:1;
            uint8_t level:1;
        }; 
        uint8_t flag;
    };

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
static int string_append(char *buf, size_t size, const char *format, va_list ap);
static char *get_color_by_level(log_level_e level);
static char *log_level(char *p, char *last, log_level_e level);
static char *log_file(char *p, char *last, const char* file, int line);
static char *log_func(char *p, char *last, const char* func);
static char *log_message(char *p, char *last, char* message);
static char* log_timestamp(char *p, char *last, struct timespec *ts);
 
static char* string_add(char *buf, char *last, const char *format, ...) {
    int r = -1;
    size_t max_writable_len = last - buf;
    va_list ap;

    va_start(ap, format);
    if (buf < last)
        r = string_append(buf, max_writable_len, format, ap);
    va_end(ap);

    return buf + r;
}
static int string_append(char *buf, size_t size, const char *format, va_list ap) {
    int r = 0;

    r = vsnprintf(buf, size, format, ap);
    buf[size - 1] = '\0'; // To ensure that the buffer is terminated 

    return r;
}

static char* log_timestamp(char *p, char *last, struct timespec *ts) {
    struct tm *tm_info = localtime(&ts->tv_sec);
    char nowstr[64];
    strftime(nowstr, sizeof(nowstr), "%Y-%m-%d %H:%M:%S", tm_info);
    p = string_add(p, last, "%s%s%s: ", COLOR_GREEN, nowstr, COLOR_RESET);
    return p;
}

static char *get_color_by_level(log_level_e level) {
    switch (level) {
        case LEVEL_INFO:
            return COLOR_GREEN;
        case LEVEL_ERROR:
            return COLOR_RED;
        case LEVEL_DEBUG:
            return COLOR_GRAY;
        case LEVEL_WARN:
            return COLOR_BLUE;
    }
    return COLOR_RESET;
}

static char *log_level(char *p, char *last, log_level_e level) {
    const char *levels[] = {
        [LEVEL_INFO]  = "INFO",
        [LEVEL_ERROR] = "ERROR",
        [LEVEL_WARN]  = "WARN",
        [LEVEL_DEBUG] = "DEBUG"
    };
    const char *color = NULL;
    const char *chosen = NULL;
    
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
        if (level == i)
            chosen = levels[i];

    if (chosen == NULL) 
        return p;
        
    color = get_color_by_level(level);
    p = string_add(p, last, "%s[%s]%s ", color, chosen, COLOR_RESET);
    return p;
}

static char *log_file(char *p, char *last, const char* file, int line) {
    p = string_add(p, last, "(%s:%d) ", file, line);
    return p;
}


static char *log_func(char *p, char *last, const char* func) {
    p = string_add(p, last, "%s%s()%s ", COLOR_YELLOW, func, COLOR_RESET);
    return p;
}

static char *log_message(char *p, char *last, char* message) {
    p = string_add(p, last, "%s ", message);
    return p;
}

int log_ctx_init(bool dbug, const char *log_file, uint8_t flag) {
    logger_ctx.queue = queue_create(LOG_MAX_QUEUE_ENTRY_NUM, sizeof(log_entry_t));
    if (!logger_ctx.queue)
        return FAIL;
    
    if (log_file) {
        logger_ctx.log_file = log_file;
        logger_ctx.fp = fopen(logger_ctx.log_file, "a");
    }
    logger_ctx.debug_mode = dbug;
    logger_ctx.flag = flag;
    pthread_create(&logger_ctx.tid, NULL, logger_thread, NULL);
    return OK;
}

void log_ctx_final(void) {
    queue_close(logger_ctx.queue);
    if (logger_ctx.fp)
        fclose(logger_ctx.fp);
    pthread_join(logger_ctx.tid, NULL);
}

void log_write(log_level_e level, const char *file, const char *func, int line, const char *fmt, ...) {
    if (level == LEVEL_DEBUG && !logger_ctx.debug_mode)
        return;

    int r = 0;

    log_entry_t entry = {
        .caller_worker_id = 0, // TODO
        .file             = file,
        .func             = func,
        .line             = line,
        .level            = level
    };
    va_list ap;

    clock_gettime(CLOCK_REALTIME, &entry.ts);

    va_start(ap, fmt);

    r = vsnprintf(entry.msg, DEFAULT_STR_SIZE, fmt, ap);
    if (r < DEFAULT_STR_SIZE && r != -1)
        queue_push(logger_ctx.queue, &entry, sizeof(entry));
    
    va_end(ap);
}

static void *logger_thread(void *data) {
    (void) data;

    log_entry_t out = {0};
    char line[DEFAULT_STR_SIZE] = {0};
    char *p = NULL, *last = NULL; 

    while (queue_pop(logger_ctx.queue, &out, sizeof(out)) != 1) {
        p = line;
        last = line + DEFAULT_STR_SIZE;

        p = log_timestamp(p, last, &out.ts);

        if (logger_ctx.level) p = log_level(p, last, out.level);

        p = log_message(p, last, out.msg);

        if (logger_ctx.file_line) p = log_file(p, last, out.file, out.line);
        if (logger_ctx.func) p = log_func(p, last, out.func);
        p = string_add(p, last, "\n");

        if (logger_ctx.fp) fwrite(line, sizeof(char), p - line, logger_ctx.fp);
        fwrite(line, sizeof(char), p - line, stdout);
    }

    return NULL;
}

#undef COLOR_RESET
#undef COLOR_BLACK
#undef COLOR_RED
#undef COLOR_GREEN
#undef COLOR_YELLOW
#undef COLOR_BLUE
#undef COLOR_GRAY
#undef LOG_MAX_QUEUE_ENTRY_NUM
#undef DEFAULT_STR_SIZE
