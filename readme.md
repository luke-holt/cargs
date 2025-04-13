# Overview
Easy to integrate command line argument parsing. Simply provide the options and commands to parse and collect the input values.

# Sample Usage
Example code.
```C
cargs_t cargs;
cargs_init(&cargs);

cargs_add_opt_flag(cargs, &help, false, "-h", "print help message");
cargs_add_opt_flag(cargs, &b, false, "-b", "boolean switch 'b'");
cargs_add_opt_int(cargs, &i, 69, "-i", "integer option");
cargs_add_opt_float(cargs, &f, 123.321, "-f", "float option");
cargs_add_opt_str(cargs, &s, "default string", "-s", "string option");

const char *helpmsg = cargs_help(cargs, argv[0]);

cargs_parse(cargs, argv[0], --argc, &argv[1]);

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
```

Example output.
```
$ ./carg-test -b -i 10 -f=123.45 -s "hello, world!"
[INFO] found flag '-b'
[INFO] found value '10' for flag '-i'
[INFO] found value '123.449997' for flag '-f'
[INFO] found string 'hello, world!' for flag '-s'
[INFO] flag 'h': false
[INFO] flag 'b': true
[INFO] int 'i': 10
[INFO] float 'f': 123.449997
[INFO] string 's': hello, world!
```

## Help Message
The help message is generated automatically, here is an example.

```
$ ./cargs-test -h
Usage: ./carg-test [FLAGS] [OPTIONS] command

Options:
   -h   print help message
   -b   boolean switch 'b'
   -i   integer option
   -f   float option
   -s   string option
```
