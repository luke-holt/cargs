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
cargs_add_opt_list(cargs, &list, &listlen, '.', "-l", "string list, delimited by '.'");
cargs_add_opt_list(cargs, &csv, &csvlen, ',', "--csv", "comma-separated values");

const char *helpmsg = cargs_help(cargs, argv[0]);

bool err = cargs_parse(cargs, argv[0], --argc, &argv[1]);
```

Auto generated help message.
```
$ ./carg-test -h
Usage: ./carg-test [OPTIONS] command

Options:
   -h      print help message
   -b      boolean switch 'b'
   -i      integer option
   -f      float option
   -s      string option
   -l      string list, delimited by '.'
   --csv   comma-separated values
```

Valid inputs.
```
$ ./carg-test -b -i10 -f 12.5 -s "hello!" -l this.is.a.string.list --csv a,b,c,d,e
Parsed cli arguments:
  -h: false
  -b: true
  -i: 10
  -f: 12.500000
  -s: hello!
  -l: len=5
    0: this
    1: is
    2: a
    3: string
    4: list
  --csv: len=5
    0: a
    1: b
    2: c
    3: d
    4: e
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

