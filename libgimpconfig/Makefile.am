## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpconfig_def = gimpconfig.def
libgimpconfig_export_symbols = -export-symbols gimpconfig.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpconfig-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpconfig.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpconfig-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpconfig.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpconfig-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpconfig-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpconfig-$(GIMP_API_VERSION).lib

gimpconfig-@GIMP_API_VERSION@.lib: gimpconfig.def
	lib -name:libgimpconfig-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpconfig.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpconfigincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpconfig

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpConfig\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc	\
	gimpconfig.def

lib_LTLIBRARIES = libgimpconfig-2.0.la

libgimpconfig_sources = 		\
	gimpconfig.h			\
	gimpconfigtypes.h		\
	gimpconfig-iface.c		\
	gimpconfig-iface.h		\
	gimpconfig-deserialize.c	\
	gimpconfig-deserialize.h	\
	gimpconfig-error.c		\
	gimpconfig-error.h		\
	gimpconfig-params.h		\
	gimpconfig-path.c		\
	gimpconfig-path.h		\
	gimpconfig-serialize.c		\
	gimpconfig-serialize.h		\
	gimpconfigwriter.c		\
	gimpconfigwriter.h		\
	gimpscanner.c			\
	gimpscanner.h			\
	gimpconfig-utils.c		\
	gimpconfig-utils.h


libgimpconfig_built_sources =

libgimpconfig_2_0_la_SOURCES = $(libgimpconfig_sources) $(libgimpconfig_built_sources)


libgimpconfiginclude_HEADERS = \
	gimpconfig.h			\
	gimpconfigtypes.h		\
	gimpconfig-iface.h		\
	gimpconfig-deserialize.h	\
	gimpconfig-error.h		\
	gimpconfig-params.h		\
	gimpconfig-path.h		\
	gimpconfig-serialize.h		\
	gimpconfigwriter.h		\
	gimpscanner.h			\
	gimpconfig-utils.h


libgimpconfig_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpconfig_export_symbols)

libgimpconfig_2_0_la_DEPENDENCIES = $(gimpconfig_def)

libgimpconfig_2_0_la_LIBADD = $(GLIB_LIBS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib


#
# rules to generate built sources
#

gen_sources = xgen-bec
CLEANFILES = $(gen_sources)

