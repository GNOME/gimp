## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_WIN32
else
libm = -lm
endif

if OS_WIN32
gimpwidgets_def = gimpwidgets.def
libgimpwidgets_export_symbols = -export-symbols gimpwidgets.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpwidgets-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpwidgets.def $(DESTDIR)$(libdir)

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
	-DG_LOG_DOMAIN=\"LibGimpWidgets\"			\
	@GTHREAD_CFLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

lib_LTLIBRARIES = libgimpwidgets-2.0.la

libgimpwidgets_2_0_la_sources = \
	gimpwidgets.c		\
	gimpwidgets.h		\
	gimpwidgetsenums.h	\
	gimpwidgetstypes.h	\
	gimpbrowser.c		\
	gimpbrowser.h		\
	gimpbutton.c		\
	gimpbutton.h		\
	gimpcellrenderercolor.c	\
	gimpcellrenderercolor.h	\
	gimpcellrenderertoggle.c\
	gimpcellrenderertoggle.h\
	gimpchainbutton.c	\
	gimpchainbutton.h	\
	gimpcolorarea.c		\
	gimpcolorarea.h		\
	gimpcolorbutton.c	\
	gimpcolorbutton.h	\
	gimpcolordisplay.c	\
	gimpcolordisplay.h	\
	gimpcolordisplaystack.c	\
	gimpcolordisplaystack.h	\
	gimpenumwidgets.c	\
	gimpenumwidgets.h	\
	gimpcolorhexentry.c	\
	gimpcolorhexentry.h	\
	gimpcolornotebook.c	\
	gimpcolornotebook.h	\
	gimpcolorscale.c	\
	gimpcolorscale.h	\
	gimpcolorscales.c	\
	gimpcolorscales.h	\
	gimpcolorselector.c	\
	gimpcolorselector.h	\
	gimpcolorselect.c	\
	gimpcolorselect.h	\
	gimpcolorselection.c	\
	gimpcolorselection.h	\
	gimpcontroller.c	\
	gimpcontroller.h	\
	gimpdialog.c		\
	gimpdialog.h		\
	gimpenumstore.c		\
	gimpenumstore.h		\
	gimpenumcombobox.c	\
	gimpenumcombobox.h	\
	gimpenumlabel.c		\
	gimpenumlabel.h		\
	gimpfileentry.c		\
	gimpfileentry.h		\
	gimpframe.c		\
	gimpframe.h		\
	gimphelpui.c		\
	gimphelpui.h		\
	gimphintbox.c		\
	gimphintbox.h		\
	gimpintcombobox.c	\
	gimpintcombobox.h	\
	gimpintstore.c		\
	gimpintstore.h		\
	gimpmemsizeentry.c	\
	gimpmemsizeentry.h	\
	gimpoldwidgets.c	\
	gimpoldwidgets.h	\
	gimpoffsetarea.c	\
	gimpoffsetarea.h	\
	gimppageselector.c	\
	gimppageselector.h	\
	gimppatheditor.c	\
	gimppatheditor.h	\
	gimppickbutton.c	\
	gimppickbutton.h	\
	gimppixmap.c		\
	gimppixmap.h		\
	gimppreview.c		\
	gimppreview.h		\
	gimppreviewarea.c	\
	gimppreviewarea.h	\
	gimppropwidgets.c	\
	gimppropwidgets.h	\
	gimpquerybox.c		\
	gimpquerybox.h		\
	gimpratioentry.c	\
	gimpratioentry.h	\
	gimpscrolledpreview.c	\
	gimpscrolledpreview.h	\
	gimpsizeentry.c		\
	gimpsizeentry.h		\
	gimpstock.c		\
	gimpstock.h		\
	gimpstringcombobox.c	\
	gimpstringcombobox.h	\
	gimpunitmenu.c		\
	gimpunitmenu.h		\
	gimpzoommodel.c		\
	gimpzoommodel.h		\
	gimpwidgets-private.c	\
	gimpwidgets-private.h

libgimpwidgets_2_0_la_built_sources = \
	gimpwidgetsenums.c	\
	gimpwidgetsmarshal.c	\
	gimpwidgetsmarshal.h	\
	gimp-wilber-pixbufs.h	

libgimpwidgets_2_0_la_extra_sources = gimpwidgetsmarshal.list

libgimpwidgets_2_0_la_SOURCES = 		\
	$(libgimpwidgets_2_0_la_built_sources)	\
	$(libgimpwidgets_2_0_la_sources)

libgimpwidgetsinclude_HEADERS = \
	gimpwidgets.h		\
	gimpwidgetsenums.h	\
	gimpwidgetstypes.h	\
	gimpbrowser.h		\
	gimpbutton.h		\
	gimpcellrenderercolor.h	\
	gimpcellrenderertoggle.h\
	gimpchainbutton.h	\
	gimpcolorarea.h		\
	gimpcolorbutton.h	\
	gimpcolordisplay.h	\
	gimpcolordisplaystack.h	\
	gimpcolorhexentry.h	\
	gimpcolornotebook.h	\
	gimpcolorscale.h	\
	gimpcolorscales.h	\
	gimpcolorselector.h	\
	gimpcolorselect.h	\
	gimpcolorselection.h	\
	gimpcontroller.h	\
	gimpdialog.h		\
	gimpenumcombobox.h	\
	gimpenumlabel.h		\
	gimpenumstore.h		\
	gimpenumwidgets.h	\
	gimpfileentry.h		\
	gimpframe.h		\
	gimphelpui.h		\
	gimphintbox.h		\
	gimpintcombobox.h	\
	gimpintstore.h		\
	gimpmemsizeentry.h	\
	gimpoldwidgets.h	\
	gimpoffsetarea.h	\
	gimppageselector.h	\
	gimppatheditor.h	\
	gimppickbutton.h	\
	gimppixmap.h		\
	gimppreview.h		\
	gimppreviewarea.h	\
	gimppropwidgets.h	\
	gimpquerybox.h		\
	gimpratioentry.h	\
	gimpscrolledpreview.h	\
	gimpsizeentry.h		\
	gimpstock.h		\
	gimpstringcombobox.h	\
	gimpunitmenu.h		\
	gimpzoommodel.h

libgimpwidgets_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined) 			\
	$(libgimpwidgets_export_symbols)

libgimpwidgets_2_0_la_LIBADD = \
	$(libgimpcolor) \
	$(libgimpconfig) \
	$(libgimpbase)	\
	$(GTK_LIBS) 	\
	$(libm)

libgimpwidgets_2_0_la_DEPENDENCIES = $(gimpwidgets_def)


## Wilber icons for the window managaer, compiled in

WILBER_IMAGES = \
	wilber-16.png	\
	wilber-32.png	\
	wilber-48.png	\
	wilber-64.png

WILBER_VARIABLES = \
	wilber_16  $(srcdir)/wilber-16.png	\
	wilber_32  $(srcdir)/wilber-32.png	\
	wilber_48  $(srcdir)/wilber-48.png	\
	wilber_64  $(srcdir)/wilber-64.png


EXTRA_DIST = \
	makefile.msc				\
	gimpwidgets.def				\
	$(libgimpwidgets_2_0_la_extra_sources)	\
	$(WILBER_IMAGES)


gimpwidgets-private.c: gimp-wilber-pixbufs.h


#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-wec xgen-wmh xgen-wmc gimp-wilber-pixbufs.h
CLEANFILES = $(gen_sources)

$(srcdir)/gimpwidgetsenums.c: $(srcdir)/gimpwidgetsenums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"gimpwidgetsenums.h\"\n#include \"libgimp/libgimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (! type)\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE \"-libgimp\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/gimpwidgetsenums.h > xgen-wec \
	&& cp xgen-wec $(@F) \
	&& rm -f xgen-wec

$(srcdir)/gimpwidgetsmarshal.h: $(srcdir)/gimpwidgetsmarshal.list
	$(GLIB_GENMARSHAL) --prefix=_gimp_widgets_marshal $(srcdir)/gimpwidgetsmarshal.list --header >> xgen-wmh \
	&& (cmp -s xgen-wmh $(@F) || cp xgen-wmh $(@F)) \
	&& rm -f xgen-wmh xgen-wmh~

$(srcdir)/gimpwidgetsmarshal.c: $(srcdir)/gimpwidgetsmarshal.h
	echo "#include \"gimpwidgetsmarshal.h\"" >> xgen-wmc \
	&& $(GLIB_GENMARSHAL) --prefix=_gimp_widgets_marshal $(srcdir)/gimpwidgetsmarshal.list --body >> xgen-wmc \
	&& cp xgen-wmc $(@F) \
	&& rm -f xgen-wmc xgen-wmc~


$(srcdir)/gimp-wilber-pixbufs.h: $(WILBER_IMAGES) Makefile.am
	$(GDK_PIXBUF_CSOURCE) --raw --build-list $(WILBER_VARIABLES) > $(@F)


#
# test programs, not installed
#

noinst_PROGRAMS = test-preview-area

test_preview_area_SOURCES = test-preview-area.c 

test_preview_area_DEPENDENCIES = \
	$(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la

test_preview_area_LDADD = \
	$(GTK_LIBS) \
	$(test_preview_area_DEPENDENCIES)



install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
