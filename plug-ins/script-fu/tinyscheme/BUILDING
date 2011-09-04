        Building TinyScheme
        -------------------

The included makefile includes logic for Linux, Solaris and Win32, and can
readily serve as an example for other OSes, especially Unixes. There are
a lot of compile-time flags in TinyScheme (preprocessor defines) that can trim
unwanted features. See next section. 'make all' and 'make clean' function as
expected.

Autoconfing TinyScheme was once proposed, but the distribution would not be
so small anymore. There are few platform dependencies in TinyScheme, and in
general compiles out of the box.

     Customizing
     -----------

     The following symbols are defined to default values in scheme.h.
     Use the -D flag of cc to set to either 1 or 0.

     STANDALONE
     Define this to produce a standalone interpreter.

     USE_MATH
     Includes math routines.

     USE_CHAR_CLASSIFIERS
     Includes character classifier procedures.

     USE_ASCII_NAMES
     Enable extended character notation based on ASCII names.

     USE_STRING_PORTS
     Enables string ports.

     USE_ERROR_HOOK
     To force system errors through user-defined error handling.
     (see "Error handling")

     USE_TRACING
     To enable use of TRACING.

     USE_COLON_HOOK
     Enable use of qualified identifiers. (see "Colon Qualifiers - Packages")
     Defining this as 0 has the rather drastic consequence that any code using
     packages will stop working, and will have to be modified. It should only
     be used if you *absolutely* need to use '::' in identifiers.

     USE_STRCASECMP
     Defines stricmp as strcasecmp, for Unix.

     STDIO_ADDS_CR
     Informs TinyScheme that stdio translates "\n" to "\r\n". For DOS/Windows.

     USE_DL
     Enables dynamically loaded routines. If you define this symbol, you
     should also include dynload.c in your compile.

     USE_PLIST
     Enables property lists (not Standard Scheme stuff). Off by default.
     
     USE_NO_FEATURES
     Shortcut to disable USE_MATH, USE_CHAR_CLASSIFIERS, USE_ASCII_NAMES,
     USE_STRING_PORTS, USE_ERROR_HOOK, USE_TRACING, USE_COLON_HOOK,
     USE_DL.

     USE_SCHEME_STACK
     Enables 'cons' stack (the alternative is a faster calling scheme, which 
     breaks continuations). Undefine it if you don't care about strict compatibility
     but you do care about faster execution.


     OS-X tip
     --------
     I don't have access to OS-X, but Brian Maher submitted the following tip:

[1] Download and install fink (I installed fink in
/usr/local/fink)
[2] Install the 'dlcompat' package using fink as such:
> fink install dlcompat
[3] Make the following changes to the
tinyscheme-1.32.tar.gz

diff -r tinyscheme-1.32/dynload.c
tinyscheme-1.32-new/dynload.c
24c24
< #define SUN_DL
---
> 
Only in tinyscheme-1.32-new/: dynload.o
Only in tinyscheme-1.32-new/: libtinyscheme.a Only in tinyscheme-1.32-new/: libtinyscheme.so diff -r tinyscheme-1.32/makefile tinyscheme-1.32-new/makefile
33,34c33,43
< LD = gcc
< LDFLAGS = -shared
---
> #LD = gcc
> #LDFLAGS = -shared
> #DEBUG=-g -Wno-char-subscripts -O
> #SYS_LIBS= -ldl
> #PLATFORM_FEATURES= -DSUN_DL=1
> 
> # Mac OS X
> CC = gcc
> CFLAGS = -I/usr/local/fink/include
> LD = gcc
> LDFLAGS = -L/usr/local/fink/lib
37c46
< PLATFORM_FEATURES= -DSUN_DL=1
---
> PLATFORM_FEATURES= -DSUN_DL=1 -DOSX
60c69
<       $(CC) -I. -c $(DEBUG) $(FEATURES)
$(DL_FLAGS) $<
---
>       $(CC) $(CFLAGS) -I. -c $(DEBUG)
$(FEATURES) $(DL_FLAGS) $<
66c75
<       $(CC) -o $@ $(DEBUG) $(OBJS) $(SYS_LIBS) 
---
>       $(CC) $(LDFLAGS) -o $@ $(DEBUG) $(OBJS)
$(SYS_LIBS)
Only in tinyscheme-1.32-new/: scheme
diff -r tinyscheme-1.32/scheme.c
tinyscheme-1.32-new/scheme.c
60,61c60,61
< #ifndef macintosh
< # include <malloc.h>
---
> #ifdef OSX
> /* Do nothing */
62a63,65
> # ifndef macintosh
> #  include <malloc.h>
> # else
77c80,81
< #endif /* macintosh */
---
> # endif /* macintosh */
> #endif /* !OSX */
Only in tinyscheme-1.32-new/: scheme.o
