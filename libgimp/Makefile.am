## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = ./libgimp-$(GIMP_API_VERSION).la

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

gimp_def = gimp.def
gimpui_def = gimpui.def
libgimp_export_symbols = -export-symbols $(srcdir)/gimp.def
libgimpui_export_symbols = -export-symbols $(srcdir)/gimpui.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimp-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libgimpui-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimp.def $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimpui.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimp-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/libgimpui-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimp.def
	-rm $(DESTDIR)$(libdir)/gimpui.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimp-$(GIMP_API_VERSION).lib gimpui-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimp-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)
	$(INSTALL) gimpui-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimp-$(GIMP_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/gimpui-$(GIMP_API_VERSION).lib

gimp-@GIMP_API_VERSION@.lib: gimp.def
	lib -name:libgimp-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimp.def -out:$@

gimpui-@GIMP_API_VERSION@.lib: gimpui.def
	lib -name:libgimpui-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpui.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

gimpincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimp

AM_CPPFLAGS = \
	-DGIMPDIR=\""$(gimpdir)"\"			\
	-DGIMP_USER_VERSION=\"$(GIMP_USER_VERSION)\"	\
	-DG_LOG_DOMAIN=\"LibGimp\"	\
	-DGIMP_COMPILATION		\
	-I$(top_srcdir)			\
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

lib_LTLIBRARIES = libgimp-@GIMP_API_VERSION@.la libgimpui-@GIMP_API_VERSION@.la

# Add all sources which are introspectable in this file for reuse.
AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libgimp/Makefile.gi

libgimp_sources = \
	$(libgimp_introspectable)	\
	gimpprocedure-params.h		\
	gimptilebackendplugin.c 	\
	gimptilebackendplugin.h 	\
	gimpunitcache.c			\
	gimpunitcache.h			\
	stdplugins-intl.h		\
	libgimp-intl.h

libgimp_private_sources = \
	gimp-debug.c			\
	gimp-debug.h			\
	gimp-private.h			\
	gimp-shm.c			\
	gimp-shm.h			\
	gimpgpparams.c			\
	gimpgpparams.h			\
	gimpparamspecs-desc.c		\
	gimpparamspecs-desc.h		\
	gimppdb-private.h		\
	gimppdbprocedure.c		\
	gimppdbprocedure.h		\
	gimppixbuf.c			\
	gimppixbuf.h			\
	gimpplugin-private.h		\
	gimpprocedure-private.h	        \
	gimpprocedureconfig-private.h	\
	\
	gimpunit_pdb.c			\
	gimpunit_pdb.h			\
	gimppdb_pdb.c			\
	gimppdb_pdb.h			\
	gimpplugin_pdb.c		\
	gimpplugin_pdb.h

libgimp_extra_sources = \
	gimpenums.c.tail	\
	gimpgpparams-body.c	\
	gimpparamspecs-body.c

libgimp_@GIMP_API_VERSION@_la_SOURCES = \
	$(libgimp_private_sources)	\
	$(libgimp_sources)

libgimpui_sources = \
	$(libgimpui_introspectable)

libgimpui_built_sources = \
	gimpuimarshal.c			\
	gimpuimarshal.h

libgimpui_extra_sources = gimpuimarshal.list

libgimpui_@GIMP_API_VERSION@_la_SOURCES = \
	$(libgimpui_built_sources)	\
	$(libgimpui_sources)

gimpinclude_HEADERS = \
	$(libgimp_introspectable_headers)	\
	gimpprocedure-params.h			\
	$(libgimpui_introspectable_headers)

libgimp_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimp_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libgimp_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimp_def)

libgimp_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(exchndl)		\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(RT_LIBS)

libgimpui_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimpui_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libgimpui_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimpui_def)

libgimpui_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimp)		\
	$(libgimpwidgets)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(libgimpmodule)	\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)

BUILT_SOURCES = \
	$(libgimp_built_sources) \
	$(libgimpui_built_sources)

EXTRA_DIST = \
	COPYING				\
	gimp.def			\
	gimpui.def			\
	$(libgimp_extra_sources)	\
	$(libgimpui_extra_sources)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-cec xgen-umh xgen-umc
CLEANFILES = $(gen_sources)

gimpenums.c: $(srcdir)/gimpenums.h $(srcdir)/gimpenums.c.tail $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include <gegl.h>\n#undef GIMP_DISABLE_DEPRECATED\n#include \"libgimpbase/gimpbase.h\"\n#include \"libgimpbase/gimpbase-private.h\"\n#include \"libgimpconfig/gimpconfigenums.h\"\n#include \"gimpenums.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libgimp\");\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > xgen-cec \
	&& cat $(srcdir)/gimpenums.c.tail >> xgen-cec \
	&& cp xgen-cec $(@F) \
	&& rm -f xgen-cec

gimpuimarshal.h: $(srcdir)/gimpuimarshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_gimpui_marshal $(srcdir)/gimpuimarshal.list --header >> xgen-umh \
	&& (cmp -s xgen-umh $(@F) || cp xgen-umh $(@F)) \
	&& rm -f xgen-umh xgen-umh~

gimpuimarshal.c: gimpuimarshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_gimpui_marshal $(srcdir)/gimpuimarshal.list --header --body >> xgen-umc \
	&& cp xgen-umc $(@F) \
	&& rm -f xgen-umc xgen-umc~

### GObject introspection

-include $(INTROSPECTION_MAKEFILE)
include ../libgimpbase/Makefile.gi
include ../libgimpcolor/Makefile.gi
include ../libgimpconfig/Makefile.gi
include ../libgimpmath/Makefile.gi
include ../libgimpmodule/Makefile.gi
include ../libgimpthumb/Makefile.gi
include ../libgimpwidgets/Makefile.gi

INTROSPECTION_GIRS = Gimp-@GIMP_API_VERSION@.gir GimpUi-@GIMP_API_VERSION@.gir

INTROSPECTION_SCANNER_ARGS = \
	--warn-all			\
	--add-include-path="$(srcdir)"	\
	--add-include-path="$(shell $(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0)"

INTROSPECTION_COMPILER_ARGS = \
	--includedir="$(srcdir)"	\
	--includedir="$(shell $(PKG_CONFIG) --define-variable=datadir="$(datadir)" --variable=girdir gobject-introspection-1.0)"

INTROSPECTION_SCANNER_ENV = CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" CPPFLAGS="$(CPPFLAGS)" CXXFLAGS="$(CXXFLAGS)"

### First GIR: Gimp
Gimp-@GIMP_API_VERSION@.gir: \
	libgimp-@GIMP_API_VERSION@.la				\
	libgimpui-@GIMP_API_VERSION@.la				\
	../libgimpbase/libgimpbase-@GIMP_API_VERSION@.la	\
	../libgimpcolor/libgimpcolor-@GIMP_API_VERSION@.la	\
	../libgimpconfig/libgimpconfig-@GIMP_API_VERSION@.la	\
	../libgimpmath/libgimpmath-@GIMP_API_VERSION@.la	\
	../libgimpmodule/libgimpmodule-@GIMP_API_VERSION@.la	\
	../libgimpthumb/libgimpthumb-@GIMP_API_VERSION@.la	\
	../libgimpwidgets/libgimpwidgets-@GIMP_API_VERSION@.la	\
	Makefile

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_EXPORT_PACKAGES = gimp-@GIMP_API_VERSION@

# The various other GObject-Introspected libraries our API references
# (i.e. types used in parameters or returned values).
Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_INCLUDES = \
	Babl-0.1	\
	cairo-1.0	\
	GdkPixbuf-2.0	\
	Gegl-0.4	\
	Gio-2.0		\
	GLib-2.0	\
	GObject-2.0	\
	Gtk-3.0

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_CFLAGS = \
	-DGIMP_COMPILATION		\
	-DGIMP_BASE_COMPILATION		\
	-DGIMP_COLOR_COMPILATION	\
	-DGIMP_CONFIG_COMPILATION	\
	-DGIMP_MATH_COMPILATION		\
	-DGIMP_MODULE_COMPILATION	\
	-DGIMP_THUMB_COMPILATION	\
	-DGIMP_WIDGETS_COMPILATION	\
	$(AM_CFLAGS)			\
	$(AM_CPPFLAGS)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(BABL_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_LIBS = \
	../libgimp/libgimp-@GIMP_API_VERSION@.la	      	\
	../libgimpbase/libgimpbase-@GIMP_API_VERSION@.la  	\
	../libgimpcolor/libgimpcolor-@GIMP_API_VERSION@.la	\
	../libgimpconfig/libgimpconfig-@GIMP_API_VERSION@.la	\
	../libgimpmath/libgimpmath-@GIMP_API_VERSION@.la	\
	../libgimpmodule/libgimpmodule-@GIMP_API_VERSION@.la

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_FILES = 	\
	$(libgimp_introspectable)					\
	../libgimp/gimpparamspecs-body.c				\
	$(libgimpbase_introspectable)					\
	$(libgimpcolor_introspectable)					\
	$(libgimpconfig_introspectable)					\
	$(libgimpmath_introspectable)					\
	$(libgimpmodule_introspectable)

Gimp_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix Gimp	\
	--symbol-prefix gimp	\
	--c-include="libgimp/gimp.h"

### Second GIR: GimpUi
GimpUi-@GIMP_API_VERSION@.gir: \
	Gimp-@GIMP_API_VERSION@.gir	\
	libgimpui-@GIMP_API_VERSION@.la				\
	../libgimpwidgets/libgimpwidgets-@GIMP_API_VERSION@.la	\
	Makefile

GimpUi_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_EXPORT_PACKAGES = gimp-ui-@GIMP_API_VERSION@

# The various other GObject-Introspected libraries our API references
# (i.e. types used in parameters or returned values).
GimpUi_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_INCLUDES = \
	Babl-0.1	\
	cairo-1.0	\
	GdkPixbuf-2.0	\
	Gegl-0.4	\
	Gio-2.0		\
	GLib-2.0	\
	GObject-2.0	\
	Gtk-3.0

GimpUi_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_CFLAGS = \
	-DGIMP_COMPILATION		\
	-DGIMP_BASE_COMPILATION		\
	-DGIMP_COLOR_COMPILATION	\
	-DGIMP_CONFIG_COMPILATION	\
	-DGIMP_MATH_COMPILATION		\
	-DGIMP_MODULE_COMPILATION	\
	-DGIMP_THUMB_COMPILATION	\
	-DGIMP_WIDGETS_COMPILATION	\
	$(AM_CFLAGS)			\
	$(AM_CPPFLAGS)			\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(BABL_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)

GimpUi_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_LIBS = \
	../libgimp/libgimpui-@GIMP_API_VERSION@.la	       	\
	../libgimpwidgets/libgimpwidgets-@GIMP_API_VERSION@.la

GimpUi_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_FILES = 	\
	$(libgimpui_introspectable)					\
	$(libgimpwidgets_introspectable)

GimpUi_@GIMP_API_MAJOR_VERSION@_@GIMP_API_MINOR_VERSION@_gir_SCANNERFLAGS = \
	--identifier-prefix Gimp	\
	--symbol-prefix gimp	\
	--include-uninstalled=Gimp-@GIMP_API_VERSION@.gir	\
	--c-include="libgimp/gimpui.h"


girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)


if ENABLE_VAPIGEN

-include $(VAPIGEN_MAKEFILE)

gimp-@GIMP_API_MAJOR_VERSION@.vapi: Gimp-@GIMP_API_VERSION@.gir gimp-@GIMP_API_MAJOR_VERSION@.deps
gimp-ui-@GIMP_API_MAJOR_VERSION@.vapi: GimpUi-@GIMP_API_VERSION@.gir GimpUi-@GIMP_API_VERSION@.metadata gimp-ui-@GIMP_API_MAJOR_VERSION@.deps gimp-@GIMP_API_MAJOR_VERSION@.vapi

VAPIGEN_VAPIS = gimp-@GIMP_API_MAJOR_VERSION@.vapi gimp-ui-@GIMP_API_MAJOR_VERSION@.vapi

# gimp VAPI
gimp_@GIMP_API_MAJOR_VERSION@_vapi_DEPS = \
	babl-@BABL_MAJOR_MINOR_VERSION@ \
	cairo \
	gdk-pixbuf-2.0 \
	gegl-@GEGL_MAJOR_MINOR_VERSION@ \
	glib-2.0 \
	gio-2.0 \
	gtk+-3.0
gimp_@GIMP_API_MAJOR_VERSION@_vapi_GIRDIRS = $(builddir)
gimp_@GIMP_API_MAJOR_VERSION@_vapi_METADATADIRS = $(srcdir)
gimp_@GIMP_API_MAJOR_VERSION@_vapi_VAPIDIRS = $(builddir)
gimp_@GIMP_API_MAJOR_VERSION@_vapi_FILES = Gimp-@GIMP_API_VERSION@.gir

gimp-$(GIMP_API_MAJOR_VERSION).deps: Makefile.am
	$(AM_V_GEN) for pkg in $(gimp_@GIMP_API_MAJOR_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

# gimp-ui VAPI
gimp_ui_@GIMP_API_MAJOR_VERSION@_vapi_DEPS = \
	babl-@BABL_MAJOR_MINOR_VERSION@ \
	cairo \
	gdk-pixbuf-2.0 \
	gegl-@GEGL_MAJOR_MINOR_VERSION@ \
	glib-2.0 \
	gio-2.0 \
	gtk+-3.0 \
	gimp-@GIMP_API_MAJOR_VERSION@

gimp_ui_@GIMP_API_MAJOR_VERSION@_vapi_GIRDIRS = $(builddir)
gimp_ui_@GIMP_API_MAJOR_VERSION@_vapi_METADATADIRS = $(srcdir)
gimp_ui_@GIMP_API_MAJOR_VERSION@_vapi_VAPIDIRS = $(builddir)
gimp_ui_@GIMP_API_MAJOR_VERSION@_vapi_FILES = GimpUi-@GIMP_API_VERSION@.gir

gimp-ui-$(GIMP_API_MAJOR_VERSION).deps: Makefile.am
	$(AM_V_GEN) for pkg in $(gimp_ui_@GIMP_API_MAJOR_VERSION@_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

vapidir = $(datadir)/vala/vapi
vapi_DATA = \
	gimp-@GIMP_API_MAJOR_VERSION@.vapi \
	gimp-@GIMP_API_MAJOR_VERSION@.deps \
	gimp-ui-@GIMP_API_MAJOR_VERSION@.vapi \
	gimp-ui-@GIMP_API_MAJOR_VERSION@.deps

CLEANFILES += $(vapi_DATA)

EXTRA_DIST += \
	GimpUi-@GIMP_API_VERSION@.metadata

endif # ENABLE_VAPIGEN
