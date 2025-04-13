#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>

#define UTIL_ANSI_RED     "\x1b[31m"
#define UTIL_ANSI_GREEN   "\x1b[32m"
#define UTIL_ANSI_YELLOW  "\x1b[33m"
#define UTIL_ANSI_BLUE    "\x1b[34m"
#define UTIL_ANSI_MAGENTA "\x1b[35m"
#define UTIL_ANSI_CYAN    "\x1b[36m"
#define UTIL_ANSI_RESET   "\x1b[0m"

typedef enum {
    UNONE = 0x01,
    UINFO = 0x02,
    UWARN = 0x04,
    UFATL = 0x08,
    UPERR = 0x10,
    UTIME = 0x20,
    UTODO = 0x40,
} ulogflag_t;

typedef struct {
    size_t count;
    size_t capacity;
    char *items;
} uarena_t;

#define UASSERT(c) \
    do { if (!(c)) { \
        ulog(UFATL, "%s:%d %s assertion failed '%s'", \
             __FILE__, __LINE__, __func__, #c); \
        abort(); \
    } } while (0)

#define TODO(msg) ulog(UTODO, "%s:%d %s %s", __FILE__, __LINE__, __func__, msg)

#define da_init(da, cap) \
    do { \
        size_t c = (cap) == 0 ? 1 : (cap); \
        (da)->items = malloc(sizeof(*(da)->items) * c); \
        (da)->count = 0; \
        (da)->capacity = (c); \
    } while (0)
#define da_delete(da) \
    do { \
        UASSERT((da)->items != NULL); \
        free((da)->items); \
        (da)->count = 0; \
        (da)->capacity = 0; \
    } while (0)
#define da_resize(da, len) \
    do { \
        (da)->items = urealloc((da)->items, sizeof(*(da)->items)*(len)); \
        (da)->capacity = (len); \
    } while (0)
#define da_reserve(da, len) \
    do { \
        if (((da)->count + (len)) >= (da)->capacity) { \
            UASSERT((da)->capacity > 0); \
            size_t c = (da)->capacity; \
            do c *= 2; while (((da)->count + (len)) >= c); \
            da_resize((da), c); \
        } \
    } while (0)
#define da_append(da, item) \
    do { \
        if ((da)->count >= (da)->capacity) \
            da_resize((da), (da)->capacity * 2); \
        (da)->items[(da)->count++] = (item); \
    } while (0)
#define da_append_many(da, list, len) \
    do { \
        da_reserve((da), (len)); \
        memcpy(&(da)->items[(da)->count], (list), sizeof(*(da)->items) * (len)); \
        (da)->count += (len); \
    } while (0)
#define da_tail(da) ((da)->items + (da)->count)

void *umalloc(size_t size);
void *urealloc(void *p, size_t size);
void ulog(int flags, const char *fmt, ...);

#endif // UTIL_H


#ifdef UTIL_IMPL

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>

void *
umalloc(size_t size)
{
    void *p = malloc(size);
    UASSERT(p != NULL);
    return p;
}

void *
urealloc(void *p, size_t size)
{
    p = realloc(p, size);
    UASSERT(p != NULL);
    return p;
}

void
ulog(int flags, const char *fmt, ...)
{
    if (flags & UTIME) {
        long ms;
        time_t s;
        struct timespec ts;
        struct tm *ti;

        clock_gettime(CLOCK_REALTIME, &ts);

        s = ts.tv_sec;
        ms = ts.tv_nsec / 1.0e6;
        ti = localtime(&s);

        fprintf(stdout, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.3ld ",
                1900 + ti->tm_year, ti->tm_mon, ti->tm_mday,
                ti->tm_hour, ti->tm_min, ti->tm_sec, ms);
    }

    if (flags & UINFO) fprintf(stdout, "[%sINFO%s] ", UTIL_ANSI_GREEN, UTIL_ANSI_RESET);
    else if (flags & UWARN) fprintf(stdout, "[%sWARN%s] ", UTIL_ANSI_YELLOW, UTIL_ANSI_RESET);
    else if (flags & UFATL) fprintf(stdout, "[%sFATL%s] ", UTIL_ANSI_RED, UTIL_ANSI_RESET);
    else if (flags & UPERR) fprintf(stdout, "[%sPERR%s] ", UTIL_ANSI_MAGENTA, UTIL_ANSI_RESET);
    else if (flags & UTODO) fprintf(stdout, "[%sTODO%s] ", UTIL_ANSI_BLUE, UTIL_ANSI_RESET);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    if (flags & UPERR)
        strerror(errno);

    fputc('\n', stdout);
}

#endif // UTIL_IMPL
