## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = MapObject

MapObject_SOURCES = \
	amb1.xpm		\
	amb2.xpm		\
	arcball.c		\
	arcball.h		\
	diffint1.xpm		\
	diffint2.xpm		\
	diffref1.xpm		\
	diffref2.xpm		\
	high1.xpm		\
	high2.xpm		\
	mapobject_apply.c	\
	mapobject_apply.h	\
	mapobject_image.c	\
	mapobject_image.h	\
	mapobject_main.c	\
	mapobject_main.h	\
	mapobject_preview.c	\
	mapobject_preview.h	\
	mapobject_shade.c	\
	mapobject_shade.h	\
	mapobject_ui.c		\
	mapobject_ui.h		\
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
