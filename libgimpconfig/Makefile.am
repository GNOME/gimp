## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpconfig_def = gimpconfig.def
libgimpconfig_export_symbols = -export-symbols $(srcdir)/gimpconfig.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpconfig-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimpconfig.def $(DESTDIR)$(libdir)

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
	-DGIMP_CONFIG_COMPILATION		\
	-I$(top_srcdir)				\
	$(GIO_UNIX_CFLAGS)			\
	$(GIO_WINDOWS_CFLAGS)			\
	$(GEGL_CFLAGS)				\
	$(CAIRO_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	-I$(includedir)

EXTRA_DIST = \
	gimpconfig.def

lib_LTLIBRARIES = libgimpconfig-@GIMP_API_VERSION@.la

libgimpconfig_sources = 		\
	gimpconfig.h			\
	gimpconfigenums.h		\
	gimpconfigtypes.h		\
	\
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
	gimpconfig-utils.c		\
	gimpconfig-utils.h		\
	gimpconfigwriter.c		\
	gimpconfigwriter.h		\
	gimpscanner.c			\
	gimpscanner.h			\
	gimpcolorconfig.c		\
	gimpcolorconfig.h

libgimpconfig_built_sources = \
	gimpconfigenums.c

libgimpconfig_@GIMP_API_VERSION@_la_SOURCES = \
	$(libgimpconfig_sources)	\
	$(libgimpconfig_built_sources)


libgimpconfiginclude_HEADERS = \
	gimpconfig.h			\
	gimpconfigenums.h		\
	gimpconfigtypes.h		\
	gimpconfig-iface.h		\
	gimpconfig-deserialize.h	\
	gimpconfig-error.h		\
	gimpconfig-params.h		\
	gimpconfig-path.h		\
	gimpconfig-serialize.h		\
	gimpconfig-utils.h		\
	gimpconfigwriter.h		\
	gimpscanner.h			\
	gimpcolorconfig.h


libgimpconfig_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpconfig_export_symbols)

EXTRA_libgimpconfig_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimpconfig_def)

libgimpconfig_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimpbase)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
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

gen_sources = xgen-cec
CLEANFILES = $(gen_sources)

xgen-cec: $(srcdir)/gimpconfigenums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"gimpconfigenums.h\"\n#include \"libgimp/libgimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libgimp\");\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/gimpconfigenums.c: xgen-cec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
