## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = rcm

rcm_SOURCES = \
	rcm.c		\
	rcm.h		\
	rcm_callback.c	\
	rcm_callback.h	\
	rcm_dialog.c	\
	rcm_dialog.h	\
	rcm_gdk.c	\
	rcm_gdk.h	\
	rcm_misc.c	\
	rcm_misc.h

EXTRA_DIST = \
	pixmaps/rcm_360.xpm	\
	pixmaps/rcm_a_b.xpm	\
	pixmaps/rcm_ccw.xpm	\
	pixmaps/rcm_cw.xpm

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
	$(RT_LIBS)							\
	$(INTLLIBS)
