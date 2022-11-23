## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
ligmamodule_def = ligmamodule.def
libligmamodule_export_symbols = -export-symbols $(srcdir)/ligmamodule.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmamodule-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmamodule.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmamodule-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmamodule.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmamodule-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmamodule-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmamodule-$(LIGMA_API_VERSION).lib

ligmamodule-@LIGMA_API_VERSION@.lib: ligmamodule.def
	lib -name:libligmamodule-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmamodule.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmamoduleincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmamodule

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibLigmaModule\"	\
	-DLIGMA_MODULE_COMPILATION		\
	-I$(top_srcdir)				\
	$(GIO_CFLAGS)				\
	$(GLIB_CFLAGS)				\
	$(GMODULE_NO_EXPORT_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	ligmamodule.def

lib_LTLIBRARIES = libligmamodule-@LIGMA_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmamodule/Makefile.gi

libligmamodule_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligmamodule_introspectable)

libligmamoduleinclude_HEADERS = \
	$(libligmamodule_introspectable_headers)

libligmamodule_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libligmamodule_export_symbols)

EXTRA_libligmamodule_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmamodule_def)

libligmamodule_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligmabase)			\
	$(libligmaconfig)		\
	$(GMODULE_NO_EXPORT_LIBS) 	\
	$(GIO_LIBS)			\
	$(GLIB_LIBS)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
