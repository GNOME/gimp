## Process this file with automake to produce Makefile.in

libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la

libligmahelp = $(top_builddir)/plug-ins/help/libligmahelp.a

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
help_browser_RC = help-browser.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/help-browser

libexec_PROGRAMS = help-browser

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(WEBKIT_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libligmahelp)		\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(WEBKIT_LIBS)		\
	$(GIO_LIBS)		\
	$(GLIB_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(help_browser_RC)

help_browser_SOURCES = \
	help-browser.c		\
	dialog.c		\
	dialog.h		\
	uri.c			\
	uri.h
