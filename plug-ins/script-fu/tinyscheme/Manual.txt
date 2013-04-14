

                       TinySCHEME Version 1.41

                    "Safe if used as prescribed"
                    -- Philip K. Dick, "Ubik"

This software is open source, covered by a BSD-style license.
Please read accompanying file COPYING.
-------------------------------------------------------------------------------

     This Scheme interpreter is based on MiniSCHEME version 0.85k4
     (see miniscm.tar.gz in the Scheme Repository)
     Original credits in file MiniSCHEMETribute.txt.

     D. Souflis (dsouflis@acm.org)

-------------------------------------------------------------------------------
     What is TinyScheme?
     -------------------

     TinyScheme is a lightweight Scheme interpreter that implements as large
     a subset of R5RS as was possible without getting very large and
     complicated. It is meant to be used as an embedded scripting interpreter
     for other programs. As such, it does not offer IDEs or extensive toolkits
     although it does sport a small top-level loop, included conditionally.
     A lot of functionality in TinyScheme is included conditionally, to allow
     developers freedom in balancing features and footprint.

     As an embedded interpreter, it allows multiple interpreter states to
     coexist in the same program, without any interference between them.
     Programmatically, foreign functions in C can be added and values
     can be defined in the Scheme environment. Being a quite small program,
     it is easy to comprehend, get to grips with, and use.

     Known bugs
     ----------

     TinyScheme is known to misbehave when memory is exhausted.


     Things that keep missing, or that need fixing
     ---------------------------------------------

     There are no hygienic macros. No rational or
     complex numbers. No unwind-protect and call-with-values.

     Maybe (a subset of) SLIB will work with TinySCHEME...

     Decent debugging facilities are missing. Only tracing is supported
     natively.


     Scheme Reference
     ----------------

     If something seems to be missing, please refer to the code and
     "init.scm", since some are library functions.  Refer to the MiniSCHEME
     readme as a last resort.

          Environments
     (interaction-environment)
     See R5RS. In TinySCHEME, immutable list of association lists.

     (current-environment)
     The environment in effect at the time of the call. An example of its
     use and its utility can be found in the sample code that implements
     packages in "init.scm":

          (macro (package form)
               `(apply (lambda ()
                         ,@(cdr form)
                         (current-environment))))

     The environment containing the (local) definitions inside the closure
     is returned as an immutable value.

     (defined? <symbol>) (defined? <symbol> <environment>)
     Checks whether the given symbol is defined in the current (or given)
     environment.

          Symbols
     (gensym)
     Returns a new interned symbol each time. Will probably move to the
     library when string->symbol is implemented.

          Directives
     (gc)
     Performs garbage collection immediatelly.

     (gcverbose) (gcverbose <bool>)
     The argument (defaulting to #t) controls whether GC produces
     visible outcome.

     (quit) (quit <num>)
     Stops the interpreter and sets the 'retcode' internal field (defaults
     to 0). When standalone, 'retcode' is returned as exit code to the OS.

     (tracing <num>)
     1, turns on tracing. 0 turns it off. (Only when USE_TRACING is 1).

          Mathematical functions
     Since rationals and complexes are absent, the respective functions
     are also missing.
     Supported: exp, log, sin, cos, tan, asin, acos, atan, floor, ceiling,
     trunc, round and also sqrt and expt when USE_MATH=1.
     Number-theoretical quotient, remainder and modulo, gcd, lcm.
     Library: exact?, inexact?, odd?, even?, zero?, positive?, negative?,
     exact->inexact. inexact->exact is a core function.

          Type predicates
     boolean?,eof-object?,symbol?,number?,string?,integer?,real?,list?,null?,
     char?,port?,input-port?,output-port?,procedure?,pair?,environment?',
     vector?. Also closure?, macro?.

          Types
     Types supported:

          Numbers (integers and reals)
          Symbols
          Pairs
          Strings
          Characters
          Ports
          Eof object
          Environments
          Vectors

          Literals
     String literals can contain escaped quotes \" as usual, but also
     \n, \r, \t, \xDD (hex representations) and \DDD (octal representations).
     Note also that it is possible to include literal newlines in string
     literals, e.g.

          (define s "String with newline here
          and here
          that can function like a HERE-string")

     Character literals contain #\space and #\newline and are supplemented
     with #\return and #\tab, with obvious meanings. Hex character
     representations are allowed (e.g. #\x20 is #\space).
     When USE_ASCII_NAMES is defined, various control characters can be
     referred to by their ASCII name.
     0	     #\nul	       17       #\dc1
     1	     #\soh             18       #\dc2
     2	     #\stx             19       #\dc3
     3	     #\etx             20       #\dc4
     4	     #\eot             21       #\nak
     5	     #\enq             22       #\syn
     6	     #\ack             23       #\etv
     7	     #\bel             24       #\can
     8	     #\bs              25       #\em
     9	     #\ht              26       #\sub
     10	     #\lf              27       #\esc
     11	     #\vt              28       #\fs
     12	     #\ff              29       #\gs
     13	     #\cr              30       #\rs
     14	     #\so              31       #\us
     15	     #\si
     16	     #\dle             127      #\del 		

     Numeric literals support #x #o #b and #d. Flonums are currently read only
     in decimal notation. Full grammar will be supported soon.

          Quote, quasiquote etc.
     As usual.

          Immutable values
     Immutable pairs cannot be modified by set-car! and set-cdr!.
     Immutable strings cannot be modified via string-set!

          I/O
     As per R5RS, plus String Ports (see below).
     current-input-port, current-output-port,
     close-input-port, close-output-port, input-port?, output-port?,
     open-input-file, open-output-file.
     read, write, display, newline, write-char, read-char, peek-char.
     char-ready? returns #t only for string ports, because there is no
     portable way in stdio to determine if a character is available.
     Also open-input-output-file, set-input-port, set-output-port (not R5RS)
     Library: call-with-input-file, call-with-output-file,
     with-input-from-file, with-output-from-file and
     with-input-output-from-to-files, close-port and input-output-port?
     (not R5RS).
     String Ports: open-input-string, open-output-string, get-output-string,
     open-input-output-string. Strings can be used with I/O routines.

          Vectors
     make-vector, vector, vector-length, vector-ref, vector-set!, list->vector,
     vector-fill!, vector->list, vector-equal? (auxiliary function, not R5RS)

          Strings
     string, make-string, list->string, string-length, string-ref, string-set!,
     substring, string->list, string-fill!, string-append, string-copy.
     string=?, string<?, string>?, string>?, string<=?, string>=?.
     (No string-ci*? yet). string->number, number->string. Also atom->string,
     string->atom (not R5RS).

          Symbols
     symbol->string, string->symbol

          Characters
     integer->char, char->integer.
     char=?, char<?, char>?, char<=?, char>=?.
     (No char-ci*?)

          Pairs & Lists
     cons, car, cdr, list, length, map, for-each, foldr, list-tail,
     list-ref, last-pair, reverse, append.
     Also member, memq, memv, based on generic-member, assoc, assq, assv
     based on generic-assoc.

          Streams
     head, tail, cons-stream

          Control features
     Apart from procedure?, also macro? and closure?
     map, for-each, force, delay, call-with-current-continuation (or call/cc),
     eval, apply. 'Forcing' a value that is not a promise produces the value.
     There is no call-with-values, values, nor dynamic-wind. Dynamic-wind in
     the presence of continuations would require support from the abstract
     machine itself.

          Property lists
     TinyScheme inherited from MiniScheme property lists for symbols.
     put, get.

          Dynamically-loaded extensions
     (load-extension <filename without extension>)
     Loads a DLL declaring foreign procedures. On Unix/Linux, one can make use
     of the ld.so.conf file or the LD_RUN_PATH system variable in order to place
     the library in a directory other than the current one. Please refer to the
     appropriate 'man' page.

          Esoteric procedures
     (oblist)
     Returns the oblist, an immutable list of all the symbols.

     (macro-expand <form>)
     Returns the expanded form of the macro call denoted by the argument

     (define-with-return (<procname> <args>...) <body>)
     Like plain 'define', but makes the continuation available as 'return'
     inside the procedure. Handy for imperative programs.

     (new-segment <num>)
     Allocates more memory segments.

     defined?
     See "Environments"

     (get-closure-code <closure>)
     Gets the code as scheme data.

     (make-closure <code> <environment>)
     Makes a new closure in the given environment.

          Obsolete procedures
     (print-width <object>)

     Programmer's Reference
     ----------------------

     The interpreter state is initialized with "scheme_init".
     Custom memory allocation routines can be installed with an alternate
     initialization function: "scheme_init_custom_alloc".
     Files can be loaded with "scheme_load_file". Strings containing Scheme
     code can be loaded with "scheme_load_string". It is a good idea to
     "scheme_load" init.scm before anything else.

     External data for keeping external state (of use to foreign functions)
     can be installed with "scheme_set_external_data".
     Foreign functions are installed with "assign_foreign". Additional
     definitions can be added to the interpreter state, with "scheme_define"
     (this is the way HTTP header data and HTML form data are passed to the
     Scheme script in the Altera SQL Server). If you wish to define the
     foreign function in a specific environment (to enhance modularity),
     use "assign_foreign_env".

     The procedure "scheme_apply0" has been added with persistent scripts in
     mind. Persistent scripts are loaded once, and every time they are needed
     to produce HTTP output, appropriate data are passed through global
     definitions and function "main" is called to do the job. One could
     add easily "scheme_apply1" etc.

     The interpreter state should be deinitialized with "scheme_deinit".

     DLLs containing foreign functions should define a function named
     init_<base-name>. E.g. foo.dll should define init_foo, and bar.so
     should define init_bar. This function should assign_foreign any foreign
     function contained in the DLL.

     The first dynamically loaded extension available for TinyScheme is
     a regular expression library. Although it's by no means an
     established standard, this library is supposed to be installed in
     a directory mirroring its name under the TinyScheme location.


     Foreign Functions
     -----------------

     The user can add foreign functions in C. For example, a function
     that squares its argument:

          pointer square(scheme *sc, pointer args) {
           if(args!=sc->NIL) {
               if(sc->isnumber(sc->pair_car(args))) {
                    double v=sc->rvalue(sc->pair_car(args));
                    return sc->mk_real(sc,v*v);
               }
           }
           return sc->NIL;
          }

   Foreign functions are now defined as closures:

   sc->interface->scheme_define(
        sc,
        sc->global_env,
        sc->interface->mk_symbol(sc,"square"),
        sc->interface->mk_foreign_func(sc, square));


     Foreign functions can use the external data in the "scheme" struct
     to implement any kind of external state.

     External data are set with the following function:
          void scheme_set_external_data(scheme *sc, void *p);

     As of v.1.17, the canonical way for a foreign function in a DLL to
     manipulate Scheme data is using the function pointers in sc->interface.

     Standalone
     ----------

     Usage: tinyscheme -?
     or:    tinyscheme [<file1> <file2> ...]
     followed by
	       -1 <file> [<arg1> <arg2> ...]
	       -c <Scheme commands> [<arg1> <arg2> ...]
     assuming that the executable is named tinyscheme.

     Use - in the place of a filename to denote stdin.
     The -1 flag is meant for #! usage in shell scripts. If you specify
          #! /somewhere/tinyscheme -1
     then tinyscheme will be called to process the file. For example, the
     following script echoes the Scheme list of its arguments.

	       #! /somewhere/tinyscheme -1
	       (display *args*)

     The -c flag permits execution of arbitrary Scheme code.


     Error Handling
     --------------

     Errors are recovered from without damage. The user can install his
     own handler for system errors, by defining *error-hook*. Defining
     to '() gives the default behavior, which is equivalent to "error".
     USE_ERROR_HOOK must be defined.

     A simple exception handling mechanism can be found in "init.scm".
     A new syntactic form is introduced:

          (catch <expr returned exceptionally>
               <expr1> <expr2> ... <exprN>)

     "Catch" establishes a scope spanning multiple call-frames
     until another "catch" is encountered.

     Exceptions are thrown with:

          (throw "message")

     If used outside a (catch ...), reverts to (error "message").

     Example of use:

          (define (foo x) (write x) (newline) (/ x 0))

          (catch (begin (display "Error!\n") 0)
               (write "Before foo ... ")
               (foo 5)
               (write "After foo"))

     The exception mechanism can be used even by system errors, by

          (define *error-hook* throw)

     which makes use of the error hook described above.

     If necessary, the user can devise his own exception mechanism with
     tagged exceptions etc.


     Reader extensions
     -----------------

     When encountering an unknown character after '#', the user-specified
     procedure *sharp-hook* (if any), is called to read the expression.
     This can be used to extend the reader to handle user-defined constants
     or whatever. It should be a procedure without arguments, reading from
     the current input port (which will be the load-port).


     Colon Qualifiers - Packages
     ---------------------------

     When USE_COLON_HOOK=1:
     The lexer now recognizes the construction <qualifier>::<symbol> and
     transforms it in the following manner (T is the transformation function):

          T(<qualifier>::<symbol>) = (*colon-hook* 'T(<symbol>) <qualifier>)

     where <qualifier> is a symbol not containing any double-colons.

     As the definition is recursive, qualifiers can be nested.
     The user can define his own *colon-hook*, to handle qualified names.
     By default, "init.scm" defines *colon-hook* as EVAL. Consequently,
     the qualifier must denote a Scheme environment, such as one returned
     by (interaction-environment). "Init.scm" defines a new syntantic form,
     PACKAGE, as a simple example. It is used like this:

          (define toto
               (package
                    (define foo 1)
                    (define bar +)))

          foo                                     ==>  Error, "foo" undefined
          (eval 'foo)                             ==>  Error, "foo" undefined
          (eval 'foo toto)                        ==>  1
          toto::foo                               ==>  1
          ((eval 'bar toto) 2 (eval 'foo toto))   ==>  3
          (toto::bar 2 toto::foo)                 ==>  3
          (eval (bar 2 foo) toto)                 ==>  3

     If the user installs another package infrastructure, he must define
     a new 'package' procedure or macro to retain compatibility with supplied
     code.

     Note: Older versions used ':' as a qualifier. Unfortunately, the use
     of ':' as a pseudo-qualifier in existing code (i.e. SLIB) essentially
     precludes its use as a real qualifier.








