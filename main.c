#include <stdio.h>
#include <stdbool.h>

#include "cargs.h"

#define UTIL_IMPL
#include "util.h"


int
main(int argc, char *argv[])
{
    // if (argc != 2) {
    //     ulog(UFATL, "invalid argc");
    //     ulog(UINFO, "usage: %s command.chain", argv[0]);
    //     exit(1);
    // }

    int i;
    bool b;

    cargs_t cargs;

    cargs_init(&cargs);

    cargs_add_opt_flag(cargs, &b, "b0", "zero");
    cargs_add_opt_flag(cargs, &b, "b1", "one");
    cargs_add_opt_flag(cargs, &b, "b2", "two");
    cargs_add_opt_flag(cargs, &b, "b3", "three");

    ulog(UNONE, "%s", cargs_help(cargs, argv[0]));

    // cargs_parse(cargs, argc, argv);

    cargs_delete(&cargs);

    return 0;
}

