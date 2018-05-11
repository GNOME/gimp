## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
lighting_RC = lighting.rc.o
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = images

libexecdir = $(gimpplugindir)/plug-ins/lighting

libexec_PROGRAMS = lighting

lighting_SOURCES = \
	lighting-apply.c	\
	lighting-apply.h	\
	lighting-image.c	\
	lighting-image.h	\
	lighting-main.c		\
	lighting-main.h		\
	lighting-preview.c	\
	lighting-preview.h	\
	lighting-shade.c	\
	lighting-shade.h	\
	lighting-stock.c	\
	lighting-stock.h	\
	lighting-ui.c		\
	lighting-ui.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libm)			\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(lighting_RC)
