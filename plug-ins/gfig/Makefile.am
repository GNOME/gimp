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
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
gfig_RC = gfig.rc.o
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = gfig-examples images

libexecdir = $(ligmaplugindir)/plug-ins/gfig

libexec_PROGRAMS = gfig

gfig_SOURCES = \
	gfig.c			\
	gfig.h			\
	gfig-arc.c		\
	gfig-arc.h		\
	gfig-bezier.c		\
	gfig-bezier.h		\
	gfig-circle.c		\
	gfig-circle.h		\
	gfig-dialog.c		\
	gfig-dialog.h		\
	gfig-dobject.c		\
	gfig-dobject.h		\
	gfig-ellipse.c		\
	gfig-ellipse.h		\
	gfig-grid.c		\
	gfig-grid.h		\
	gfig-icons.c		\
	gfig-icons.h		\
	gfig-line.c		\
	gfig-line.h		\
	gfig-poly.c		\
	gfig-poly.h		\
	gfig-preview.c		\
	gfig-preview.h		\
	gfig-rectangle.c	\
	gfig-rectangle.h	\
	gfig-spiral.c		\
	gfig-spiral.h		\
	gfig-star.c		\
	gfig-star.h		\
	gfig-style.c		\
	gfig-style.h    	\
	gfig-types.h    	\
	images/gfig-icon-images.c	\
	images/gfig-icon-images.h

EXTRA_DIST = README

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
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(gfig_RC)
