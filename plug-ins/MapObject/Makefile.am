## Process this file with automake to produce Makefile.in

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
	mapobject_pixmaps.h	\
	mapobject_preview.c	\
	mapobject_preview.h	\
	mapobject_shade.c	\
	mapobject_shade.h	\
	mapobject_ui.c		\
	mapobject_ui.h		\
	specref1.xpm		\
	specref2.xpm

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/plug-ins/libgck	\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la		\
	$(top_builddir)/libgimp/libgimp.la		\
	$(top_builddir)/plug-ins/libgck/gck/libgck.la	\
	$(GTK_LIBS)					\
	$(INTLLIBS)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
