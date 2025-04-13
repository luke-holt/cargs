#ifndef CARGS_H
#define CARGS_H

#include <stdbool.h>
#include <stdint.h>

typedef uintptr_t cargs_t;

void cargs_init(cargs_t *context);
void cargs_delete(cargs_t *context);


// returns true if error else returns false
bool cargs_parse(cargs_t context, const char *name, int argc, char **argv);

const char *cargs_help(cargs_t context, const char *name);
const char *cargs_error(cargs_t context);

bool cargs_add_opt_flag(cargs_t context, bool *v, bool def, const char *name, const char *help);
bool cargs_add_opt_int(cargs_t context, int *v, int def, const char *name, const char *help);
bool cargs_add_opt_float(cargs_t context, float *v, float def, const char *name, const char *help);
bool cargs_add_opt_str(cargs_t context, char **v, const char *def, const char *name, const char *help);

// void cargs_add_opt_chain(cargs_t *context, char **v, int *vlen, const char *name, const char *help);

#endif // CARGS_H
