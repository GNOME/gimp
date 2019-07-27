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
gimpcolor_def = gimpcolor.def
libgimpcolor_export_symbols = -export-symbols $(srcdir)/gimpcolor.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpcolor-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimpcolor.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpcolor-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpcolor.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpcolor-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpcolor-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpcolor-$(GIMP_API_VERSION).lib

gimpcolor-@GIMP_API_VERSION@.lib: gimpcolor.def
	lib -name:libgimpcolor-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpcolor.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpcolorincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpcolor

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpColor\"	\
	-DGIMP_COLOR_COMPILATION	\
	-I$(top_srcdir)			\
	$(GEGL_CFLAGS)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(LCMS_CFLAGS)			\
	-I$(includedir)

EXTRA_DIST = \
	gimpcolor.def

lib_LTLIBRARIES = libgimpcolor-@GIMP_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libgimpcolor/Makefile.gi

libgimpcolor_@GIMP_API_VERSION@_la_SOURCES = \
	gimpcolor.h			\
	$(libgimpcolor_introspectable)

libgimpcolorinclude_HEADERS = \
	gimpcolor.h			\
	$(libgimpcolor_introspectable_headers)

libgimpcolor_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpcolor_export_symbols)

EXTRA_libgimpcolor_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimpcolor_def)

libgimpcolor_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimpbase)		\
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
	$(libgimpbase)	\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la

test_color_parser_LDADD = \
	$(CAIRO_LIBS) 		\
	$(GLIB_LIBS) 		\
	$(test_color_parser_DEPENDENCIES)


CLEANFILES = $(EXTRA_PROGRAMS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
