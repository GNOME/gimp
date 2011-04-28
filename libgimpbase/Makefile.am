## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
ole32_lib = -lole32
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
	-DLOCALEDIR=\""$(gimplocaledir)"\"	\
	-DPLUGINDIR=\""$(gimpplugindir)"\"	\
	-DSYSCONFDIR=\""$(gimpsysconfdir)"\"	\
	-DGIMP_PACKAGE=\""@PACKAGE@"\"		\
	-DGIMP_DATA_VERSION=\"$(GIMP_DATA_VERSION)\"		\
	-DGIMP_SYSCONF_VERSION=\"$(GIMP_SYSCONF_VERSION)\"	\
	-DGIMP_PLUGIN_VERSION=\"$(GIMP_PLUGIN_VERSION)\"	\
	-DG_LOG_DOMAIN=\"LibGimpBase\"				\
	-DGIMP_BASE_COMPILATION

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir)

INCLUDES = \
	-I$(top_srcdir)		\
	$(GLIB_CFLAGS)		\
	$(BINRELOC_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc	\
	gimpbase.def

lib_LTLIBRARIES = libgimpbase-2.0.la

# help `make' along by giving another name for the file, which it knows
# how to build
../libgimpbase/gimpversion.h: gimpversion.h
	@:


libgimpbase_sources = \
	gimpbase.h		\
	gimpbaseenums.h		\
	gimpbasetypes.h		\
	gimpbasetypes.c		\
	gimplimits.h		\
	gimpparam.h		\
	gimpversion.h		\
	\
	gimpbase-private.c	\
	gimpbase-private.h	\
	gimpchecks.c           	\
	gimpchecks.h           	\
	gimpcpuaccel.c		\
	gimpcpuaccel.h		\
	gimpdatafiles.c		\
	gimpdatafiles.h		\
	gimpenv.c		\
	gimpenv.h		\
	gimpmemsize.c		\
	gimpmemsize.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimpparasiteio.c	\
	gimpparasiteio.h	\
	gimpprotocol.c		\
	gimpprotocol.h		\
	gimprectangle.c		\
	gimprectangle.h		\
	gimpreloc.c		\
	gimpreloc.h		\
	gimpsignal.c            \
	gimpsignal.h            \
	gimpunit.c		\
	gimpunit.h		\
	gimputils.c            	\
	gimputils.h            	\
	gimpwin32-io.h		\
	gimpwire.c		\
	gimpwire.h

libgimpbase_built_sources = \
	gimpbaseenums.c

libgimpbase_2_0_la_SOURCES = $(libgimpbase_sources) $(libgimpbase_built_sources)


libgimpbaseinclude_HEADERS = \
	gimpbase.h		\
	gimpbaseenums.h		\
	gimpbasetypes.h		\
	gimpcpuaccel.h		\
	gimplimits.h		\
	gimpparam.h		\
	gimpversion.h		\
	\
	gimpchecks.h		\
	gimpdatafiles.h		\
	gimpenv.h		\
	gimpmemsize.h		\
	gimpparasite.h		\
	gimpparasiteio.h	\
	gimprectangle.h		\
	gimpsignal.h		\
	gimpunit.h		\
	gimputils.h


libgimpbase_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpbase_export_symbols)		\
	$(CARBON_LDFLAGS)

libgimpbase_2_0_la_DEPENDENCIES = $(gimpbase_def)

libgimpbase_2_0_la_LIBADD = $(GLIB_LIBS) $(ole32_lib)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

#
# test programs, not to be built by default and never installed
#

TESTS = test-cpu-accel

test_cpu_accel_SOURCES = test-cpu-accel.c

test_cpu_accel_DEPENDENCIES = \
	$(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

test_cpu_accel_LDADD = \
	$(GLIB_LIBS)	\
	$(test_cpu_accel_DEPENDENCIES)


EXTRA_PROGRAMS = test-cpu-accel


#
# rules to generate built sources
#

gen_sources = xgen-bec
CLEANFILES = $(EXTRA_PROGRAMS) $(gen_sources)

$(srcdir)/gimpbaseenums.c: $(srcdir)/gimpbaseenums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#undef GIMP_DISABLE_DEPRECATED\n#include \"gimpbasetypes.h\"\n#include \"libgimp/libgimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libgimp\");\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/gimpbaseenums.h > xgen-bec \
	&& cp xgen-bec $(@F) \
	&& rm -f xgen-bec


DISTCLEANFILES = gimpversion.h
