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
    char **list;
    int listlen;
    char **csv;
    int csvlen;

    cargs_t cargs;
    cargs_init(&cargs);

    cargs_add_opt_flag(cargs, &help, false, "-h", "print help message");
    cargs_add_opt_flag(cargs, &b, false, "-b", "boolean switch 'b'");
    cargs_add_opt_int(cargs, &i, 69, "-i", "integer option");
    cargs_add_opt_float(cargs, &f, 123.321, "-f", "float option");
    cargs_add_opt_str(cargs, &s, "default string", "-s", "string option");
    cargs_add_opt_list(cargs, &list, &listlen, '.', "-l", "string list, delimited by '.'");
    cargs_add_opt_list(cargs, &csv, &csvlen, ',', "--csv", "comma-separated values");

    const char *helpmsg = cargs_help(cargs, argv[0]);

    bool err = cargs_parse(cargs, argv[0], --argc, &argv[1]);
    if (err) {
        printf("%s\n", cargs_error(cargs));
        exit(0);
    }

    if (help) {
        printf("%s\n", helpmsg);
        exit(0);
    }

    printf("Parsed cli arguments:\n");
    printf("  -h: %s\n", help?"true":"false");
    printf("  -b: %s\n", b?"true":"false");
    printf("  -i: %d\n", i);
    printf("  -f: %f\n", f);
    printf("  -s: %s\n", s);
    printf("  -l: len=%d\n", listlen);
    for (int i = 0; i < listlen; i++)
        printf("    %d: %s\n", i, list[i]);
    printf("  --csv: len=%d\n", csvlen);
    for (int i = 0; i < csvlen; i++)
        printf("    %d: %s\n", i, csv[i]);

    cargs_delete(&cargs);

    return 0;
}

