## Process this file with automake to produce Makefile.in

libgimpcolorincludedir = $(includedir)/gimp-$(GIMP_MAJOR_VERSION).$(GIMP_MINOR_VERSION)/libgimpcolor

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpColor\"		\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpcolor.def

lib_LTLIBRARIES = libgimpcolor.la

libgimpcolor_la_SOURCES = \
	gimpcolor.h		\
	gimpcolortypes.h	\
	gimpcolorspace.c	\
	gimpcolorspace.h	\
	gimphsv.c		\
	gimphsv.h		\
	gimprgb.c		\
	gimprgb.h

libgimpcolorinclude_HEADERS = \
	gimpcolor.h		\
	gimpcolortypes.h	\
	gimpcolorspace.h	\
	gimphsv.h		\
	gimprgb.h

libgimpcolor_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)
libgimpcolor_la_LIBADD = $(GLIB_LIBS) -lm

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimpcolor/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=libgimpcolor/$@ CONFIG_HEADERS= $(SHELL) ./config.status
