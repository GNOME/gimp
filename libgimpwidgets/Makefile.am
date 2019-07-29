## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
libgdi32 = -lgdi32
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
gimpwidgets_def = gimpwidgets.def
libgimpwidgets_export_symbols = -export-symbols $(srcdir)/gimpwidgets.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpwidgets-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/gimpwidgets.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpwidgets-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpwidgets.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpwidgets-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpwidgets-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpwidgets-$(GIMP_API_VERSION).lib

gimpwidgets-@GIMP_API_VERSION@.lib: gimpwidgets.def
	lib -name:libgimpwidgets-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpwidgets.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpwidgetsincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpwidgets

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpWidgets\"	\
	-DGIMP_WIDGETS_COMPILATION		\
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

lib_LTLIBRARIES = libgimpwidgets-@GIMP_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libgimpwidgets/Makefile.gi

libgimpwidgets_sources = \
	gimpcolorprofilestore-private.h	\
	gimpcolorscale.c		\
	gimpcolorscale.h		\
	gimpcolorscales.c		\
	gimpcolorscales.h		\
	gimpcolorselect.c		\
	gimpcolorselect.h		\
	gimpcontroller.c		\
	gimpcontroller.h		\
	gimpeevl.c			\
	gimpeevl.h			\
	gimppickbutton-private.h	\
	gimpwidgets-private.c		\
	gimpwidgets-private.h		\
	$(libgimpwidgets_introspectable)

libgimpwidgets_built_sources = \
	gimp-color-picker-cursors.c	\
	gimp-color-picker-cursors.h	\
	gimp-icon-pixbufs.c		\
	gimp-icon-pixbufs.h		\
	gimpwidgetsenums.c		\
	gimpwidgetsmarshal.c		\
	gimpwidgetsmarshal.h

libgimpwidgets_extra_sources = gimpwidgetsmarshal.list

libgimpwidgets_@GIMP_API_VERSION@_la_SOURCES = \
	$(libgimpwidgets_built_sources)	\
	$(libgimpwidgets_sources)

libgimpwidgetsinclude_HEADERS = \
	gimpcolorscale.h		\
	gimpcolorscales.h		\
	gimpcolorselect.h		\
	gimpcontroller.h		\
	$(libgimpwidgets_introspectable_headers)

libgimpwidgets_@GIMP_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpwidgets_export_symbols)	\
	$(framework_cocoa)			\
	$(xnone)

EXTRA_libgimpwidgets_@GIMP_API_VERSION@_la_DEPENDENCIES = $(gimpwidgets_def)

libgimpwidgets_@GIMP_API_VERSION@_la_LIBADD = \
	$(libgimpbase)		\
	$(libgimpcolor)		\
	$(libgimpconfig)	\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(LCMS_LIBS)		\
	$(libm)			\
	$(libgdi32)

BUILT_SOURCES = \
	$(libgimpwidgets_built_sources)

EXTRA_DIST = \
	gimpwidgets.def			\
	$(libgimpwidgets_extra_sources)

#
# platform-dependent source files
#


if PLATFORM_OSX_QUARTZ
libgimpwidgets_sources += gimppickbutton-quartz.c gimppickbutton-quartz.h
AM_CPPFLAGS += "-xobjective-c"
else
libgimpwidgets_sources += \
	gimppickbutton-default.c 	\
	gimppickbutton-default.h	\
	gimppickbutton-kwin.c		\
	gimppickbutton-kwin.h
endif


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = \
	gimp-color-picker-cursors.c	\
	gimp-color-picker-cursors.h	\
	gimp-icon-pixbufs.c		\
	gimp-icon-pixbufs.h		\
	xgen-wec 			\
	xgen-wmh			\
	xgen-wmc

CLEANFILES = $(gen_sources)

xgen-wec: $(srcdir)/gimpwidgetsenums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"gimpwidgetsenums.h\"\n#include \"libgimp/libgimp-intl.h\"" \
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
$(srcdir)/gimpwidgetsenums.c: xgen-wec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi

gimpwidgetsmarshal.h: $(srcdir)/gimpwidgetsmarshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_gimp_widgets_marshal $(srcdir)/gimpwidgetsmarshal.list --header >> xgen-wmh \
	&& (cmp -s xgen-wmh $(@F) || cp xgen-wmh $(@F)) \
	&& rm -f xgen-wmh xgen-wmh~

gimpwidgetsmarshal.c: gimpwidgetsmarshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=_gimp_widgets_marshal $(srcdir)/gimpwidgetsmarshal.list --header --body >> xgen-wmc \
	&& cp xgen-wmc $(@F) \
	&& rm -f xgen-wmc xgen-wmc~

gimp-color-picker-cursors.h: $(top_builddir)/cursors/gimp-color-picker-cursors.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_srcdir)/cursors --generate-header \
	  --target=$@ $(top_builddir)/cursors/gimp-color-picker-cursors.gresource.xml

gimp-color-picker-cursors.c: gimp-color-picker-cursors.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_srcdir)/cursors --generate-source \
	  --target=$@ $(top_builddir)/cursors/gimp-color-picker-cursors.gresource.xml

gimp-icon-pixbufs.h: $(top_builddir)/icons/Color/gimp-icon-pixbufs.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_srcdir)/icons/Color --generate-header \
	  --target=$@ $(top_builddir)/icons/Color/gimp-icon-pixbufs.gresource.xml

gimp-icon-pixbufs.c: gimp-icon-pixbufs.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_srcdir)/icons/Color --generate-source \
	  --target=$@ $(top_builddir)/icons/Color/gimp-icon-pixbufs.gresource.xml


#
# test programs, not installed
#

EXTRA_PROGRAMS = \
	test-preview-area	\
	test-eevl


test_preview_area_SOURCES = test-preview-area.c

test_preview_area_LDADD = \
	$(GTK_LIBS)	\
	$(libgimpbase)	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la


test_eevl_SOURCES = \
	test-eevl.c

test_eevl_LDADD = \
	$(GLIB_LIBS)	\
	$(libgimpcolor)		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la


#
# test programs, not to be built by default and never installed
#

TESTS = test-eevl$(EXEEXT)




CLEANFILES += $(EXTRA_PROGRAMS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
