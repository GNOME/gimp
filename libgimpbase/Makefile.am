## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpbase_def = gimpbase.def
libgimpbase_export_symbols = -export-symbols gimpbase.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpbase-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpbase.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpbase-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpbase.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpbase-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpbase-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpbase-$(GIMP_API_VERSION).lib

gimpbase-@GIMP_API_VERSION@.lib: gimpbase.def
	lib -name:libgimpbase-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpbase.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpbaseincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpbase

AM_CPPFLAGS = \
	-DPREFIX=\""$(prefix)"\"		\
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DLOCALEDIR=\""$(localedir)"\"		\
	-DPLUGINDIR=\""$(gimpplugindir)"\"	\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpBase\"		\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc	\
	gimpbase.def

lib_LTLIBRARIES = libgimpbase-1.3.la

# help `make' along by giving another name for the file, which it knows
# how to build
../libgimpbase/gimpversion.h: gimpversion.h
	@:

libgimpbase_1_3_la_SOURCES = \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimplimits.h		\
	gimpversion.h		\
	\
	gimpbase-private.c	\
	gimpbase-private.h	\
	gimpdatafiles.c		\
	gimpdatafiles.h		\
	gimpenv.c		\
	gimpenv.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimpsignal.c            \
	gimpsignal.h            \
	gimpunit.c		\
	gimpunit.h		\
	gimputils.c            	\
	gimputils.h            	\
	gimpwin32-io.h		\
	gimpwire.c		\
	gimpwire.h

libgimpbaseinclude_HEADERS = \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimplimits.h		\
	gimpversion.h		\
	\
	gimpdatafiles.h		\
	gimpenv.h		\
	gimpparasite.h		\
	gimpparasiteio.h	\
	gimpsignal.h		\
	gimpunit.h		\
	gimputils.h

libgimpbase_1_3_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpbase_export_symbols)

libgimpbase_1_3_la_DEPENDENCIES = $(gimpbase_def)

libgimpbase_1_3_la_LIBADD = $(GLIB_LIBS)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
