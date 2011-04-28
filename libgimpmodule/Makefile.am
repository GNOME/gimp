## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpmodule_def = gimpmodule.def
libgimpmodule_export_symbols = -export-symbols gimpmodule.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpmodule-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpmodule.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpmodule-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpmodule.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpmodule-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpmodule-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpmodule-$(GIMP_API_VERSION).lib

gimpmodule-@GIMP_API_VERSION@.lib: gimpmodule.def
	lib -name:libgimpmodule-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpmodule.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpmoduleincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpmodule

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpModule\"	\
	-DGIMP_MODULE_COMPILATION

INCLUDES = \
	-I$(top_srcdir)			\
	$(GLIB_CFLAGS)			\
	$(GMODULE_NO_EXPORT_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc		\
	gimpmodule.def

lib_LTLIBRARIES = libgimpmodule-2.0.la

libgimpmodule_2_0_la_SOURCES = \
	gimpmoduletypes.h	\
	gimpmodule.c		\
	gimpmodule.h		\
	gimpmoduledb.c		\
	gimpmoduledb.h

libgimpmoduleinclude_HEADERS = \
	gimpmoduletypes.h	\
	gimpmodule.h		\
	gimpmoduledb.h

libgimpmodule_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpmodule_export_symbols)

libgimpmodule_2_0_la_DEPENDENCIES = $(gimpmodule_def)

libgimpmodule_2_0_la_LIBADD = \
	$(libgimpbase)			\
	$(GMODULE_NO_EXPORT_LIBS) 	\
	$(GLIB_LIBS)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
