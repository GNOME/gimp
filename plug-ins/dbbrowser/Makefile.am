## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

noinst_LTLIBRARIES = libgimpprocbrowser.la
libexec_PROGRAMS = plugin-browser procedure-browser

libgimpprocbrowser_la_SOURCES = \
	gimpprocbox.c		\
	gimpprocbox.h		\
	gimpprocbrowser.c	\
	gimpprocbrowser.h	\
	gimpprocview.c		\
	gimpprocview.h

plugin_browser_SOURCES = \
	plugin-browser.c

procedure_browser_SOURCES = \
	procedure-browser.c


EXTRA_DIST = README

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

LDADD = \
	libgimpprocbrowser.la	\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
