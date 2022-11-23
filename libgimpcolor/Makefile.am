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
ligmacolor_def = ligmacolor.def
libligmacolor_export_symbols = -export-symbols $(srcdir)/ligmacolor.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmacolor-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmacolor.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmacolor-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmacolor.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmacolor-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmacolor-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmacolor-$(LIGMA_API_VERSION).lib

ligmacolor-@LIGMA_API_VERSION@.lib: ligmacolor.def
	lib -name:libligmacolor-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmacolor.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmacolorincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmacolor

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibLigmaColor\"	\
	-DLIGMA_COLOR_COMPILATION	\
	-I$(top_srcdir)			\
	$(GEGL_CFLAGS)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(LCMS_CFLAGS)			\
	-I$(includedir)

EXTRA_DIST = \
	ligmacolor.def

lib_LTLIBRARIES = libligmacolor-@LIGMA_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmacolor/Makefile.gi

libligmacolor_@LIGMA_API_VERSION@_la_SOURCES = \
	ligmacolor.h			\
	$(libligmacolor_introspectable)

libligmacolorinclude_HEADERS = \
	ligmacolor.h			\
	$(libligmacolor_introspectable_headers)

libligmacolor_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libligmacolor_export_symbols)

EXTRA_libligmacolor_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmacolor_def)

libligmacolor_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligmabase)		\
	$(GEGL_LIBS)		\
	$(CAIRO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(LCMS_LIBS)		\
	$(libm)


#
# test programs, not to be built by default and never installed
#

TESTS = test-color-parser$(EXEEXT)

EXTRA_PROGRAMS = test-color-parser

test_color_parser_DEPENDENCIES = \
	$(libligmabase)	\
	$(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la

test_color_parser_LDADD = \
	$(CAIRO_LIBS) 		\
	$(GLIB_LIBS) 		\
	$(test_color_parser_DEPENDENCIES)


CLEANFILES = $(EXTRA_PROGRAMS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
