## Process this file with automake to produce Makefile.in

libgimpmathincludedir = $(includedir)/libgimpmath

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpMath\"		\
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
	gimpmath.def

lib_LTLIBRARIES = libgimpmath.la

libgimpmath_la_SOURCES = \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.c	\
	gimpmatrix.h	\
	gimpvector.c	\
	gimpvector.h

libgimpmathinclude_HEADERS = \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.h	\
	gimpvector.h

libgimpmath_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)
libgimpmath_la_LIBADD = $(GLIB_LIBS) -lm

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimpmath/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=libgimpmath/$@ CONFIG_HEADERS= $(SHELL) ./config.status
