## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Widgets\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	$(PANGOFT2_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappwidgets.a

libappwidgets_a_sources = \
	widgets-enums.h			\
	widgets-types.h			\
	gimpbrusheditor.c		\
	gimpbrusheditor.h		\
	gimpbrushfactoryview.c		\
	gimpbrushfactoryview.h		\
	gimpbrushpreview.c		\
	gimpbrushpreview.h		\
	gimpbufferpreview.c		\
	gimpbufferpreview.h		\
	gimpbufferview.c		\
	gimpbufferview.h		\
	gimpchannellistitem.c		\
	gimpchannellistitem.h		\
	gimpchannellistview.c		\
	gimpchannellistview.h		\
	gimpcoloreditor.c		\
	gimpcoloreditor.h		\
	gimpcolormapeditor.c		\
	gimpcolormapeditor.h		\
	gimpcolorpanel.c		\
	gimpcolorpanel.h		\
	gimpcomponentlistitem.c		\
	gimpcomponentlistitem.h		\
	gimpcontainereditor.c		\
	gimpcontainereditor.h		\
	gimpcontainergridview.c		\
	gimpcontainergridview.h		\
	gimpcontainerlistview.c		\
	gimpcontainerlistview.h		\
	gimpcontainermenu.c		\
	gimpcontainermenu.h		\
	gimpcontainermenuimpl.c		\
	gimpcontainermenuimpl.h		\
	gimpcontainerview.c		\
	gimpcontainerview.h		\
	gimpcontainerview-utils.c	\
	gimpcontainerview-utils.h	\
	gimpcursor.c			\
	gimpcursor.h			\
	gimpdataeditor.c		\
	gimpdataeditor.h		\
	gimpdatafactoryview.c		\
	gimpdatafactoryview.h		\
	gimpdeviceinfo.c		\
	gimpdeviceinfo.h		\
	gimpdevices.c			\
	gimpdevices.h			\
	gimpdialogfactory.c		\
	gimpdialogfactory.h		\
	gimpdnd.c			\
	gimpdnd.h			\
	gimpdock.c			\
	gimpdock.h			\
	gimpdockable.c			\
	gimpdockable.h			\
	gimpdockbook.c			\
	gimpdockbook.h			\
	gimpdocumentview.c		\
	gimpdocumentview.h		\
	gimpdrawablelistitem.c		\
	gimpdrawablelistitem.h		\
	gimpdrawablelistview.c		\
	gimpdrawablelistview.h		\
	gimpdrawablepreview.c		\
	gimpdrawablepreview.h		\
	gimpeditor.c			\
	gimpeditor.h			\
	gimpenummenu.c			\
	gimpenummenu.h			\
	gimpfontselection.c		\
	gimpfontselection.h		\
	gimpfontselection-dialog.c	\
	gimpfontselection-dialog.h	\
	gimpgradienteditor.c		\
	gimpgradienteditor.h		\
	gimpgradientpreview.c		\
	gimpgradientpreview.h		\
	gimphelp.c			\
	gimphelp.h			\
	gimphistogrambox.c		\
	gimphistogrambox.h		\
	gimphistogramview.c		\
	gimphistogramview.h		\
	gimpimagedock.c			\
	gimpimagedock.h			\
	gimpimagefilepreview.c		\
	gimpimagefilepreview.h		\
	gimpimagepreview.c		\
	gimpimagepreview.h		\
	gimpimageview.c			\
	gimpimageview.h			\
	gimpitemfactory.c		\
	gimpitemfactory.h		\
	gimpitemlistitem.c		\
	gimpitemlistitem.h		\
	gimpitemlistview.c		\
	gimpitemlistview.h		\
	gimplayerlistitem.c		\
	gimplayerlistitem.h		\
	gimplayerlistview.c		\
	gimplayerlistview.h		\
	gimplistitem.c			\
	gimplistitem.h			\
	gimpmenufactory.c		\
	gimpmenufactory.h		\
	gimpmenuitem.c			\
	gimpmenuitem.h			\
	gimpnavigationpreview.c		\
	gimpnavigationpreview.h		\
	gimppaletteeditor.c		\
	gimppaletteeditor.h		\
	gimppalettepreview.c		\
	gimppalettepreview.h		\
	gimppatternpreview.c		\
	gimppatternpreview.h		\
	gimppreview.c			\
	gimppreview.h			\
	gimppreview-utils.c		\
	gimppreview-utils.h		\
	gimppropwidgets.c		\
	gimppropwidgets.h		\
	gimpselectioneditor.c		\
	gimpselectioneditor.h		\
	gimptoolbox.c			\
	gimptoolbox.h			\
	gimptoolbox-color-area.c	\
	gimptoolbox-color-area.h	\
	gimptoolbox-indicator-area.c	\
	gimptoolbox-indicator-area.h	\
	gimptoolinfopreview.c		\
	gimptoolinfopreview.h		\
	gimpvectorslistview.c		\
	gimpvectorslistview.h		\
	gimpviewabledialog.c		\
	gimpviewabledialog.h		\
	gimpwidgets-constructors.c	\
	gimpwidgets-constructors.h	\
	gimpwidgets-utils.c		\
	gimpwidgets-utils.h		\
	gtkwrapbox.c			\
	gtkwrapbox.h			\
	gtkhwrapbox.c			\
	gtkhwrapbox.h			\
	gtkvwrapbox.c			\
	gtkvwrapbox.h

libappwidgets_a_built_sources = widgets-enums.c

libappwidgets_a_SOURCES = \
	$(libappwidgets_a_built_sources) $(libappwidgets_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-wec
CLEANFILES = $(gen_sources)

$(srcdir)/widgets-enums.c: $(srcdir)/widgets-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gtk/gtk.h>\n#include \"widgets-enums.h\"\n#include \"libgimp/gimpintl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/widgets-enums.h > xgen-wec \
	&& cp xgen-wec $(@F) \
	&& rm -f xgen-wec
