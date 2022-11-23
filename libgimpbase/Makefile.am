## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
ole32_lib = -lole32
ligmabase_def = ligmabase.def
libligmabase_export_symbols = -export-symbols $(srcdir)/ligmabase.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmabase-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmabase.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmabase-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmabase.def
else
libm = -lm
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmabase-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmabase-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmabase-$(LIGMA_API_VERSION).lib

ligmabase-@LIGMA_API_VERSION@.lib: ligmabase.def
	lib -name:libligmabase-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmabase.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmabaseincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmabase

AM_CPPFLAGS = \
	-DPREFIX=\""$(prefix)"\"				\
	-DLIGMADIR=\""$(ligmadir)"\"				\
	-DLIGMADATADIR=\""$(ligmadatadir)"\"			\
	-DLOCALEDIR=\""$(ligmalocaledir)"\"			\
	-DPLUGINDIR=\""$(ligmaplugindir)"\"			\
	-DLIGMASYSCONFDIR=\""$(ligmasysconfdir)"\"		\
	-DLIGMA_PACKAGE=\""@PACKAGE@"\"				\
	-DLIGMA_DATA_VERSION=\"$(LIGMA_DATA_VERSION)\"		\
	-DLIGMA_USER_VERSION=\"$(LIGMA_USER_VERSION)\"		\
	-DLIGMA_SYSCONF_VERSION=\"$(LIGMA_SYSCONF_VERSION)\"	\
	-DLIGMA_PLUGIN_VERSION=\"$(LIGMA_PLUGIN_VERSION)\"	\
	-DG_LOG_DOMAIN=\"LibLigmaBase\"				\
	-DLIGMA_BASE_COMPILATION					\
	-I$(top_srcdir)						\
	$(GIO_CFLAGS)						\
	$(GEXIV2_CFLAGS)					\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir)

AM_LDFLAGS = \
	$(xnone)

EXTRA_DIST = \
	ligmabase.def

lib_LTLIBRARIES = libligmabase-@LIGMA_API_VERSION@.la

# help `make' along by giving another name for the file, which it knows
# how to build
../libligmabase/ligmaversion.h: ligmaversion.h
	@:

# Add all sources which are introspectable in this file for reuse.
AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmabase/Makefile.gi

libligmabase_sources = \
	$(libligmabase_introspectable)	\
	ligmabase.h			\
	ligmacompatenums.h		\
	\
	ligmabase-private.c		\
	ligmabase-private.h		\
	ligmaprotocol.c			\
	ligmaprotocol.h			\
	ligmareloc.c			\
	ligmareloc.h			\
	ligmawin32-io.h			\
	ligmawire.c			\
	ligmawire.h

libligmabase_built_sources = \
	ligmabaseenums.c		\
	ligmacompatenums.c

libligmabase_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligmabase_sources)	\
	$(libligmabase_built_sources)


libligmabaseinclude_HEADERS = \
	ligmabase.h		\
	ligmabaseenums.h		\
	ligmabasetypes.h		\
	ligmacpuaccel.h		\
	ligmalimits.h		\
	ligmaparamspecs.h	\
	ligmaversion.h		\
	\
	ligmachecks.h		\
	ligmaenv.h		\
	ligmamemsize.h		\
	ligmametadata.h		\
	ligmaparasite.h		\
	ligmaparasiteio.h	\
	ligmarectangle.h		\
	ligmasignal.h		\
	ligmaunit.h		\
	ligmautils.h		\
	ligmavaluearray.h

libligmabase_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libligmabase_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libligmabase_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmabase_def)

libligmabase_@LIGMA_API_VERSION@_la_LIBADD = \
	$(GIO_LIBS)	\
	$(GEXIV2_LIBS)	\
	$(libm)		\
	$(ole32_lib)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

#
# test programs, not to be built by default and never installed
#

TESTS = test-cpu-accel

test_cpu_accel_SOURCES = test-cpu-accel.c

test_cpu_accel_DEPENDENCIES = \
	$(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

test_cpu_accel_LDADD = \
	$(GLIB_LIBS)	\
	$(test_cpu_accel_DEPENDENCIES)


EXTRA_PROGRAMS = test-cpu-accel


#
# rules to generate built sources
#

gen_sources = stamp-ligmabaseenums.h stamp-ligmacompatenums.h xgen-bec xgen-cec
CLEANFILES = $(EXTRA_PROGRAMS) $(gen_sources)

xgen-bec: $(srcdir)/ligmabaseenums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-ligmabaseenums.h\"\n#include \"config.h\"\n#include <glib-object.h>\n#undef LIGMA_DISABLE_DEPRECATED\n#include \"ligmabasetypes.h\"\n#include \"libligma/libligma-intl.h\"\n#include \"ligmabaseenums.h\"\n" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libligma\");\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-ligmabaseenums.h: xgen-bec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/ligmabaseenums.c: xgen-bec stamp-ligmabaseenums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi

xgen-cec: $(srcdir)/ligmacompatenums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-ligmacompatenums.h\"\n#include \"config.h\"\n#include <glib-object.h>\n#include \"ligmabasetypes.h\"\n#include \"ligmacompatenums.h\"\n#include \"libligma/libligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libligma\");\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-ligmacompatenums.h: xgen-cec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/ligmacompatenums.c: xgen-cec stamp-ligmacompatenums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi

DISTCLEANFILES = ligmaversion.h
