#include <stdio.h>
#include <stdbool.h>

#define UTIL_IMPL
#include "util.h"

static inline bool isletter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline bool isnumber(char c) { return c >= '0' && c <= '9'; }
static inline bool isuscore(char c) { return c == '_'; }
static inline bool isperiod(char c) { return c == '.'; }
static inline bool iseos(char c) { return c == '\0'; }

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

bool cargs_chain(const char *chain, uslicelist_t *list);

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        ulog(UFATL, "invalid argc");
        ulog(UINFO, "usage: %s command.chain", argv[0]);
        exit(1);
    }

    ulog(UINFO, "parsing command chain: \"%s\"", argv[1]);
    uslicelist_t list;
    if (cargs_chain(argv[1], &list)) {

        for (int i = 0; i < list.count; i++) {
            ulog(UINFO, "  %.*s", list.items[i].len, list.items[i].str);
        }

        da_delete(&list);
    }

    return 0;
}

bool
cargs_chain(const char *chain, uslicelist_t *list)
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

