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
	$(INSTALL) .libs/libgimpcolor-1.3.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpcolor-1.3.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpcolor-1.3.lib

install-ms-lib:
	$(INSTALL) gimpcolor-1.3.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpcolor-1.3.lib

gimpcolor-1.3.lib: gimpcolor.def
	lib -name:libgimpcolor-1.3-@LT_CURRENT_MINUS_AGE@.dll -def:gimpcolor.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpcolorincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpcolor

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"LibGimpColor\"	\
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
	gimpcolor.def

lib_LTLIBRARIES = libgimpcolor-1.3.la

libgimpcolor_1_3_la_SOURCES = @STRIP_BEGIN@ \
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
	gimprgb.h			\
@STRIP_END@

libgimpcolorinclude_HEADERS = @STRIP_BEGIN@ \
	gimpcolor.h			\
	gimpcolortypes.h		\
	gimpadaptivesupersample.h	\
	gimpbilinear.h			\
	gimpcolorspace.h		\
	gimphsv.h			\
	gimprgb.h			\
@STRIP_END@

EXTRA_HEADERS =

libgimpcolor_1_3_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(no_undefined) \
	$(libgimpcolor_export_symbols) \
@STRIP_END@

libgimpcolor_1_3_la_LIBADD = $(GLIB_LIBS) $(libm)

libgimpcolor_1_3_la_DEPENDENCIES = $(gimpcolor_def)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
