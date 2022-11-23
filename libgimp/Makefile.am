## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = ./libligma-$(LIGMA_API_VERSION).la

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
if HAVE_EXCHNDL
exchndl = -lexchndl
endif

ligma_def = ligma.def
ligmaui_def = ligmaui.def
libligma_export_symbols = -export-symbols $(srcdir)/ligma.def
libligmaui_export_symbols = -export-symbols $(srcdir)/ligmaui.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligma-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libligmaui-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligma.def $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmaui.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligma-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/libligmaui-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligma.def
	-rm $(DESTDIR)$(libdir)/ligmaui.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligma-$(LIGMA_API_VERSION).lib ligmaui-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligma-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)
	$(INSTALL) ligmaui-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligma-$(LIGMA_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/ligmaui-$(LIGMA_API_VERSION).lib

ligma-@LIGMA_API_VERSION@.lib: ligma.def
	lib -name:libligma-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligma.def -out:$@

ligmaui-@LIGMA_API_VERSION@.lib: ligmaui.def
	lib -name:libligmaui-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmaui.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

ligmaincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligma

AM_CPPFLAGS = \
	-DLIGMADIR=\""$(ligmadir)"\"			\
	-DLIGMA_USER_VERSION=\"$(LIGMA_USER_VERSION)\"	\
	-DG_LOG_DOMAIN=\"LibLigma\"	\
	-DLIGMA_COMPILATION		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/pdb		\
	$(GTK_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GEXIV2_CFLAGS)		\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

lib_LTLIBRARIES = libligma-@LIGMA_API_VERSION@.la libligmaui-@LIGMA_API_VERSION@.la

# Add all sources which are introspectable in this file for reuse.
AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligma/Makefile.gi

libligma_sources = \
	$(libligma_introspectable)	\
	ligmaprocedure-params.h		\
	ligmatilebackendplugin.c 	\
	ligmatilebackendplugin.h 	\
	ligmaunitcache.c			\
	ligmaunitcache.h			\
	stdplugins-intl.h		\
	libligma-intl.h

libligma_private_sources = \
	ligma-debug.c			\
	ligma-debug.h			\
	ligma-private.h			\
	ligma-shm.c			\
	ligma-shm.h			\
	ligmagpparams.c			\
	ligmagpparams.h			\
	ligmaparamspecs-desc.c		\
	ligmaparamspecs-desc.h		\
	ligmapdb-private.h		\
	ligmapdbprocedure.c		\
	ligmapdbprocedure.h		\
	ligmapixbuf.c			\
	ligmapixbuf.h			\
	ligmaplugin-private.h		\
	ligmaprocedure-private.h	        \
	ligmaprocedureconfig-private.h	\
	\
	ligmaunit_pdb.c			\
	ligmaunit_pdb.h			\
	ligmapdb_pdb.c			\
	ligmapdb_pdb.h			\
	ligmaplugin_pdb.c		\
	ligmaplugin_pdb.h

libligma_extra_sources = \
	ligmaenums.c.tail	\
	ligmagpparams-body.c	\
	ligmaparamspecs-body.c

libligma_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligma_private_sources)	\
	$(libligma_sources)

libligmaui_sources = \
	$(libligmaui_introspectable)

libligmaui_built_sources = \
	ligmauimarshal.c			\
	ligmauimarshal.h

libligmaui_extra_sources = ligmauimarshal.list

libligmaui_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligmaui_built_sources)	\
	$(libligmaui_sources)

ligmainclude_HEADERS = \
	$(libligma_introspectable_headers)	\
	ligmaprocedure-params.h			\
	$(libligmaui_introspectable_headers)

libligma_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libligma_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libligma_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligma_def)

libligma_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligmaconfig)	\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(exchndl)		\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(RT_LIBS)

libligmaui_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libligmaui_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libligmaui_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmaui_def)

libligmaui_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligma)		\
	$(libligmaconfig)	\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(libligmamodule)	\
	$(libligmawidgets)	\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)

BUILT_SOURCES = \
	$(libligma_built_sources) \
	$(libligmaui_built_sources)

EXTRA_DIST = \
	COPYING				\
	ligma.def			\
	ligmaui.def			\
	$(libligma_extra_sources)	\
	$(libligmaui_extra_sources)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-cec xgen-umh xgen-umc
CLEANFILES = $(gen_sources)

ligmaenums.c: $(srcdir)/ligmaenums.h $(srcdir)/ligmaenums.c.tail $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include <gegl.h>\n#undef LIGMA_DISABLE_DEPRECATED\n#include \"libligmabase/ligmabase.h\"\n#include \"libligmabase/ligmabase-private.h\"\n#include \"libligmaconfig/ligmaconfigenums.h\"\n#include \"ligmaenums.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libligma\");\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > xgen-cec \
	&& cat $(srcdir)/ligmaenums.c.tail >> xgen-cec \
	&& cp xgen-cec $(@F) \
	&& rm -f xgen-cec

ligmauimarshal.h: $(srcdir)/ligmauimarshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_ligmaui_marshal $(srcdir)/ligmauimarshal.list --header >> xgen-umh \
	&& (cmp -s xgen-umh $(@F) || cp xgen-umh $(@F)) \
	&& rm -f xgen-umh xgen-umh~

ligmauimarshal.c: ligmauimarshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_ligmaui_marshal $(srcdir)/ligmauimarshal.list --prototypes --body >> xgen-umc \
	&& cp xgen-umc $(@F) \
	&& rm -f xgen-umc xgen-umc~

### GObject introspection

if HAVE_INTROSPECTION

-include $(INTROSPECTION_MAKEFILE)
include ../libligmabase/Makefile.gi
include ../libligmacolor/Makefile.gi
include ../libligmaconfig/Makefile.gi
include ../libligmamath/Makefile.gi
include ../libligmamodule/Makefile.gi
include ../libligmathumb/Makefile.gi
include ../libligmawidgets/Makefile.gi

INTROSPECTION_GIRS = Ligma-@LIGMA_API_VERSION@.gir LigmaUi-@LIGMA_API_VERSION@.gir

INTROSPECTION_SCANNER_ARGS = \
	--warn-all			\
	--add-include-path="$(srcdir)"	\
	--add-include-path="$(shell $(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0)"

INTROSPECTION_COMPILER_ARGS = \
	--includedir="$(srcdir)"	\
	--includedir="$(shell $(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0)"

INTROSPECTION_SCANNER_ENV = CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)"

### First GIR: Ligma
Ligma-@LIGMA_API_VERSION@.gir: \
	libligma-@LIGMA_API_VERSION@.la				\
	libligmaui-@LIGMA_API_VERSION@.la				\
	../libligmabase/libligmabase-@LIGMA_API_VERSION@.la	\
	../libligmacolor/libligmacolor-@LIGMA_API_VERSION@.la	\
	../libligmaconfig/libligmaconfig-@LIGMA_API_VERSION@.la	\
	../libligmamath/libligmamath-@LIGMA_API_VERSION@.la	\
	../libligmamodule/libligmamodule-@LIGMA_API_VERSION@.la	\
	../libligmathumb/libligmathumb-@LIGMA_API_VERSION@.la	\
	../libligmawidgets/libligmawidgets-@LIGMA_API_VERSION@.la	\
	Makefile

Ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_EXPORT_PACKAGES = ligma-@LIGMA_API_VERSION@

# The various other GObject-Introspected libraries our API references
# (i.e. types used in parameters or returned values).
Ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_INCLUDES = \
	Babl-0.1	\
	cairo-1.0	\
	GdkPixbuf-2.0	\
	Gegl-0.4	\
	Gio-2.0		\
	GLib-2.0	\
	GObject-2.0	\
	Gtk-3.0

Ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_CFLAGS = \
	-DLIGMA_COMPILATION		\
	-DLIGMA_BASE_COMPILATION		\
	-DLIGMA_COLOR_COMPILATION	\
	-DLIGMA_CONFIG_COMPILATION	\
	-DLIGMA_MATH_COMPILATION		\
	-DLIGMA_MODULE_COMPILATION	\
	-DLIGMA_THUMB_COMPILATION	\
	-DLIGMA_WIDGETS_COMPILATION	\
	$(AM_CFLAGS)			\
	$(AM_CPPFLAGS)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(BABL_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)

Ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_LIBS = \
	../libligma/libligma-@LIGMA_API_VERSION@.la	      	\
	../libligmabase/libligmabase-@LIGMA_API_VERSION@.la  	\
	../libligmacolor/libligmacolor-@LIGMA_API_VERSION@.la	\
	../libligmaconfig/libligmaconfig-@LIGMA_API_VERSION@.la	\
	../libligmamath/libligmamath-@LIGMA_API_VERSION@.la	\
	../libligmamodule/libligmamodule-@LIGMA_API_VERSION@.la

Ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_FILES = 	\
	$(libligma_introspectable)					\
	../libligma/ligmaparamspecs-body.c				\
	$(libligmabase_introspectable)					\
	$(libligmacolor_introspectable)					\
	$(libligmaconfig_introspectable)					\
	$(libligmamath_introspectable)					\
	$(libligmamodule_introspectable)

Ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix Ligma	\
	--symbol-prefix ligma		\
	--c-include="libligma/ligma.h"

### Second GIR: LigmaUi
LigmaUi-@LIGMA_API_VERSION@.gir: \
	Ligma-@LIGMA_API_VERSION@.gir	\
	libligmaui-@LIGMA_API_VERSION@.la				\
	../libligmawidgets/libligmawidgets-@LIGMA_API_VERSION@.la	\
	Makefile

LigmaUi_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_EXPORT_PACKAGES = ligma-ui-@LIGMA_API_VERSION@

# The various other GObject-Introspected libraries our API references
# (i.e. types used in parameters or returned values).
LigmaUi_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_INCLUDES = \
	Babl-0.1	\
	cairo-1.0	\
	GdkPixbuf-2.0	\
	Gegl-0.4	\
	Gio-2.0		\
	GLib-2.0	\
	GObject-2.0	\
	Gtk-3.0

LigmaUi_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_CFLAGS = \
	-DLIGMA_COMPILATION		\
	-DLIGMA_BASE_COMPILATION		\
	-DLIGMA_COLOR_COMPILATION	\
	-DLIGMA_CONFIG_COMPILATION	\
	-DLIGMA_MATH_COMPILATION		\
	-DLIGMA_MODULE_COMPILATION	\
	-DLIGMA_THUMB_COMPILATION	\
	-DLIGMA_WIDGETS_COMPILATION	\
	$(AM_CFLAGS)			\
	$(AM_CPPFLAGS)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(BABL_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)

LigmaUi_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_LIBS = \
	../libligma/libligmaui-@LIGMA_API_VERSION@.la	       	\
	../libligma/libligma-@LIGMA_API_VERSION@.la	       	\
	../libligmawidgets/libligmawidgets-@LIGMA_API_VERSION@.la

LigmaUi_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_FILES = 	\
	$(libligmaui_introspectable)					\
	$(libligmawidgets_introspectable)

LigmaUi_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix Ligma	\
	--symbol-prefix=ligma_ui		\
	--symbol-prefix=ligma		\
	--include-uninstalled=Ligma-@LIGMA_API_VERSION@.gir	\
	--c-include="libligma/ligmaui.h"


girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)


if ENABLE_VAPIGEN

-include $(VAPIGEN_MAKEFILE)

ligma-@LIGMA_API_VERSION@.vapi: Ligma-@LIGMA_API_VERSION@.gir ligma-@LIGMA_API_VERSION@.deps
ligma-ui-@LIGMA_API_VERSION@.vapi: LigmaUi-@LIGMA_API_VERSION@.gir LigmaUi-@LIGMA_API_VERSION@.metadata ligma-ui-@LIGMA_API_VERSION@.deps ligma-@LIGMA_API_VERSION@.vapi

VAPIGEN_VAPIS = ligma-@LIGMA_API_VERSION@.vapi ligma-ui-@LIGMA_API_VERSION@.vapi

# ligma VAPI
ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_DEPS = \
	babl-@BABL_MAJOR_MINOR_VERSION@ \
	cairo \
	gdk-pixbuf-2.0 \
	gegl-@GEGL_MAJOR_MINOR_VERSION@ \
	glib-2.0 \
	gio-2.0 \
	gtk+-3.0
ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_GIRDIRS = $(builddir)
ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_METADATADIRS = $(srcdir)
ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_VAPIDIRS = $(builddir)
ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_FILES = Ligma-@LIGMA_API_VERSION@.gir

ligma-$(LIGMA_API_VERSION).deps: Makefile.am
	$(AM_V_GEN) for pkg in $(ligma_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

# ligma-ui VAPI
ligma_ui_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_DEPS = \
	babl-@BABL_MAJOR_MINOR_VERSION@ \
	cairo \
	gdk-pixbuf-2.0 \
	gegl-@GEGL_MAJOR_MINOR_VERSION@ \
	glib-2.0 \
	gio-2.0 \
	gtk+-3.0 \
	ligma-@LIGMA_API_VERSION@

ligma_ui_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_GIRDIRS = $(builddir)
ligma_ui_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_METADATADIRS = $(srcdir)
ligma_ui_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_VAPIDIRS = $(builddir)
ligma_ui_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_FILES = LigmaUi-@LIGMA_API_VERSION@.gir

ligma-ui-$(LIGMA_API_VERSION).deps: Makefile.am
	$(AM_V_GEN) for pkg in $(ligma_ui_@LIGMA_API_MAJOR_VERSION@_@LIGMA_API_MINOR_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

vapidir = $(datadir)/vala/vapi
vapi_DATA = \
	ligma-@LIGMA_API_VERSION@.vapi \
	ligma-@LIGMA_API_VERSION@.deps \
	ligma-ui-@LIGMA_API_VERSION@.vapi \
	ligma-ui-@LIGMA_API_VERSION@.deps

CLEANFILES += $(vapi_DATA)

EXTRA_DIST += \
	LigmaUi-@LIGMA_API_VERSION@.metadata

endif # ENABLE_VAPIGEN
endif # HAVE_INTROSPECTION
