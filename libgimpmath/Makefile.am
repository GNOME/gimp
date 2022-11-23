## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_WIN32
else
libm = -lm
endif

if OS_WIN32
ligmamath_def = ligmamath.def
libligmamath_export_symbols = -export-symbols $(srcdir)/ligmamath.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmamath-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmamath.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmamath-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmamath.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmamath-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmamath-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmamath-$(LIGMA_API_VERSION).lib

ligmamath-@LIGMA_API_VERSION@.lib: ligmamath.def
	lib -name:libligmamath-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmamath.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmamathincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmamath

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibLigmaMath\"	\
	-DLIGMA_MATH_COMPILATION		\
	-I$(top_srcdir)			\
	$(GLIB_CFLAGS)			\
	-I$(includedir)

EXTRA_DIST = \
	ligmamath.def

lib_LTLIBRARIES = libligmamath-@LIGMA_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmamath/Makefile.gi

libligmamath_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligmamath_introspectable)

libligmamathinclude_HEADERS = \
	$(top_srcdir)/libligmamath/ligmamath.h	\
	$(libligmamath_introspectable_headers)

libligmamath_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libligmamath_export_symbols)

EXTRA_libligmamath_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmamath_def)

libligmamath_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligmabase)	\
	$(GLIB_LIBS)	\
	$(libm)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
