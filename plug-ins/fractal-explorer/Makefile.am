## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
fractal_explorer_RC = fractal-explorer.rc.o
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = examples

libexecdir = $(ligmaplugindir)/plug-ins/fractal-explorer

libexec_PROGRAMS = fractal-explorer

fractal_explorer_SOURCES = \
	fractal-explorer-dialogs.h	\
	fractal-explorer-dialogs.c	\
	fractal-explorer.c		\
	fractal-explorer.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(fractal_explorer_RC)
