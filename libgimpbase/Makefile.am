## Process this file with automake to produce Makefile.in

libgimpbaseincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpbase

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpBase\"		\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/intl	\
	-I../intl		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpbase.def

lib_LTLIBRARIES = libgimpbase-1.3.la

libgimpbase_1_3_la_SOURCES = \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimpenv.c		\
	gimpenv.h		\
	gimplimits.h		\
	gimpsignal.c            \
	gimpsignal.h            \
	gimputils.c		\
	gimputils.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpunit.h		\
	gimpwire.c		\
	gimpwire.h

libgimpbaseinclude_HEADERS = \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimpenv.h		\
	gimplimits.h		\
	gimpsignal.h            \
	gimputils.h		\
	gimpparasite.h		\
	gimpparasiteio.h	\
	gimpunit.h

libgimpbase_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libgimpbase_1_3_la_LIBADD = $(GLIB_LIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/libgimpcolor/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=libgimpcolor/$@ CONFIG_HEADERS= $(SHELL) ./config.status
