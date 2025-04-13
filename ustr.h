#ifndef USTR_H
#define USTR_H

#include <stddef.h>

#define isletter(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define isnumber(c) (c >= '0' && c <= '9')
#define isuscore(c) (c == '_')
#define isperiod(c) (c == '.')
#define iseos(c) (c == '\0')

typedef struct {
    size_t len;
    const char *str;
} uslice_t;

typedef struct {
    size_t count;
    size_t capacity;
    uslice_t *items;
} uslicelist_t;

typedef struct {
    size_t count;
    size_t capacity;
    char *items;
} ustr_builder_t;

// allocate string builder
void ustr_builder_alloc(ustr_builder_t *builder);

// free string builder
void ustr_builder_free(ustr_builder_t *builder);

// leak string memory from builder
char *ustr_builder_leak(ustr_builder_t *builder);

void ustr_builder_putc(ustr_builder_t *builder, char c);
void ustr_builder_puts(ustr_builder_t *builder, const char *s);
void ustr_builder_printf(ustr_builder_t *builder, const char *fmt, ...);
void ustr_builder_concat_list(ustr_builder_t *builder, const char **s, int count);
void ustr_builder_concat_var(ustr_builder_t *builder, ...);

#endif // USTR_H


#define USTR_IMPL
#ifdef USTR_IMPL

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "util.h"


void
ustr_builder_alloc(ustr_builder_t *builder)
{
    UASSERT(builder);
    da_init(builder, 4096);
}

void
ustr_builder_free(ustr_builder_t *builder)
{
    UASSERT(builder);
    da_delete(builder);
}

char *
ustr_builder_leak(ustr_builder_t *builder)
{
    char *s = builder->items;
    memset(builder, 0, sizeof(*builder));
    return s;
}

void
ustr_builder_putc(ustr_builder_t *builder, char c)
{
    UASSERT(builder);
    da_append(builder, c);
}

void
ustr_builder_puts(ustr_builder_t *builder, const char *s)
{
    UASSERT(builder);
    UASSERT(s);
    da_append_many(builder, s, strlen(s));
}

void
ustr_builder_printf(ustr_builder_t *builder, const char *fmt, ...)
{
    UASSERT(builder);
    UASSERT(fmt);

    va_list args;
    va_start(args, fmt);

    size_t len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    va_start(args, fmt);
    vsnprintf(builder->items + builder->count, len + 1, fmt, args);
    va_end(args);

    builder->count += len;
}

void
ustr_builder_concat_list(ustr_builder_t *builder, const char **s, int count)
{
    while (count--) {
        da_append_many(builder, *s, strlen(*s));
        s++;
    }
}

void
ustr_builder_concat_var(ustr_builder_t *builder, ...)
{
    va_list args;
    va_start(args, builder);
    char *s;
    while ((s = va_arg(args, char *)) != 0)
        da_append_many(builder, s, strlen(s));
    va_end(args);
}

#endif // USTR_IMPL
