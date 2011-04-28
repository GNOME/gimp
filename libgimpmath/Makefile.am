## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

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
	$(INSTALL) .libs/libgimpmath-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpmath.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpmath-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpmath.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpmath-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpmath-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpmath-$(GIMP_API_VERSION).lib

gimpmath-@GIMP_API_VERSION@.lib: gimpmath.def
	lib -name:libgimpmath-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpmath.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpmathincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpmath

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpMath\"	\
	-DGIMP_MATH_COMPILATION

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc		\
	gimpmath.def

lib_LTLIBRARIES = libgimpmath-2.0.la

libgimpmath_2_0_la_SOURCES = \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.c	\
	gimpmatrix.h	\
	gimpmd5.c	\
	gimpmd5.h	\
	gimpvector.c	\
	gimpvector.h

libgimpmathinclude_HEADERS = \
	gimpmath.h	\
	gimpmathtypes.h	\
	gimpmatrix.h	\
	gimpmd5.h	\
	gimpvector.h

libgimpmath_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimpmath_export_symbols)

libgimpmath_2_0_la_DEPENDENCIES = $(gimpmath_def) $(libgimpbase)

libgimpmath_2_0_la_LIBADD = $(GLIB_LIBS) $(libm)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
