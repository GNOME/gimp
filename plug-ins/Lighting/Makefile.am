## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = Lighting

Lighting_SOURCES = \
	amb1.xpm		\
	amb2.xpm		\
	diffint1.xpm		\
	diffint2.xpm		\
	diffref1.xpm		\
	diffref2.xpm		\
	high1.xpm		\
	high2.xpm		\
	lighting_apply.c	\
	lighting_apply.h	\
	lighting_image.c	\
	lighting_image.h	\
	lighting_main.c		\
	lighting_main.h		\
	lighting_preview.c	\
	lighting_preview.h	\
	lighting_shade.c	\
	lighting_shade.h	\
	lighting_ui.c		\
	lighting_ui.h		\
	specref1.xpm		\
	specref2.xpm

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)
