## Process this file with automake to produce Makefile.in

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
file_ico_RC = file-ico.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-ico

libexec_PROGRAMS = file-ico

file_ico_CFLAGS = $(PNG_CFLAGS)

file_ico_SOURCES = \
	ico.c		\
	ico.h		\
	ico-dialog.c	\
	ico-dialog.h	\
	ico-load.c	\
	ico-load.h	\
	ico-save.c	\
	ico-save.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)             \
	$(GEGL_LIBS)		\
	$(PNG_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_ico_RC)
