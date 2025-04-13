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
    ustr_builder_t errorlog;
    int namemaxlen;
    int helpmaxlen;
} ctx_t;

static bool newopt(ctx_t *ctx, const char *name, const char *help, void *ptr, void *ptrlen, uintptr_t def, dtype_t dtype);
static bool parse_chain(ctx_t *ctx, const char *chain, uslicelist_t *list);

static int parse_opt_flag(ctx_t *ctx, opt_t *opt, char *arg);
static int parse_opt_int(ctx_t *ctx, opt_t *opt, char *arg, char *nextarg);
static int parse_opt_float(ctx_t *ctx, opt_t *opt, char *arg, char *nextarg);
static int parse_opt_str(ctx_t *ctx, opt_t *opt, char *arg, char *nextarg);

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
        ustr_builder_printf(&ctx->errorlog, "Flag '%s' already exists\n", name);
        return true;
    }

    opt_t opt;
    char *s;

    // copy name to arena
    s = da_endptr(&ctx->arena);
    opt.namelen = strlen(name);
    da_append_many(&ctx->arena, name, opt.namelen + 1);
    s[opt.namelen] = '\0';
    opt.name = s;

    if (ctx->namemaxlen < opt.namelen)
        ctx->namemaxlen = opt.namelen;

    // copy help to &ctx->arena
    s = da_endptr(&ctx->arena);
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

    return false;
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
parse_chain(ctx_t *ctx, const char *chain, uslicelist_t *list)
{
    da_init(list, 1);
    size_t len = strlen(chain);
    size_t i = 0;
    size_t n = 0;
    for (;;) {
        int l = match_ident(chain + i);

        // match error
        if (l < 0) {
            ustr_builder_printf(&ctx->errorlog, "Invalid char in chain\n");
            ustr_builder_printf(&ctx->errorlog, "%s\n", chain);
            ustr_builder_printf(&ctx->errorlog, "%*s\n", i - l, "^");
            da_delete(list);
            return true;
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
            ustr_builder_printf(&ctx->errorlog, "Trailing period in chain\n");
            ustr_builder_printf(&ctx->errorlog, "%s\n", chain);
            ustr_builder_printf(&ctx->errorlog, "%*s\n", i+1, "^");
            da_delete(list);
            return true;
        }
    }

    da_resize(list, list->count);
    return false;
}

void
cargs_init(cargs_t *context)
{
    UASSERT(context);
    ctx_t *ctx = umalloc(sizeof(ctx_t));
    da_init(&ctx->arena, 4096);
    da_init(&ctx->optlist, 1);
    ustr_builder_alloc(&ctx->errorlog);
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
    if (ctx->errorlog.items)
        ustr_builder_free(&ctx->errorlog);
    free(ctx);
    *context = (cargs_t)NULL;
}

const char *
cargs_error(cargs_t context)
{
    UASSERT(context);
    ctx_t *ctx = (ctx_t *)context;
    if (*da_last_item(&ctx->errorlog) == '\n')
        da_pop(&ctx->errorlog);
    return ustr_builder_terminate(&ctx->errorlog);
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
        ustr_builder_printf(&helpmsg, "   %-*s   %s", nw, ctx->optlist.items[i].name, ctx->optlist.items[i].help);
        if (i < ctx->optlist.count - 1)
            da_append(&helpmsg, '\n');
    }

    return ustr_builder_leak(&helpmsg);
}

int
parse_opt_flag(ctx_t *ctx, opt_t *opt, char *arg)
{
    UASSERT(opt);
    UASSERT(arg);

    if (opt->namelen == strlen(arg)) {
        *(bool *)opt->ptr = true;
        return 1;
    } else {
        ustr_builder_printf(&ctx->errorlog, "Flag doesn't match (%s) (%s)\n", opt->name, arg);
        return -1;
    }
}

int
parse_opt_int(ctx_t *ctx, opt_t *opt, char *arg, char *nextarg)
{
    UASSERT(opt);
    UASSERT(arg);

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
            rc = 1;
        }

        // invalid character in string following flag
        else {
            ustr_builder_printf(&ctx->errorlog, "Invalid character for integer flag\n");
            ustr_builder_printf(&ctx->errorlog, "%s\n", arg);
            ustr_builder_printf(&ctx->errorlog, "%*s\n", end - arg + 1, "^");
            rc = -1;
        }
    }

    // missing operand
    else if (nextarg == NULL) {
        ustr_builder_printf(&ctx->errorlog, "Missing operand for flag '%s'\n", arg);
        rc = -1;
    }

    // check nextarg
    else {
        char *end;
        val = strtol(nextarg, &end, 10);

        if (end - nextarg != strlen(nextarg)) {
            ustr_builder_printf(&ctx->errorlog, "Invalid character for integer flag '%s'\n", arg);
            ustr_builder_printf(&ctx->errorlog, "%s\n", nextarg);
            ustr_builder_printf(&ctx->errorlog, "%*s\n", end - nextarg + 1, "^");
            rc = -1;
        } else {
            rc = 2;
        }
    }

    if (rc > 0)
        *(int *)opt->ptr = val;

    return rc;
}

int
parse_opt_float(ctx_t *ctx, opt_t *opt, char *arg, char *nextarg)
{
    UASSERT(opt);
    UASSERT(arg);

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
            rc = 1;
        }

        // invalid character in string following flag
        else {
            ustr_builder_printf(&ctx->errorlog, "Invalid character for float flag\n");
            ustr_builder_printf(&ctx->errorlog, "%s\n", arg);
            ustr_builder_printf(&ctx->errorlog, "%*s\n", end - arg + 1, "^");
            rc = -1;
        }
    }

    // missing operand
    else if (nextarg == NULL) {
        ustr_builder_printf(&ctx->errorlog, "Missing operand for flag '%s'\n", arg);
        rc = -1;
    }

    // check nextarg
    else {
        char *end;
        val = strtof(nextarg, &end);

        if (end - nextarg != strlen(nextarg)) {
            ustr_builder_printf(&ctx->errorlog, "Invalid character for float flag '%s'\n", arg);
            ustr_builder_printf(&ctx->errorlog, "%s\n", nextarg);
            ustr_builder_printf(&ctx->errorlog, "%*s\n", end - nextarg + 1, "^");
            rc = -1;
        } else {
            rc = 2;
        }
    }

    if (rc > 0)
        *(float *)opt->ptr = val;

    return rc;
}

int
parse_opt_str(ctx_t *ctx, opt_t *opt, char *arg, char *nextarg)
{
    UASSERT(opt);
    UASSERT(arg);

    size_t arglen = strlen(arg);
    int rc;
    char *s;

    if (opt->namelen < arglen) {

        s = (arg[opt->namelen] == '=')
            ? arg + opt->namelen + 1
            : arg + opt->namelen;

        rc = 1;
    }

    // missing operand
    else if (nextarg == NULL) {
        ustr_builder_printf(&ctx->errorlog, "Missing operand for flag '%s'\n", opt->name);
        rc = -1;
    }

    // check nextarg
    else {
        s = nextarg;
        rc = 2;
    }

    if (rc > 0)
        *(char **)opt->ptr = s;

    return rc;
}

bool
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
            ustr_builder_printf(&ctx->errorlog, "Unknown flag '%s'\n", arg);
            return true;
        }

        opt_t *opt = &ctx->optlist.items[optidx];

        if (opt->processed) {
            ustr_builder_printf(&ctx->errorlog, "Duplicate flag '%s'\n", opt->name);
            return true;
        }

        int n;

        switch (opt->dtype) {
        case CARGS_BOOL: n = parse_opt_flag(ctx, opt, arg); break;
        case CARGS_INT: n = parse_opt_int(ctx, opt, arg, nextarg); break;
        case CARGS_FLOAT: n = parse_opt_float(ctx, opt, arg, nextarg); break;
        case CARGS_STR: n = parse_opt_str(ctx, opt, arg, nextarg); break;
        case CARGS_CHAIN:
        default:
            n = -1;
            ustr_builder_printf(&ctx->errorlog, "unimplemented arg dtype '%d'\n", opt->dtype);
            break;
        }

        // error
        if (n < 0)
            return true;

        opt->processed = true;
        i += n;
    }

    for (int i = 0; i < ctx->optlist.count; i++) {
        opt_t *opt = &ctx->optlist.items[i];

        if (opt->processed)
            continue;

        switch (opt->dtype) {
        case CARGS_BOOL:
            *(bool *)opt->ptr = (bool)opt->def;
            break;
        case CARGS_INT:
            *(int *)opt->ptr = (int)opt->def;
            break;
        case CARGS_FLOAT:
            *(float *)opt->ptr = (float)opt->def;
            break;
        case CARGS_STR:
            *(char **)opt->ptr = (char *)opt->def;
            break;
        case CARGS_CHAIN:
        default:
            break;
        }
    }

    return false;
}
