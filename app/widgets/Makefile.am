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
	gimpaction.c			\
	gimpaction.h			\
	gimpactionfactory.c		\
	gimpactionfactory.h		\
	gimpactiongroup.c		\
	gimpactiongroup.h		\
	gimpbrusheditor.c		\
	gimpbrusheditor.h		\
	gimpbrushfactoryview.c		\
	gimpbrushfactoryview.h		\
	gimpbufferview.c		\
	gimpbufferview.h		\
	gimpcellrenderertoggle.c	\
	gimpcellrenderertoggle.h	\
	gimpcellrendererviewable.c	\
	gimpcellrendererviewable.h	\
	gimpchanneltreeview.c		\
	gimpchanneltreeview.h		\
	gimpcolorbar.c			\
	gimpcolorbar.h			\
	gimpcolordisplayeditor.c	\
	gimpcolordisplayeditor.h	\
	gimpcoloreditor.c		\
	gimpcoloreditor.h		\
	gimpcolorframe.c		\
	gimpcolorframe.h		\
	gimpcolormapeditor.c		\
	gimpcolormapeditor.h		\
	gimpcolorpanel.c		\
	gimpcolorpanel.h		\
	gimpcomponenteditor.c		\
	gimpcomponenteditor.h		\
	gimpcontainerbox.c		\
	gimpcontainerbox.h		\
	gimpcontainercombobox.c		\
	gimpcontainercombobox.h		\
	gimpcontainereditor.c		\
	gimpcontainereditor.h		\
	gimpcontainergridview.c		\
	gimpcontainergridview.h		\
	gimpcontainerpopup.c		\
	gimpcontainerpopup.h		\
	gimpcontainertreeview.c		\
	gimpcontainertreeview.h		\
	gimpcontainertreeview-dnd.c	\
	gimpcontainertreeview-dnd.h	\
	gimpcontainerview.c		\
	gimpcontainerview.h		\
	gimpcontainerview-utils.c	\
	gimpcontainerview-utils.h	\
	gimpcursor.c			\
	gimpcursor.h			\
	gimpdasheditor.c		\
	gimpdasheditor.h		\
	gimpdataeditor.c		\
	gimpdataeditor.h		\
	gimpdatafactoryview.c		\
	gimpdatafactoryview.h		\
	gimpdeviceinfo.c		\
	gimpdeviceinfo.h		\
	gimpdevices.c			\
	gimpdevices.h			\
	gimpdevicestatus.c		\
	gimpdevicestatus.h		\
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
	gimpdocked.c			\
	gimpdocked.h			\
	gimpdocumentview.c		\
	gimpdocumentview.h		\
	gimpdrawabletreeview.c		\
	gimpdrawabletreeview.h		\
	gimpeditor.c			\
	gimpeditor.h			\
	gimpenumaction.c		\
	gimpenumaction.h		\
	gimpenumcombobox.c		\
	gimpenumcombobox.h		\
	gimpenumstore.c			\
	gimpenumstore.h			\
	gimpenumwidgets.c		\
	gimpenumwidgets.h		\
	gimperrorconsole.c		\
	gimperrorconsole.h		\
	gimpfgbgeditor.c		\
	gimpfgbgeditor.h		\
	gimpfiledialog.c		\
	gimpfiledialog.h		\
	gimpfontview.c			\
	gimpfontview.h			\
	gimpgradienteditor.c		\
	gimpgradienteditor.h		\
	gimpgrideditor.c		\
	gimpgrideditor.h		\
	gimphelp.c			\
	gimphelp.h			\
	gimphelp-ids.h			\
	gimphistogrambox.c		\
	gimphistogrambox.h		\
	gimphistogrameditor.c		\
	gimphistogrameditor.h		\
	gimphistogramview.c		\
	gimphistogramview.h		\
	gimpimagedock.c			\
	gimpimagedock.h			\
	gimpimageeditor.c		\
	gimpimageeditor.h		\
	gimpimageview.c			\
	gimpimageview.h			\
	gimpitemfactory.c		\
	gimpitemfactory.h		\
	gimpitemtreeview.c		\
	gimpitemtreeview.h		\
	gimplayertreeview.c		\
	gimplayertreeview.h		\
	gimpmenufactory.c		\
	gimpmenufactory.h		\
	gimpnavigationpreview.c		\
	gimpnavigationpreview.h		\
	gimppaletteeditor.c		\
	gimppaletteeditor.h		\
	gimppatternfactoryview.c	\
	gimppatternfactoryview.h	\
	gimppluginaction.c		\
	gimppluginaction.h		\
	gimppreview.c			\
	gimppreview.h			\
	gimppreview-popup.c		\
	gimppreview-popup.h		\
	gimppreviewrenderer.c		\
	gimppreviewrenderer.h		\
	gimppreviewrenderer-utils.c	\
	gimppreviewrenderer-utils.h	\
	gimppreviewrendererbrush.c	\
	gimppreviewrendererbrush.h	\
	gimppreviewrendererdrawable.c	\
	gimppreviewrendererdrawable.h	\
	gimppreviewrenderergradient.c	\
	gimppreviewrenderergradient.h	\
	gimppreviewrendererimage.c	\
	gimppreviewrendererimage.h	\
	gimppreviewrendererimagefile.c	\
	gimppreviewrendererimagefile.h	\
	gimppreviewrendererlayer.c	\
	gimppreviewrendererlayer.h	\
	gimppreviewrenderervectors.c	\
	gimppreviewrenderervectors.h	\
	gimppropwidgets.c		\
	gimppropwidgets.h		\
	gimpselectioneditor.c		\
	gimpselectioneditor.h		\
	gimpsessioninfo.c		\
	gimpsessioninfo.h		\
	gimpstringaction.c		\
	gimpstringaction.h		\
	gimpstrokeeditor.c		\
	gimpstrokeeditor.h		\
	gimptemplateeditor.c		\
	gimptemplateeditor.h		\
	gimptemplateview.c		\
	gimptemplateview.h		\
	gimptexteditor.c		\
	gimptexteditor.h		\
	gimpthumbbox.c			\
	gimpthumbbox.h			\
	gimptoolbox.c			\
	gimptoolbox.h			\
	gimptoolbox-color-area.c	\
	gimptoolbox-color-area.h	\
	gimptoolbox-dnd.c		\
	gimptoolbox-dnd.h		\
	gimptoolbox-indicator-area.c	\
	gimptoolbox-indicator-area.h	\
	gimptooldialog.c		\
	gimptooldialog.h		\
	gimptooloptionseditor.c		\
	gimptooloptionseditor.h		\
	gimptoolview.c			\
	gimptoolview.h			\
	gimpuimanager.c			\
	gimpuimanager.h			\
	gimpundoeditor.c		\
	gimpundoeditor.h		\
	gimpunitcombobox.c		\
	gimpunitcombobox.h		\
	gimpunitstore.c			\
	gimpunitstore.h			\
	gimpvectorstreeview.c		\
	gimpvectorstreeview.h		\
	gimpviewablebutton.c		\
	gimpviewablebutton.h		\
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

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-wec
CLEANFILES = $(gen_sources)

$(srcdir)/widgets-enums.c: $(srcdir)/widgets-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gtk/gtk.h>\n#include \"widgets-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/widgets-enums.h > xgen-wec \
	&& cp xgen-wec $(@F) \
	&& rm -f xgen-wec
