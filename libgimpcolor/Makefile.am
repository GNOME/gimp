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
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc		\
	gimpcolor.def

lib_LTLIBRARIES = libgimpcolor-1.3.la

libgimpcolor_1_3_la_SOURCES = \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.c	\
	gimpadaptivesupersample.h	\
	gimpbilinear.c			\
	gimpbilinear.h			\
	gimpcmyk.c			\
	gimpcmyk.h			\
	gimpcolorspace.c		\
	gimpcolorspace.h		\
	gimphsv.c			\
	gimphsv.h			\
	gimprgb.c			\
	gimprgb.h

libgimpcolorinclude_HEADERS = \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.h	\
	gimpbilinear.h			\
	gimpcmyk.h			\
	gimpcolorspace.h		\
	gimphsv.h			\
	gimprgb.h

libgimpcolor_1_3_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpcolor_export_symbols)

libgimpcolor_1_3_la_DEPENDENCIES = $(gimpcolor_def) $(libgimpbase)

libgimpcolor_1_3_la_LIBADD = $(GLIB_LIBS) $(libm)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
