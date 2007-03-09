## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = images

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
	rcm_misc.h	\
	rcm_stock.c	\
	rcm_stock.h

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
