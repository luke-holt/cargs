#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>

typedef enum {
    UNONE = 0x01,
    UINFO = 0x02,
    UWARN = 0x04,
    UFATL = 0x08,
    UPERR = 0x10,
    UTIME = 0x20,
} ulogflag_t;

typedef struct {
    size_t count;
    size_t capacity;
    char *items;
} uarena_t;

typedef struct {
    size_t len;
    const char *str;
} uslice_t;

typedef struct {
    size_t count;
    size_t capacity;
    uslice_t *items;
} uslicelist_t;

#define UASSERT(c) \
    do { if (!(c)) { \
        ulog(UFATL, "%s:%d %s assertion failed '%s'", \
             __FILE__, __LINE__, __func__, #c); \
        abort(); \
    } } while (0)

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
        memset((da), 0, sizeof(*(da))); \
    } while (0)
#define da_resize(da, len) \
    do { \
        (da)->items = urealloc((da)->items, sizeof(*(da)->items)*(len)); \
        (da)->capacity = (len); \
    } while (0)
#define da_append(da, item) \
    do { \
        if ((da)->count >= (da)->capacity) \
            da_resize((da), (da)->capacity * 2); \
        (da)->items[(da)->count++] = (item); \
    } while (0)
#define da_append_many(da, list, len) \
    do { \
        if (((da)->count + (len)) >= (da)->capacity) { \
            UASSERT((da)->capacity > 0); \
            do (da)->capacity *= 2; \
            while (((da)->count + (len)) >= (da)->capacity); \
            da_resize((da), (da)->capacity); \
        } \
        memcpy(&(da)->items[(da)->count], (list), sizeof(*(da)->items) * (len)); \
        (da)->count += (len); \
    } while (0)

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

    if (flags & UINFO) fprintf(stdout, "[INFO] ");
    else if (flags & UWARN) fprintf(stdout, "[WARN] ");
    else if (flags & UFATL) fprintf(stdout, "[FATL] ");
    else if (flags & UPERR) fprintf(stdout, "[PERR] ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    if (flags & UPERR)
        strerror(errno);

    fputc('\n', stdout);
}

#endif // UTIL_IMPL
