## Process this file with automake to produce Makefile.in

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
	$(INSTALL) .libs/libgimpcolor-$(LT_RELEASE).dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpcolor-$(LT_RELEASE).dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpcolor-$(LT_RELEASE).lib

install-ms-lib:
	$(INSTALL) gimpcolor-$(LT_RELEASE).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpcolor-$(LT_RELEASE).lib

gimpcolor-@LT_RELEASE@.lib: gimpcolor.def
	lib -name:libgimpcolor-$(LT_RELEASE)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpcolor.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpcolorincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpcolor

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
	gimpcolorspace.h		\
	gimphsv.h			\
	gimprgb.h

libgimpcolor_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(no_undefined) 					\
	$(libgimpcolor_export_symbols)

libgimpcolor_1_3_la_LIBADD = $(GLIB_LIBS) $(libm)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
