## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
libgdi32 = -lgdi32
libmscms = -lmscms
else
libm = -lm
endif

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
ligmawidgets_def = ligmawidgets.def
libligmawidgets_export_symbols = -export-symbols $(srcdir)/ligmawidgets.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmawidgets-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmawidgets.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmawidgets-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmawidgets.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmawidgets-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmawidgets-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmawidgets-$(LIGMA_API_VERSION).lib

ligmawidgets-@LIGMA_API_VERSION@.lib: ligmawidgets.def
	lib -name:libligmawidgets-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmawidgets.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmawidgetsincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmawidgets

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibLigmaWidgets\"	\
	-DLIGMA_WIDGETS_COMPILATION		\
	-DDATAROOTDIR=\""$(datarootdir)"\"	\
	-I$(top_srcdir)				\
	$(GEGL_CFLAGS)				\
	$(GTK_CFLAGS)				\
	$(LCMS_CFLAGS)				\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

lib_LTLIBRARIES = libligmawidgets-@LIGMA_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmawidgets/Makefile.gi

libligmawidgets_sources = \
	ligmacolorprofilestore-private.h	\
	ligmacolorscale.c		\
	ligmacolorscale.h		\
	ligmacolorscales.c		\
	ligmacolorscales.h		\
	ligmacolorselect.c		\
	ligmacolorselect.h		\
	ligmacontroller.c		\
	ligmacontroller.h		\
	ligmaeevl.c			\
	ligmaeevl.h			\
	ligmapickbutton-private.h	\
	ligmawidgets-private.c		\
	ligmawidgets-private.h		\
	$(libligmawidgets_introspectable)

libligmawidgets_built_sources = \
	ligma-color-picker-cursors.c	\
	ligma-color-picker-cursors.h	\
	ligma-icon-pixbufs.c		\
	ligma-icon-pixbufs.h		\
	ligmawidgetsenums.c		\
	ligmawidgetsmarshal.c		\
	ligmawidgetsmarshal.h

libligmawidgets_extra_sources = ligmawidgetsmarshal.list

libligmawidgets_@LIGMA_API_VERSION@_la_SOURCES = \
	$(libligmawidgets_built_sources)	\
	$(libligmawidgets_sources)

libligmawidgetsinclude_HEADERS = \
	ligmacolorscale.h		\
	ligmacolorscales.h		\
	ligmacolorselect.h		\
	ligmacontroller.h		\
	$(libligmawidgets_introspectable_headers)

libligmawidgets_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libligmawidgets_export_symbols)	\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libligmawidgets_@LIGMA_API_VERSION@_la_DEPENDENCIES = $(ligmawidgets_def)

libligmawidgets_@LIGMA_API_VERSION@_la_LIBADD = \
	$(libligmabase)		\
	$(libligmacolor)		\
	$(libligmaconfig)	\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(LCMS_LIBS)		\
	$(libm)			\
	$(libgdi32)		\
	$(libmscms)

BUILT_SOURCES = \
	$(libligmawidgets_built_sources)

EXTRA_DIST = \
	ligmawidgets.def			\
	$(libligmawidgets_extra_sources)

#
# platform-dependent source files
#


if PLATFORM_OSX_QUARTZ
libligmawidgets_sources += ligmapickbutton-quartz.c ligmapickbutton-quartz.h
AM_CPPFLAGS += "-xobjective-c"
else

if PLATFORM_WIN32
libligmawidgets_sources += ligmapickbutton-win32.c ligmapickbutton-win32.h
else
libligmawidgets_sources += \
	ligmapickbutton-default.c 	\
	ligmapickbutton-default.h	\
	ligmapickbutton-kwin.c		\
	ligmapickbutton-kwin.h		\
	ligmapickbutton-xdg.c		\
	ligmapickbutton-xdg.h
endif

endif


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = \
	ligma-color-picker-cursors.c	\
	ligma-color-picker-cursors.h	\
	ligma-icon-pixbufs.c		\
	ligma-icon-pixbufs.h		\
	stamp-ligmawidgetsenums.h	\
	xgen-wec 			\
	xgen-wmh			\
	xgen-wmc

CLEANFILES = $(gen_sources)

xgen-wec: $(srcdir)/ligmawidgetsenums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-ligmawidgetsenums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"ligmawidgetsenums.h\"\n#include \"libligma/libligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libligma\");\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-ligmawidgetsenums.h: xgen-wec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/ligmawidgetsenums.c: xgen-wec stamp-ligmawidgetsenums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi

ligmawidgetsmarshal.h: $(srcdir)/ligmawidgetsmarshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_ligma_widgets_marshal $(srcdir)/ligmawidgetsmarshal.list --header >> xgen-wmh \
	&& (cmp -s xgen-wmh $(@F) || cp xgen-wmh $(@F)) \
	&& rm -f xgen-wmh xgen-wmh~

ligmawidgetsmarshal.c: ligmawidgetsmarshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_ligma_widgets_marshal $(srcdir)/ligmawidgetsmarshal.list --prototypes --body >> xgen-wmc \
	&& cp xgen-wmc $(@F) \
	&& rm -f xgen-wmc xgen-wmc~

ligma-color-picker-cursors.h: $(top_builddir)/cursors/ligma-color-picker-cursors.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --internal \
	  --sourcedir=$(top_srcdir)/cursors --generate-header \
	  --target=$@ $(top_builddir)/cursors/ligma-color-picker-cursors.gresource.xml

ligma-color-picker-cursors.c: ligma-color-picker-cursors.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --internal \
	  --sourcedir=$(top_srcdir)/cursors --generate-source \
	  --target=$@ $(top_builddir)/cursors/ligma-color-picker-cursors.gresource.xml

ligma-icon-pixbufs.h: $(top_builddir)/icons/Color/ligma-icon-pixbufs.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --internal \
	  --sourcedir=$(top_builddir)/icons/Color --generate-header \
	  --target=$@ $(top_builddir)/icons/Color/ligma-icon-pixbufs.gresource.xml

ligma-icon-pixbufs.c: ligma-icon-pixbufs.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --internal \
	  --sourcedir=$(top_builddir)/icons/Color --generate-source \
	  --target=$@ $(top_builddir)/icons/Color/ligma-icon-pixbufs.gresource.xml


#
# test programs, not installed
#

EXTRA_PROGRAMS = \
	test-preview-area	\
	test-eevl


test_preview_area_SOURCES = test-preview-area.c

test_preview_area_LDADD = \
	$(GTK_LIBS)	\
	$(libligmabase)	\
	$(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la


test_eevl_SOURCES = \
	test-eevl.c

test_eevl_LDADD = \
	$(GLIB_LIBS)	\
	$(libligmacolor)		\
	$(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la


#
# test programs, not to be built by default and never installed
#

TESTS = test-eevl$(EXEEXT)




CLEANFILES += $(EXTRA_PROGRAMS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
