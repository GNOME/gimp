## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpmodule_def = gimpmodule.def
libgimpmodule_export_symbols = -export-symbols gimpmodule.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpmodule-1.3.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpmodule-1.3.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpmodule-1.3.lib

install-ms-lib:
	$(INSTALL) gimpmodule-1.3.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpmodule-1.3.lib

gimpmodule-1.3.lib: gimpmodule.def
	lib -name:libgimpmodule-1.3-@LT_CURRENT_MINUS_AGE@.dll -def:gimpmodule.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpmoduleincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimpmodule

AM_CPPFLAGS = \
	-DPREFIX=\""$(prefix)"\"		\
	-DGIMPDIR=\""$(gimpdir)"\"		\
	-DDATADIR=\""$(gimpdatadir)"\"		\
	-DLOCALEDIR=\""$(localedir)"\"		\
	-DPLUGINDIR=\""$(gimpplugindir)"\"	\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DG_LOG_DOMAIN=\"LibGimpModule\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS) \
	$(GMODULE_CFLAGS) \
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc	\
	gimpmodule.def

lib_LTLIBRARIES = libgimpmodule-1.3.la

libgimpmodule_1_3_la_SOURCES = \
	gimpmoduletypes.h	\
	gimpmodule.c		\
	gimpmodule.h		\
	gimpmoduledb.c		\
	gimpmoduledb.h

libgimpmoduleinclude_HEADERS = \
	gimpmoduletypes.h	\
	gimpmodule.h		\
	gimpmoduledb.h

EXTRA_HEADERS =

libgimpmodule_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(no_undefined) 					\
	$(libgimpmodule_export_symbols)

libgimpmodule_1_3_la_DEPENDENCIES = $(gimpmodule_def)

libgimpmodule_1_3_la_LIBADD = \
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GLIB_LIBS)							\
	$(GMODULE_LIBS)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
