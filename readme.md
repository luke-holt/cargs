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
```

Auto generated help message.
```
$ ./carg-test -h
Usage: ./carg-test [FLAGS] [OPTIONS] command

Options:
   -h   print help message
   -b   boolean switch 'b'
   -i   integer option
   -f   float option
   -s   string option
```

Valid inputs.
```
$ ./carg-test -b -i 10 -f=1.5 -s "hello"
[INFO] flag 'h': false
[INFO] flag 'b': true
[INFO] int 'i': 10
[INFO] float 'f': 1.500000
[INFO] string 's': hello
```

Error reporting.
```
$ ./carg-test -i
Missing operand for flag '-i'
```
```
$ ./carg-test -f notafloat
Invalid character for float flag '-f'
notafloat
^
```
```
$ ./carg-test -x
Unknown flag '-x'
```

