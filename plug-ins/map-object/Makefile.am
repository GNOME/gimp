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
map_object_RC = map-object.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/map-object

libexec_PROGRAMS = map-object

map_object_SOURCES = \
	arcball.c		\
	arcball.h		\
	map-object-apply.c	\
	map-object-apply.h	\
	map-object-icons.c	\
	map-object-icons.h	\
	map-object-image.c	\
	map-object-image.h	\
	map-object-main.c	\
	map-object-main.h	\
	map-object-preview.c	\
	map-object-preview.h	\
	map-object-shade.c	\
	map-object-shade.h	\
	map-object-ui.c		\
	map-object-ui.h		\
	../lighting/images/lighting-icon-images.c	\
	../lighting/images/lighting-icon-images.h

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
	$(map_object_RC)
