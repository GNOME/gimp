## Process this file with automake to produce Makefile.in

libgimpbaseincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpbase

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpBase\"		\
	@GIMP_THREAD_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

EXTRA_DIST = \
	gimpversion.h.in	\
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpbase.def

lib_LTLIBRARIES = libgimpbase-1.3.la

# help `make' along by giving another name for the file, which it knows
# how to build
../libgimpbase/gimpversion.h: gimpversion.h
	@:

libgimpbase_1_3_la_SOURCES = @STRIP_BEGIN@ \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimpenv.c		\
	gimpenv.h		\
	gimplimits.h		\
	gimpsignal.c            \
	gimpsignal.h            \
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpunit.h		\
	gimpversion.h		\
	gimpwire.c		\
	gimpwire.h		\
@STRIP_END@

libgimpbaseinclude_HEADERS = @STRIP_BEGIN@ \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimpenv.h		\
	gimplimits.h		\
	gimpsignal.h            \
	gimpparasite.h		\
	gimpparasiteio.h	\
	gimpunit.h		\
	gimpversion.h		\
@STRIP_END@

EXTRA_HEADERS +=

libgimpbase_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimpbase_1_3_la_LIBADD = $(GLIB_LIBS)
