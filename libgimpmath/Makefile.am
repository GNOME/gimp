## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_WIN32
else
libm = -lm
endif

if OS_WIN32
gimpmath_def = gimpmath.def
libgimpmath_export_symbols = -export-symbols gimpmath.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpmath-1.3.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpmath-1.3.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpmath-1.3.lib

install-ms-lib:
	$(INSTALL) gimpmath-1.3.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpmath-1.3.lib

gimpmath-1.3.lib: gimpmath.def
	lib -name:libgimpmath-1.3-@LT_CURRENT_MINUS_AGE@.dll -def:gimpmath.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpmathincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpmath

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"LibGimpMath\"	\
	@GIMP_THREAD_FLAGS@		\
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
	$(no_undefined) \
	$(libgimpmath_export_symbols) \
@STRIP_END@

libgimpmath_1_3_la_LIBADD = $(GLIB_LIBS) $(libm)

libgimpmath_1_3_la_DEPENDENCIES = $(gimpmath_def)

#
# test programs, not to be built by default and never installed
#

TESTS = test-md5$(EXEEXT)

EXTRA_PROGRAMS = test-md5

test_md5_DEPENDENCIES = \
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la

test_md5_LDADD = @STRIP_BEGIN@ 	\
	$(GLIB_LIBS) 			\
	$(test_md5_DEPENDENCIES)	\
@STRIP_END@

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
