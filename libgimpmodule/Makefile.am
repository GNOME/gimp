## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpmodule_def = gimpmodule.def
libgimpmodule_export_symbols = -export-symbols gimpmodule.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpmodule-$(LT_RELEASE).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpmodule.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpmodule-$(LT_RELEASE).dll.a
	-rm $(DESTDIR)$(libdir)/gimpmodule.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpmodule-$(LT_RELEASE).lib

install-ms-lib:
	$(INSTALL) gimpmodule-$(LT_RELEASE).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpmodule-$(LT_RELEASE).lib

gimpmodule-@LT_RELEASE@.lib: gimpmodule.def
	lib -name:libgimpmodule-$(LT_RELEASE)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpmodule.def -out:$@

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
