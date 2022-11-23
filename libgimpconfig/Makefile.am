## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
ligmaconfig_def = ligmaconfig.def
libligmaconfig_export_symbols = -export-symbols $(srcdir)/ligmaconfig.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmaconfig-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmaconfig.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmaconfig-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmaconfig.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmaconfig-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmaconfig-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmaconfig-$(LIGMA_API_VERSION).lib

ligmaconfig-@LIGMA_API_VERSION@.lib: ligmaconfig.def
	lib -name:libligmaconfig-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmaconfig.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmaconfigincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmaconfig

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibLigmaConfig\"	\
	-DLIGMA_CONFIG_COMPILATION		\
	-I$(top_srcdir)				\
	$(GIO_UNIX_CFLAGS)			\
	$(GIO_WINDOWS_CFLAGS)			\
	$(GEGL_CFLAGS)				\
	$(CAIRO_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	-I$(includedir)

EXTRA_DIST = \
	ligmaconfig.def

lib_LTLIBRARIES = libligmaconfig-@LIGMA_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmaconfig/Makefile.gi

libligmaconfig_sources = 		\
	ligmaconfig.h			\
	$(libligmaconfig_introspectable)

libligmaconfig_built_sources = \
	ligmaconfigenums.c

libligmaconfig_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligmaconfig_sources)	\
	$(libligmaconfig_built_sources)

libligmaconfiginclude_HEADERS = \
	ligmaconfig.h			\
	$(libligmaconfig_introspectable_headers)

libligmaconfig_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libligmaconfig_export_symbols)

EXTRA_libligmaconfig_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmaconfig_def)

libligmaconfig_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligmabase)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(GIO_UNIX_LIBS)	\
	$(GIO_WINDOWS_LIBS)	\
	$(GEGL_LIBS)		\
	$(CAIRO_LIBS)		\
	$(GDK_PIXBUF_LIBS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib


#
# rules to generate built sources
#

gen_sources = stamp-ligmaconfigenums.h xgen-cec
CLEANFILES = $(gen_sources)

xgen-cec: $(srcdir)/ligmaconfigenums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-ligmaconfigenums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"ligmaconfigenums.h\"\n#include \"libligma/libligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libligma\");\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-ligmaconfigenums.h: xgen-cec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/ligmaconfigenums.c: xgen-cec stamp-ligmaconfigenums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
