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
	gimpmd5.c	\
	gimpmd5.h	\
	gimpvector.c	\
	gimpvector.h	\
@STRIP_END@

libgimpmathinclude_HEADERS = @STRIP_BEGIN@ \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.h	\
	gimpmd5.h	\
	gimpvector.h	\
@STRIP_END@

EXTRA_HEADERS =

libgimpmath_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
@STRIP_END@

libgimpmath_1_3_la_LIBADD = $(GLIB_LIBS) -lm


#
# test programs, not to be built by default and never installed
#

TESTS = test-md5

EXTRA_PROGRAMS = test-md5

test_md5_DEPENDENCIES = \
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la

test_md5_LDADD = @STRIP_BEGIN@ 	\
	$(GLIB_LIBS) 			\
	$(test_md5_DEPENDENCIES)	\
@STRIP_END@
