## Process this file with automake to produce Makefile.in

LDFLAGS = -mwindows

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = winsnap

winsnap_SOURCES = \
	winsnap.c	\
	winsnap.h

EXTRA_DIST = \
	resource.h	\
	select.cur	\
	small.ico	\
	winsnap.ico	\
	winsnap.rc

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
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
