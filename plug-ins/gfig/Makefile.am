## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = gfig-examples images

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gfig

gfig_SOURCES = \
	gfig.c		\
	gfig.h		\
	gfig-arc.c	\
	gfig-arc.h	\
	gfig-bezier.c	\
	gfig-bezier.h	\
	gfig-circle.c	\
	gfig-circle.h	\
	gfig-dobject.c	\
	gfig-dobject.h	\
	gfig-ellipse.c	\
	gfig-ellipse.h	\
	gfig-grid.c	\
	gfig-grid.h	\
	gfig-line.c	\
	gfig-line.h	\
	gfig-poly.c	\
	gfig-poly.h	\
	gfig-preview.c	\
	gfig-preview.h	\
	gfig-spiral.c	\
	gfig-spiral.h	\
	gfig-star.c	\
	gfig-star.h	\
	gfig-stock.c	\
	gfig-stock.h	\
	pix-data.h

EXTRA_DIST = README

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)
