## Process this file with automake to produce Makefile.in

SUBDIRS = scripts
BUILT_SOURCES=	gtclenums.h
AUTOMAKE_OPTIONS=	no-dependencies
SUFFIXES = .h

scriptdata =

libexec_PROGRAMS = gimptcl

gimptcl_SOURCES= \
	gtcl.h		\
	gtclenums.h	\
	gtclMain.c	\
	gtcl_misc.c	\
	gtclConst.c	\
	gtclPDB.c	\
	gtclEnums.h

INCLUDES= \
   ${X_CFLAGS}					\
	-I/usr/local/include		\
	-I$(top_srcdir)

LDADD = \
        $(top_builddir)/libgimp/libgimp.la                      \
        @LIBTCL_LIB@	                                           \
        $(X_LIBS)                                               \
        -lc                                                     \
        -lm -ldl

gtclenums.h: ${top_srcdir}/libgimp/gimpenums.h gtclEnums.h
	misc/enum.pl ${top_srcdir}/libgimp/gimpenums.h gtclEnums.h >$@
