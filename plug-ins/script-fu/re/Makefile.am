## Process this file with automake to produce Makefile.in

#if PLATFORM_WIN32
#no_undefined = -no-undefined
#endif
#
#if OS_WIN32
#mwindows = -mwindows
#endif

SCHEME_H_DIR=../tinyscheme

AM_CFLAGS = -DUSE_INTERFACE=1 -I $(SCHEME_H_DIR)

noinst_LIBRARIES = libre.a

libre_a_SOURCES = \
    ccass.h   \
    cname.h    \
    debug.c    \
    re.c       \
    re.h       \
    regcomp.c  \
    regerror.c \
    regex2.h   \
    regexec.c  \
    regex.h    \
    regfree.c  \
    split.c    \
    utils.h
