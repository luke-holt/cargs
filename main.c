#include <stdbool.h>

#include "cargs.h"

#define UTIL_IMPL
#include "util.h"

int
main(int argc, char *argv[])
{
    int i;
    bool b;
    bool blag;
    float f;
    char *s;

    cargs_t cargs;

    cargs_init(&cargs);

    cargs_add_opt_flag(cargs, &b, "-b", "flag");
    cargs_add_opt_flag(cargs, &blag, "-blag", "flag");
    cargs_add_opt_int(cargs, &i, "-i", "int");
    cargs_add_opt_float(cargs, &f, "-f", "float");
    cargs_add_opt_str(cargs, &s, "-s", "string");

    ulog(UNONE, "%s", cargs_help(cargs, argv[0]));

    cargs_parse(cargs, argv[0], --argc, &argv[1]);

    cargs_delete(&cargs);

    return 0;
}

