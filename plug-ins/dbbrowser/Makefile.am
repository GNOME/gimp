## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = README

libexec_PROGRAMS = dbbrowser

dbbrowser_SOURCES = \
	dbbrowser.c		\
	dbbrowser_utils.c	\
	dbbrowser_utils.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
