## Process this file with automake to produce Makefile.in

libgimpmathincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpmath

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpMath\"		\
	@GIMP_THREAD_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)		\
@STRIP_END@

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimpmath.def

lib_LTLIBRARIES = libgimpmath-1.3.la

libgimpmath_1_3_la_SOURCES = @STRIP_BEGIN@ \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.c	\
	gimpmatrix.h	\
	gimpvector.c	\
	gimpvector.h	\
@STRIP_END@

libgimpmathinclude_HEADERS = @STRIP_BEGIN@ \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.h	\
	gimpvector.h	\
@STRIP_END@

EXTRA_HEADERS =

libgimpmath_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimpmath_1_3_la_LIBADD = $(GLIB_LIBS) -lm
