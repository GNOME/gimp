## Process this file with automake to produce Makefile.in

twain_LDFLAGS = -mwindows

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = twain

twain_SOURCES = \
	tw_func.c	\
	tw_func.h	\
	tw_util.c	\
	tw_util.h	\
	twain.c		\
	twain.h

EXTRA_DIST = \
	README		\
	tw_dump.c	\
	tw_dump.h	\
	tw_sess.c

INCLUDES = \
	-I$(top_srcdir)			\
	$(GLIB_CFLAGS)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la	\
	$(GLIB_LIBS)						\
	$(RT_LIBS)							\
	$(INTLLIBS)						\
	-luser32
