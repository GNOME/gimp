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
	gfig_arc.c	\
	gfig_arc.h	\
	gfig_bezier.c	\
	gfig_bezier.h	\
	gfig_circle.c	\
	gfig_circle.h	\
	gfig_ellipse.c	\
	gfig_ellipse.h	\
	gfig_line.c	\
	gfig_line.h	\
	gfig_poly.c	\
	gfig_poly.h	\
	gfig_spiral.c	\
	gfig_spiral.h	\
	gfig_star.c	\
	gfig_star.h	\
	gfig-stock.c	\
	gfig-stock.h	\
	pix_data.h

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
