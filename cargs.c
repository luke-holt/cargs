#include <stdio.h>

#include "cargs.h"

#include "util.h"

#ifndef USTR_IMPL
#define USTR_IMPL
#endif
#include "ustr.h"

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
    uintptr_t def;
    dtype_t dtype;
    int namelen;
    int helplen;
    bool processed;
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

static bool newopt(ctx_t *ctx, const char *name, const char *help, void *ptr, void *ptrlen, uintptr_t def, dtype_t dtype);
static bool parse_chain(const char *chain, uslicelist_t *list);

static int parse_opt_flag(opt_t *opt, char *arg);
static int parse_opt_int(opt_t *opt, char *arg, char *nextarg);
static int parse_opt_float(opt_t *opt, char *arg, char *nextarg);
static int parse_opt_str(opt_t *opt, char *arg, char *nextarg);

static int optlist_best_match_name(optlist_t *list, const char *name);

int
optlist_best_match_name(optlist_t *list, const char *name)
{
    int best_match = -1;
    int len = 0;
    for (int i = 0; i < list->count; i++) {
        if (0 == strncmp(list->items[i].name, name, list->items[i].namelen)) {
            if (len < list->items[i].namelen) {
                len = list->items[i].namelen;
                best_match = i;
            }
        }
    }
    return best_match;
}

bool
newopt(ctx_t *ctx, const char *name, const char *help, void *ptr, void *ptrlen, uintptr_t def, dtype_t dtype)
{
    UASSERT(ctx);
    UASSERT(name);
    UASSERT(help);
    UASSERT(ptr);

    int idx = optlist_best_match_name(&ctx->optlist, name);
    if ((idx >= 0) && (0 == strcmp(name, ctx->optlist.items[idx].name))) {
        ulog(UWARN, "flag '%s' already exists", name);
        return false;
    }

    opt_t opt;
    char *s;

    // copy name to arena
    s = da_tail(&ctx->arena);
    opt.namelen = strlen(name);
    da_append_many(&ctx->arena, name, opt.namelen + 1);
    s[opt.namelen] = '\0';
    opt.name = s;

    if (ctx->namemaxlen < opt.namelen)
        ctx->namemaxlen = opt.namelen;

    // copy help to &ctx->arena
    s = da_tail(&ctx->arena);
    opt.helplen = strlen(help);
    da_append_many(&ctx->arena, help, opt.helplen + 1);
    s[opt.helplen] = '\0';
    opt.help = s;

    if (ctx->helpmaxlen < opt.helplen)
        ctx->helpmaxlen = opt.helplen;

    opt.ptr = ptr;
    opt.ptrlen = ptrlen;
    opt.dtype = dtype;
    opt.processed = false;
    opt.def = def;

    da_append(&ctx->optlist, opt);

    return true;
}


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

bool
cargs_add_opt_flag(cargs_t context, bool *v, bool def, const char *name, const char *help)
{
    UASSERT(context);
    return newopt((ctx_t *)context, name, help, (void *)v, NULL, (uintptr_t)def, CARGS_BOOL);
}

bool
cargs_add_opt_int(cargs_t context, int *v, int def, const char *name, const char *help)
{
    UASSERT(context);
    return newopt((ctx_t *)context, name, help, (void *)v, NULL, (uintptr_t)def, CARGS_INT);
}

bool
cargs_add_opt_float(cargs_t context, float *v, float def, const char *name, const char *help)
{
    UASSERT(context);
    return newopt((ctx_t *)context, name, help, (void *)v, NULL, (uintptr_t)def, CARGS_FLOAT);
}

bool
cargs_add_opt_str(cargs_t context, char **v, const char *def, const char *name, const char *help)
{
    UASSERT(context);
    return newopt((ctx_t *)context, name, help, (void *)v, NULL, (uintptr_t)def, CARGS_STR);
}

// void
// cargs_add_opt_chain(cargs_t context, char **v, int *vlen, const char *name, const char *help)
// {
// }

const char *
cargs_help(cargs_t context, const char *name)
{
    UASSERT(context);
    ctx_t *ctx = (ctx_t *)context;

    ustr_builder_t helpmsg;
    ustr_builder_alloc(&helpmsg);

    size_t nw = ctx->namemaxlen;
    size_t hw = ctx->helpmaxlen;

    ustr_builder_printf(&helpmsg, "Usage: %s [FLAGS] [OPTIONS] command\n\nOptions:\n", name);

    size_t len = 0;
    for (int i = 0; i < ctx->optlist.count; i++) {
        ustr_builder_printf(&helpmsg, "   %-*s   ", nw, ctx->optlist.items[i].name);
        ustr_builder_printf(&helpmsg, "%s", ctx->optlist.items[i].help);
        if (i < ctx->optlist.count - 1)
            da_append(&helpmsg, '\n');
    }

    return ustr_builder_leak(&helpmsg);
}

int
parse_opt_flag(opt_t *opt, char *arg)
{
    UASSERT(opt);
    UASSERT(arg);

    if (opt->namelen == strlen(arg)) {
        *(bool *)opt->ptr = true;
        ulog(UINFO, "found flag '%s'", opt->name);
        return 1;
    } else {
        *(bool *)opt->ptr = (bool)opt->def;
        ulog(UWARN, "flag doesn't match (%s) (%s)", opt->name, arg);
        return -1;
    }
}

int
parse_opt_int(opt_t *opt, char *arg, char *nextarg)
{
    UASSERT(opt);
    UASSERT(arg);
    UASSERT(nextarg);

    size_t arglen = strlen(arg);
    int rc;
    long val;

    if (opt->namelen < arglen) {
        char *end;

        val = (arg[opt->namelen] == '=')
            ? strtol(arg + opt->namelen + 1, &end, 10)
            : strtol(arg + opt->namelen, &end, 10);

        // valid integer following flag
        if (end - arg == arglen) {
            ulog(UINFO, "found value '%li' for flag '%s'", val, opt->name);
            rc = 1;
        }

        // invalid character in string following flag
        else {
            ulog(UWARN, "invalid character for integer flag");
            ulog(UWARN, "%s", arg);
            ulog(UWARN, "%*s", end - arg + 1, "^");
            rc = -1;
        }
    }

    // missing operand
    else if (nextarg == NULL) {
        ulog(UWARN, "missing operand for flag '%s'", arg);
        rc = -1;
    }

    // check nextarg
    else {
        char *end;
        val = strtol(nextarg, &end, 10);

        if (end - nextarg != strlen(nextarg)) {
            ulog(UWARN, "invalid character for integer flag '%s'", arg);
            ulog(UWARN, "%s", nextarg);
            ulog(UWARN, "%*s", end - nextarg + 1, "^");
            rc = -1;
        } else {
            ulog(UINFO, "found value '%li' for flag '%s'", val, opt->name);
            rc = 2;
        }
    }

    *(int *)opt->ptr = (rc < 0) ? (int)opt->def : val;

    return rc;
}

int
parse_opt_float(opt_t *opt, char *arg, char *nextarg)
{
    UASSERT(opt);
    UASSERT(arg);
    UASSERT(nextarg);

    size_t arglen = strlen(arg);
    int rc;
    float val;

    if (opt->namelen < arglen) {
        char *end;

        val = (arg[opt->namelen] == '=')
            ? strtof(arg + opt->namelen + 1, &end)
            : strtof(arg + opt->namelen, &end);

        // valid float following flag
        if (end - arg == arglen) {
            ulog(UINFO, "found value '%f' for flag '%s'", val, opt->name);
            rc = 1;
        }

        // invalid character in string following flag
        else {
            ulog(UWARN, "invalid character for float flag");
            ulog(UWARN, "%s", arg);
            ulog(UWARN, "%*s", end - arg + 1, "^");
            rc = -1;
        }
    }

    // missing operand
    else if (nextarg == NULL) {
        ulog(UWARN, "missing operand for flag '%s'", arg);
        rc = -1;
    }

    // check nextarg
    else {
        char *end;
        val = strtof(nextarg, &end);

        if (end - nextarg != strlen(nextarg)) {
            ulog(UWARN, "invalid character for float flag '%s'", arg);
            ulog(UWARN, "%s", nextarg);
            ulog(UWARN, "%*s", end - nextarg + 1, "^");
            rc = -1;
        } else {
            ulog(UINFO, "found value '%e' for flag '%s'", val, opt->name);
            rc = 2;
        }
    }

    *(float *)opt->ptr = (rc < 0) ? (float)opt->def : val;

    return rc;
}

int
parse_opt_str(opt_t *opt, char *arg, char *nextarg)
{
    UASSERT(opt);
    UASSERT(arg);
    UASSERT(nextarg);

    size_t arglen = strlen(arg);
    int rc;
    char *s;

    if (opt->namelen < arglen) {

        s = (arg[opt->namelen] == '=')
            ? arg + opt->namelen + 1
            : arg + opt->namelen;

        ulog(UINFO, "found string '%s' for flag '%s'", s, opt->name);

        rc = 1;
    }

    // missing operand
    else if (nextarg == NULL) {
        ulog(UWARN, "missing operand for flag '%s'", opt->name);
        rc = -1;
    }

    // check nextarg
    else {
        s = nextarg;
        ulog(UINFO, "found string '%s' for flag '%s'", s, opt->name);
        rc = 2;
    }

    *(char **)opt->ptr = (rc < 0) ? (char *)opt->def : s;

    return rc;
}

void
cargs_parse(cargs_t context, const char *name, int argc, char **argv)
{
    UASSERT(context);
    ctx_t *ctx = (ctx_t *)context;

    // parse optional flags
    for (int i = 0; i < argc; ) {
        char *arg = argv[i];
        char *nextarg = ((i + 1) < argc) ? argv[i+1] : NULL;

        int optidx = optlist_best_match_name(&ctx->optlist, arg);
        if (optidx < 0) {
            ulog(UWARN, "unable to match option '%s'", arg);
            i++;
            continue;
        }

        opt_t *opt = &ctx->optlist.items[optidx];

        if (opt->processed)
            ulog(UWARN, "already processed flag '%s'", opt->name);

        int n;

        switch (opt->dtype) {
        case CARGS_BOOL: n = parse_opt_flag(opt, arg); break;
        case CARGS_INT: n = parse_opt_int(opt, arg, nextarg); break;
        case CARGS_FLOAT: n = parse_opt_float(opt, arg, nextarg); break;
        case CARGS_STR: n = parse_opt_str(opt, arg, nextarg); break;
        case CARGS_CHAIN:
        default:
            n = -1;
            ulog(UWARN, "unimplemented arg dtype '%d'", opt->dtype);
            break;
        }

        // error
        if (n < 0) {
            ulog(UWARN, "parse error for arg dtype '%s' '%d'", arg, opt->dtype);
            i++;
            continue;
        } else {
            opt->processed = true;
        }

        i += n;
    }
}
