## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpbase_def = gimpbase.def
libgimpbase_export_symbols = -export-symbols gimpbase.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpbase-$(LT_RELEASE).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpbase.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpbase-$(LT_RELEASE).dll.a
	-rm $(DESTDIR)$(libdir)/gimpbase.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpbase-$(LT_RELEASE).lib

install-ms-lib:
	$(INSTALL) gimpbase-$(LT_RELEASE).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpbase-$(LT_RELEASE).lib

gimpbase-@LT_RELEASE@.lib: gimpbase.def
	lib -name:libgimpbase-$(LT_RELEASE)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpbase.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpbaseincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpbase

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
	gimpunit.h		\
	gimpversion.h		\
	\
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
	gimputils.c            	\
	gimputils.h            	\
	gimpwin32-io.h		\
	gimpwire.c		\
	gimpwire.h

libgimpbaseinclude_HEADERS = \
	gimpbase.h		\
	gimpbasetypes.h		\
	gimplimits.h		\
	gimpunit.h		\
	gimputils.h		\
	gimpversion.h		\
	\
	gimpdatafiles.h		\
	gimpenv.h		\
	gimpparasite.h		\
	gimpparasiteio.h	\
	gimpsignal.h

libgimpbase_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(no_undefined) 					\
	$(libgimpbase_export_symbols)

libgimpbase_1_3_la_DEPENDENCIES = $(gimpbase_def)

libgimpbase_1_3_la_LIBADD = $(GLIB_LIBS)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

