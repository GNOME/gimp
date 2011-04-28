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
libgimpcolor_export_symbols = -export-symbols gimpcolor.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpcolor-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpcolor.def $(DESTDIR)$(libdir)

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
	-DGIMP_COLOR_COMPILATION

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc		\
	gimpcolor.def

lib_LTLIBRARIES = libgimpcolor-2.0.la

libgimpcolor_2_0_la_SOURCES = \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.c	\
	gimpadaptivesupersample.h	\
	gimpbilinear.c			\
	gimpbilinear.h			\
	gimpcmyk.c			\
	gimpcmyk.h			\
	gimpcolormanaged.c		\
	gimpcolormanaged.h		\
	gimpcolorspace.c		\
	gimpcolorspace.h		\
	gimphsl.c			\
	gimphsl.h			\
	gimphsv.c			\
	gimphsv.h			\
	gimprgb.c			\
	gimprgb.h			\
	gimprgb-parse.c

libgimpcolorinclude_HEADERS = \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.h	\
	gimpbilinear.h			\
	gimpcmyk.h			\
	gimpcolormanaged.h		\
	gimpcolorspace.h		\
	gimphsl.h			\
	gimphsv.h			\
	gimprgb.h

libgimpcolor_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpcolor_export_symbols)

libgimpcolor_2_0_la_DEPENDENCIES = $(gimpcolor_def) $(libgimpbase)

libgimpcolor_2_0_la_LIBADD = $(GLIB_LIBS) $(libm)


#
# test programs, not to be built by default and never installed
#

TESTS = test-color-parser$(EXEEXT)

EXTRA_PROGRAMS = test-color-parser

test_color_parser_DEPENDENCIES = \
	$(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la

test_color_parser_LDADD = \
	$(GLIB_LIBS) 		\
	$(test_color_parser_DEPENDENCIES)


CLEANFILES = $(EXTRA_PROGRAMS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
