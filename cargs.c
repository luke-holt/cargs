#include <stdio.h>

#include "cargs.h"

#include "util.h"

typedef enum {
    CARGS_BOOL,
    CARGS_INT,
    CARGS_FLOAT,
    CARGS_STR,
    CARGS_CHAIN,
} dtype_t;

typedef struct {
    const char *name;
    const char *help;
    void *ptr;
    int *ptrlen;
    dtype_t dtype;
} opt_t;

typedef struct {
    size_t count;
    size_t capacity;
    opt_t *items;
} optlist_t;

typedef struct {
    uarena_t arena;
    optlist_t optlist;
    int namemaxlen;
    int helpmaxlen;
} ctx_t;

static inline bool isletter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline bool isnumber(char c) { return c >= '0' && c <= '9'; }
static inline bool isuscore(char c) { return c == '_'; }
static inline bool isperiod(char c) { return c == '.'; }
static inline bool iseos(char c) { return c == '\0'; }

static opt_t
newopt(ctx_t *ctx, const char *name, const char *help, void *ptr, void *ptrlen, dtype_t dtype) {
    UASSERT(ctx);
    UASSERT(name);
    UASSERT(help);

    opt_t opt;
    char *s;
    int len;

    // copy name to arena
    s = da_tail(&ctx->arena);
    len = strlen(name);
    da_append_many(&ctx->arena, name, len + 1);
    s[len] = '\0';
    opt.name = s;

    if (ctx->namemaxlen < len)
        ctx->namemaxlen = len;

    // copy help to &ctx->arena
    s = da_tail(&ctx->arena);
    len = strlen(help);
    da_append_many(&ctx->arena, help, len + 1);
    s[len] = '\0';
    opt.help = s;

    if (ctx->helpmaxlen < len)
        ctx->helpmaxlen = len;

    opt.ptr = ptr;
    opt.ptrlen = ptrlen;
    opt.dtype = CARGS_BOOL;

    return opt;
}

static bool parse_chain(const char *chain, uslicelist_t *list);

int
match_ident(const char *s)
{
    size_t n = 0;
    for (;;) {
        // reached end of current identifier
        if (isperiod(s[n]) || iseos(s[n])) {
            return n;
        }
        // invalid identifier char
        if (!(isletter(s[n]) || isuscore(s[n]) || isnumber(s[n])))
            return -(n + 1);
        n++;
    }
}

bool
parse_chain(const char *chain, uslicelist_t *list)
{
    da_init(list, 1);
    size_t len = strlen(chain);
    size_t i = 0;
    size_t n = 0;
    for (;;) {
        int l = match_ident(chain + i);

        // match error
        if (l < 0) {
            ulog(UWARN, "invalid char in chain");
            ulog(UWARN, "%s", chain);
            ulog(UWARN, "%*s", i - l, "^");
            da_delete(list);
            return false;
        }

        // add slice to list
        da_append(list, ((uslice_t) { .len = l, .str = chain + i }));
        i += l;

        // reached end of chain string
        if (iseos(chain[i]))
            break;

        // jump over divider
        else if (isperiod(chain[i]) && !iseos(chain[i+1]))
            i++;

        // trailing period
        else {
            ulog(UWARN, "trailing period in chain");
            ulog(UWARN, "%s", chain);
            ulog(UWARN, "%*s", i+1, "^");
            da_delete(list);
            return false;
        }
    }

    da_resize(list, list->count);
    return true;
}

void
cargs_init(cargs_t *context)
{
    UASSERT(context);
    ctx_t *ctx = umalloc(sizeof(ctx_t));
    da_init(&ctx->arena, 4096);
    da_init(&ctx->optlist, 1);
    ctx->namemaxlen = 0;
    ctx->helpmaxlen = 0;
    *context = (cargs_t)ctx;
}

void
cargs_delete(cargs_t *context)
{
    UASSERT(context);
    UASSERT(*context);
    ctx_t *ctx = (ctx_t *)*context;
    da_delete(&ctx->arena);
    da_delete(&ctx->optlist);
    free(ctx);
    *context = (cargs_t)NULL;
}

void
cargs_add_opt_flag(cargs_t context, bool *v, const char *name, const char *help)
{
    UASSERT(context);
    ctx_t *ctx = (ctx_t *)context;
    opt_t opt = newopt(ctx, name, help, (void *)v, NULL, CARGS_BOOL);
    da_append(&ctx->optlist, opt);
}

void
cargs_add_opt_int(cargs_t context, int *v, const char *name, const char *help)
{
}

void
cargs_add_opt_float(cargs_t context, float *v, const char *name, const char *help)
{
}

void
cargs_add_opt_str(cargs_t context, char *v, const char *name, const char *help)
{
}

// void
// cargs_add_opt_chain(cargs_t context, char **v, int *vlen, const char *name, const char *help)
// {
// }

#include <stdarg.h>

void ustrbuild_printf(uarena_t *arena, const char *fmt, ...) {
    UASSERT(arena);
    UASSERT(fmt);

    va_list args;
    va_start(args, fmt);
    size_t len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    da_reserve(arena, len + 1);

    va_start(args, fmt);
    vsnprintf(arena->items + arena->count, len + 1, fmt, args);
    va_end(args);

    arena->count += len;
}

char *ustrbuild_end(uarena_t *arena) {
    da_resize(arena, arena->count + 1);
    arena->items[arena->count] = '\0';
    return arena->items;
}

const char *
cargs_help(cargs_t context, const char *name)
{
    UASSERT(context);
    ctx_t *ctx = (ctx_t *)context;

    uarena_t helpmsg;
    da_init(&helpmsg, 4096);

    size_t nw = ctx->namemaxlen;
    size_t hw = ctx->helpmaxlen;

    ustrbuild_printf(&helpmsg, "Usage: %s [FLAGS] [OPTIONS] command\n", name);
    ustrbuild_printf(&helpmsg, "\n");
    ustrbuild_printf(&helpmsg, "Options:\n");

    size_t len = 0;
    for (int i = 0; i < ctx->optlist.count; i++) {
        ustrbuild_printf(&helpmsg, "   %*s   ", nw, ctx->optlist.items[i].name);
        ustrbuild_printf(&helpmsg, "%s", ctx->optlist.items[i].help);
        if (i < ctx->optlist.count - 1)
            da_append(&helpmsg, '\n');
    }

    // leak help string
    return ustrbuild_end(&helpmsg);
}

void
cargs_parse(cargs_t context, const char *name, int argc, char **argv)
{
    UASSERT(context);
    ctx_t *ctx = (ctx_t *)context;

    opt_t *o = &ctx->optlist.items[0];

    // parse_chain

}
