## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

AM_CPPFLAGS = \
        -DISO_CODES_LOCATION=\"$(ISO_CODES_LOCATION)\"		\
        -DISO_CODES_LOCALEDIR=\"$(ISO_CODES_LOCALEDIR)\"	\
	-DG_LOG_DOMAIN=\"Gimp-Widgets\"				\
	-I$(top_builddir)					\
	-I$(top_srcdir)						\
	-I$(top_builddir)/app					\
	-I$(top_srcdir)/app					\
	$(GEGL_CFLAGS)						\
	$(GTK_CFLAGS)						\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

noinst_LIBRARIES = libappwidgets.a

libappwidgets_a_sources = \
	widgets-enums.h			\
	widgets-types.h			\
	gimpaction.c			\
	gimpaction.h			\
	gimpaction-history.c		\
	gimpaction-history.h		\
	gimpactioneditor.c		\
	gimpactioneditor.h		\
	gimpactionfactory.c		\
	gimpactionfactory.h		\
	gimpactiongroup.c		\
	gimpactiongroup.h		\
	gimpactionview.c		\
	gimpactionview.h		\
	gimpblobeditor.c		\
	gimpblobeditor.h		\
	gimpbrusheditor.c		\
	gimpbrusheditor.h		\
	gimpbrushfactoryview.c		\
	gimpbrushfactoryview.h		\
	gimpbrushselect.c		\
	gimpbrushselect.h		\
	gimpbuffersourcebox.c		\
	gimpbuffersourcebox.h		\
	gimpbufferview.c		\
	gimpbufferview.h		\
	gimpcairo-wilber.c		\
	gimpcairo-wilber.h		\
	gimpcellrendererbutton.c	\
	gimpcellrendererbutton.h	\
	gimpcellrendererdashes.c	\
	gimpcellrendererdashes.h	\
	gimpcellrendererviewable.c	\
	gimpcellrendererviewable.h	\
	gimpcircle.c			\
	gimpcircle.h			\
	gimpchanneltreeview.c		\
	gimpchanneltreeview.h		\
	gimpclipboard.c			\
	gimpclipboard.h			\
	gimpcolorbar.c			\
	gimpcolorbar.h			\
	gimpcolordialog.c		\
	gimpcolordialog.h		\
	gimpcolordisplayeditor.c	\
	gimpcolordisplayeditor.h	\
	gimpcoloreditor.c		\
	gimpcoloreditor.h		\
	gimpcolorframe.c		\
	gimpcolorframe.h		\
	gimpcolorhistory.c		\
	gimpcolorhistory.h		\
	gimpcolormapeditor.c		\
	gimpcolormapeditor.h		\
	gimpcolorpanel.c		\
	gimpcolorpanel.h		\
	gimpcolorselectorpalette.c	\
	gimpcolorselectorpalette.h	\
	gimpcombotagentry.c		\
	gimpcombotagentry.h		\
	gimpcomponenteditor.c		\
	gimpcomponenteditor.h		\
	gimpcontainerbox.c		\
	gimpcontainerbox.h		\
	gimpcontainercombobox.c		\
	gimpcontainercombobox.h		\
	gimpcontainereditor.c		\
	gimpcontainereditor.h		\
	gimpcontainerentry.c		\
	gimpcontainerentry.h		\
	gimpcontainericonview.c		\
	gimpcontainericonview.h		\
	gimpcontainerpopup.c		\
	gimpcontainerpopup.h		\
	gimpcontainertreestore.c	\
	gimpcontainertreestore.h	\
	gimpcontainertreeview.c		\
	gimpcontainertreeview.h		\
	gimpcontainertreeview-dnd.c	\
	gimpcontainertreeview-dnd.h	\
	gimpcontainertreeview-private.h	\
	gimpcontainerview.c		\
	gimpcontainerview.h		\
	gimpcontainerview-utils.c	\
	gimpcontainerview-utils.h	\
	gimpcontrollereditor.c		\
	gimpcontrollereditor.h		\
	gimpcontrollerinfo.c		\
	gimpcontrollerinfo.h		\
	gimpcontrollerlist.c		\
	gimpcontrollerlist.h		\
	gimpcontrollers.c		\
	gimpcontrollers.h		\
	gimpcontrollerkeyboard.c	\
	gimpcontrollerkeyboard.h	\
	gimpcontrollermouse.c		\
	gimpcontrollermouse.h		\
	gimpcontrollerwheel.c		\
	gimpcontrollerwheel.h		\
	gimpcriticaldialog.c		\
	gimpcriticaldialog.h		\
	gimpcursor.c			\
	gimpcursor.h			\
	gimpcurveview.c			\
	gimpcurveview.h			\
	gimpdashboard.c			\
	gimpdashboard.h			\
	gimpdasheditor.c		\
	gimpdasheditor.h		\
	gimpdataeditor.c		\
	gimpdataeditor.h		\
	gimpdatafactoryview.c		\
	gimpdatafactoryview.h		\
	gimpdeviceeditor.c		\
	gimpdeviceeditor.h		\
	gimpdeviceinfo.c		\
	gimpdeviceinfo.h		\
	gimpdeviceinfo-coords.c		\
	gimpdeviceinfo-coords.h		\
	gimpdeviceinfoeditor.c		\
	gimpdeviceinfoeditor.h		\
	gimpdevicemanager.c		\
	gimpdevicemanager.h		\
	gimpdevices.c			\
	gimpdevices.h			\
	gimpdevicestatus.c		\
	gimpdevicestatus.h		\
	gimpdial.c			\
	gimpdial.h			\
	gimpdialogfactory.c		\
	gimpdialogfactory.h		\
	gimpdnd.c			\
	gimpdnd.h			\
	gimpdnd-xds.c			\
	gimpdnd-xds.h			\
	gimpdock.c			\
	gimpdock.h			\
	gimpdockcolumns.c		\
	gimpdockcolumns.h		\
	gimpdockable.c			\
	gimpdockable.h			\
	gimpdockbook.c			\
	gimpdockbook.h			\
	gimpdockcontainer.c		\
	gimpdockcontainer.h		\
	gimpdocked.c			\
	gimpdocked.h			\
	gimpdockwindow.c		\
	gimpdockwindow.h		\
	gimpdocumentview.c		\
	gimpdocumentview.h		\
	gimpdrawabletreeview.c		\
	gimpdrawabletreeview.h		\
	gimpdynamicseditor.c		\
	gimpdynamicseditor.h		\
	gimpdynamicsfactoryview.c	\
	gimpdynamicsfactoryview.h	\
	gimpdynamicsoutputeditor.c	\
	gimpdynamicsoutputeditor.h	\
	gimpeditor.c			\
	gimpeditor.h			\
	gimpenumaction.c		\
	gimpenumaction.h		\
	gimperrorconsole.c		\
	gimperrorconsole.h		\
	gimperrordialog.c		\
	gimperrordialog.h		\
	gimpexportdialog.c		\
	gimpexportdialog.h		\
	gimpfgbgeditor.c		\
	gimpfgbgeditor.h		\
	gimpfgbgview.c			\
	gimpfgbgview.h			\
	gimpfiledialog.c		\
	gimpfiledialog.h		\
	gimpfileprocview.c		\
	gimpfileprocview.h		\
	gimpfilleditor.c		\
	gimpfilleditor.h		\
	gimpfontselect.c		\
	gimpfontselect.h		\
	gimpfontview.c			\
	gimpfontview.h			\
	gimpgradienteditor.c		\
	gimpgradienteditor.h		\
	gimpgradientselect.c		\
	gimpgradientselect.h		\
	gimpgrideditor.c		\
	gimpgrideditor.h		\
	gimphandlebar.c			\
	gimphandlebar.h			\
	gimphelp.c			\
	gimphelp.h			\
	gimphelp-ids.h			\
	gimphistogrambox.c		\
	gimphistogrambox.h		\
	gimphistogrameditor.c		\
	gimphistogrameditor.h		\
	gimphistogramview.c		\
	gimphistogramview.h		\
	gimpiconpicker.c		\
	gimpiconpicker.h		\
	gimpiconsizescale.c		\
	gimpiconsizescale.h		\
	gimpimagecommenteditor.c	\
	gimpimagecommenteditor.h	\
	gimpimageeditor.c		\
	gimpimageeditor.h		\
	gimpimageparasiteview.c		\
	gimpimageparasiteview.h		\
	gimpimageprofileview.c		\
	gimpimageprofileview.h		\
	gimpimagepropview.c		\
	gimpimagepropview.h		\
	gimpimageview.c			\
	gimpimageview.h			\
	gimpitemtreeview.c		\
	gimpitemtreeview.h		\
	gimplanguagecombobox.c		\
	gimplanguagecombobox.h		\
	gimplanguageentry.c		\
	gimplanguageentry.h		\
	gimplanguagestore.c		\
	gimplanguagestore.h		\
	gimplanguagestore-parser.c	\
	gimplanguagestore-parser.h	\
	gimplayermodebox.c		\
	gimplayermodebox.h		\
	gimplayermodecombobox.c		\
	gimplayermodecombobox.h		\
	gimplayertreeview.c		\
	gimplayertreeview.h		\
	gimpmenudock.c			\
	gimpmenudock.h			\
	gimpmenufactory.c		\
	gimpmenufactory.h		\
	gimpmessagebox.c		\
	gimpmessagebox.h		\
	gimpmessagedialog.c		\
	gimpmessagedialog.h		\
	gimpmeter.c			\
	gimpmeter.h			\
	gimpnavigationview.c		\
	gimpnavigationview.h		\
	gimpopendialog.c		\
	gimpopendialog.h		\
	gimpoverlaybox.c		\
	gimpoverlaybox.h		\
	gimpoverlaychild.c		\
	gimpoverlaychild.h		\
	gimpoverlaydialog.c		\
	gimpoverlaydialog.h		\
	gimpoverlayframe.c		\
	gimpoverlayframe.h		\
	gimppaletteeditor.c		\
	gimppaletteeditor.h		\
	gimppaletteselect.c		\
	gimppaletteselect.h		\
	gimppaletteview.c		\
	gimppaletteview.h		\
	gimppanedbox.c			\
	gimppanedbox.h			\
	gimppatternfactoryview.c	\
	gimppatternfactoryview.h	\
	gimppatternselect.c		\
	gimppatternselect.h		\
	gimppdbdialog.c			\
	gimppdbdialog.h			\
	gimppickablebutton.c		\
	gimppickablebutton.h		\
	gimppickablepopup.c		\
	gimppickablepopup.h		\
	gimppixbuf.c			\
	gimppixbuf.h			\
	gimppluginview.c		\
	gimppluginview.h		\
	gimppolar.c			\
	gimppolar.h			\
	gimppopup.c			\
	gimppopup.h			\
	gimpprefsbox.c			\
	gimpprefsbox.h			\
	gimpprocedureaction.c		\
	gimpprocedureaction.h		\
	gimpprogressbox.c		\
	gimpprogressbox.h		\
	gimpprogressdialog.c		\
	gimpprogressdialog.h		\
	gimppropwidgets.c		\
	gimppropwidgets.h		\
	gimpradioaction.c		\
	gimpradioaction.h		\
	gimprender.c			\
	gimprender.h			\
	gimpsamplepointeditor.c		\
	gimpsamplepointeditor.h		\
	gimpsavedialog.c		\
	gimpsavedialog.h		\
	gimpsearchpopup.c		\
	gimpsearchpopup.h		\
	gimpselectiondata.c		\
	gimpselectiondata.h		\
	gimpselectioneditor.c		\
	gimpselectioneditor.h		\
	gimpsessioninfo.c		\
	gimpsessioninfo.h		\
	gimpsessioninfo-aux.c		\
	gimpsessioninfo-aux.h		\
	gimpsessioninfo-book.c		\
	gimpsessioninfo-book.h		\
	gimpsessioninfo-dock.c		\
	gimpsessioninfo-dock.h		\
	gimpsessioninfo-dockable.c	\
	gimpsessioninfo-dockable.h	\
	gimpsessioninfo-private.h	\
	gimpsessionmanaged.c		\
	gimpsessionmanaged.h		\
	gimpsettingsbox.c		\
	gimpsettingsbox.h		\
	gimpsettingseditor.c		\
	gimpsettingseditor.h		\
	gimpsizebox.c			\
	gimpsizebox.h			\
	gimpspinscale.c			\
	gimpspinscale.h			\
	gimpstringaction.c		\
	gimpstringaction.h		\
	gimpstrokeeditor.c		\
	gimpstrokeeditor.h		\
	gimpsymmetryeditor.c		\
	gimpsymmetryeditor.h		\
	gimptagentry.c			\
	gimptagentry.h			\
	gimptagpopup.c			\
	gimptagpopup.h			\
	gimptemplateeditor.c		\
	gimptemplateeditor.h		\
	gimptemplateview.c		\
	gimptemplateview.h		\
	gimptextbuffer.c		\
	gimptextbuffer.h		\
	gimptextbuffer-serialize.c	\
	gimptextbuffer-serialize.h	\
	gimptexteditor.c		\
	gimptexteditor.h		\
	gimptextproxy.c			\
	gimptextproxy.h			\
	gimptextstyleeditor.c		\
	gimptextstyleeditor.h		\
	gimptexttag.c			\
	gimptexttag.h			\
	gimpthumbbox.c			\
	gimpthumbbox.h			\
	gimptoggleaction.c		\
	gimptoggleaction.h		\
	gimptoolbox.c			\
	gimptoolbox.h			\
	gimptoolbox-color-area.c	\
	gimptoolbox-color-area.h	\
	gimptoolbox-dnd.c		\
	gimptoolbox-dnd.h		\
	gimptoolbox-image-area.c	\
	gimptoolbox-image-area.h	\
	gimptoolbox-indicator-area.c	\
	gimptoolbox-indicator-area.h	\
	gimptooleditor.c		\
	gimptooleditor.h		\
	gimptooloptionseditor.c		\
	gimptooloptionseditor.h		\
	gimptoolpalette.c		\
	gimptoolpalette.h		\
	gimptoolpreseteditor.c		\
	gimptoolpreseteditor.h		\
	gimptoolpresetfactoryview.c	\
	gimptoolpresetfactoryview.h	\
	gimptranslationstore.c		\
	gimptranslationstore.h		\
	gimpuimanager.c			\
	gimpuimanager.h			\
	gimpundoeditor.c		\
	gimpundoeditor.h		\
	gimpvectorstreeview.c		\
	gimpvectorstreeview.h		\
	gimpview.c			\
	gimpview.h			\
	gimpview-popup.c		\
	gimpview-popup.h		\
	gimpviewablebox.c		\
	gimpviewablebox.h		\
	gimpviewablebutton.c		\
	gimpviewablebutton.h		\
	gimpviewabledialog.c		\
	gimpviewabledialog.h		\
	gimpviewrenderer.c		\
	gimpviewrenderer.h		\
	gimpviewrenderer-frame.c	\
	gimpviewrenderer-frame.h	\
	gimpviewrenderer-utils.c	\
	gimpviewrenderer-utils.h	\
	gimpviewrendererbrush.c		\
	gimpviewrendererbrush.h		\
	gimpviewrendererbuffer.c	\
	gimpviewrendererbuffer.h	\
	gimpviewrendererdrawable.c	\
	gimpviewrendererdrawable.h	\
	gimpviewrenderergradient.c	\
	gimpviewrenderergradient.h	\
	gimpviewrendererimage.c 	\
	gimpviewrendererimage.h 	\
	gimpviewrendererimagefile.c	\
	gimpviewrendererimagefile.h	\
	gimpviewrendererlayer.c		\
	gimpviewrendererlayer.h		\
	gimpviewrendererpalette.c	\
	gimpviewrendererpalette.h	\
	gimpviewrenderervectors.c	\
	gimpviewrenderervectors.h	\
	gimpwidgets-constructors.c	\
	gimpwidgets-constructors.h	\
	gimpwidgets-utils.c		\
	gimpwidgets-utils.h		\
	gimpwindow.c			\
	gimpwindow.h			\
	gimpwindowstrategy.c		\
	gimpwindowstrategy.h

libappwidgets_a_built_sources = widgets-enums.c

libappwidgets_a_SOURCES = \
	$(libappwidgets_a_built_sources) $(libappwidgets_a_sources)


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-wec
CLEANFILES = $(gen_sources)

xgen-wec: $(srcdir)/widgets-enums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gtk/gtk.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"widgets-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/widgets-enums.c: xgen-wec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
