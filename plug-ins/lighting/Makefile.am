## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

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
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
lighting_RC = lighting.rc.o
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = images

libexecdir = $(ligmaplugindir)/plug-ins/lighting

libexec_PROGRAMS = lighting

lighting_SOURCES = \
	lighting-apply.c	\
	lighting-apply.h	\
	lighting-icons.c	\
	lighting-icons.h	\
	lighting-image.c	\
	lighting-image.h	\
	lighting-main.c		\
	lighting-main.h		\
	lighting-preview.c	\
	lighting-preview.h	\
	lighting-shade.c	\
	lighting-shade.h	\
	lighting-ui.c		\
	lighting-ui.h		\
	images/lighting-icon-images.c	\
	images/lighting-icon-images.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
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
	$(lighting_RC)
