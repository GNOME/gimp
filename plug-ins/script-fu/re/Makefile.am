## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
    -DUSE_INTERFACE=1 \
    -I$(top_srcdir)

noinst_LIBRARIES = libre.a

libre_a_SOURCES = \
    cclass.h   \
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

EXTRA_DIST = \
    COPYRIGHT  \
    README     \
    README.1st \
    WHATSNEW
