## Process this file with automake to produce Makefile.in

LDFLAGS = -mwindows

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
	tw_sess.c	\
	twain.ico	\
	twain.rc

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)				\
	-luser32

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
