## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_PROGRAMS = Lighting

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
	lighting_pixmaps.h	\
	lighting_preview.c	\
	lighting_preview.h	\
	lighting_shade.c	\
	lighting_shade.h	\
	lighting_ui.c		\
	lighting_ui.h		\
	specref1.xpm		\
	specref2.xpm

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/plug-ins/libgck	\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la		\
	$(top_builddir)/libgimp/libgimp.la		\
	$(top_builddir)/plug-ins/libgck/gck/libgck.la	\
	$(GTK_LIBS)					\
	$(INTLLIBS)

DEPS = \
	$(top_builddir)/libgimp/libgimpui.la		\
	$(top_builddir)/libgimp/libgimp.la		\
	$(top_builddir)/plug-ins/libgck/gck/libgck.la
	
Lighting_DEPENDENCIES = $(DEPS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done
