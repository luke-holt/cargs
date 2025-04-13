#include <stdbool.h>

#include "cargs.h"

#define UTIL_IMPL
#include "util.h"

int
main(int argc, char *argv[])
{
    int i;
    bool b;
    bool help;
    float f;
    char *s;

    cargs_t cargs;
    cargs_init(&cargs);

    cargs_add_opt_flag(cargs, &help, false, "-h", "print help message");
    cargs_add_opt_flag(cargs, &b, false, "-b", "boolean switch 'b'");
    cargs_add_opt_int(cargs, &i, 69, "-i", "integer option");
    cargs_add_opt_float(cargs, &f, 123.321, "-f", "float option");
    cargs_add_opt_str(cargs, &s, "default string", "-s", "string option");

    const char *helpmsg = cargs_help(cargs, argv[0]);

    bool err = cargs_parse(cargs, argv[0], --argc, &argv[1]);
    if (err) {
        ulog(UNONE, "%s", cargs_error(cargs));
        exit(0);
    }

    cargs_delete(&cargs);

    if (help) {
        ulog(UNONE, "%s", helpmsg);
        exit(0);
    }

    ulog(UINFO, "flag 'h': %s", help?"true":"false");
    ulog(UINFO, "flag 'b': %s", b?"true":"false");
    ulog(UINFO, "int 'i': %d", i);
    ulog(UINFO, "float 'f': %f", f);
    ulog(UINFO, "string 's': %s", s);

    return 0;
}

