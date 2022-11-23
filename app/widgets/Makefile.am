## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

AM_CPPFLAGS = \
        -DISO_CODES_LOCATION=\"$(ISO_CODES_LOCATION)\"		\
        -DISO_CODES_LOCALEDIR=\"$(ISO_CODES_LOCALEDIR)\"	\
	-DG_LOG_DOMAIN=\"Ligma-Widgets\"				\
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
	ligmaaccellabel.c		\
	ligmaaccellabel.h		\
	ligmaaction.c			\
	ligmaaction.h			\
	ligmaaction-history.c		\
	ligmaaction-history.h		\
	ligmaactioneditor.c		\
	ligmaactioneditor.h		\
	ligmaactionfactory.c		\
	ligmaactionfactory.h		\
	ligmaactiongroup.c		\
	ligmaactiongroup.h		\
	ligmaactionimpl.c		\
	ligmaactionimpl.h		\
	ligmaactionview.c		\
	ligmaactionview.h		\
	ligmablobeditor.c		\
	ligmablobeditor.h		\
	ligmabrusheditor.c		\
	ligmabrusheditor.h		\
	ligmabrushfactoryview.c		\
	ligmabrushfactoryview.h		\
	ligmabrushselect.c		\
	ligmabrushselect.h		\
	ligmabuffersourcebox.c		\
	ligmabuffersourcebox.h		\
	ligmabufferview.c		\
	ligmabufferview.h		\
	ligmacairo-wilber.c		\
	ligmacairo-wilber.h		\
	ligmacellrendererbutton.c	\
	ligmacellrendererbutton.h	\
	ligmacellrendererdashes.c	\
	ligmacellrendererdashes.h	\
	ligmacellrendererviewable.c	\
	ligmacellrendererviewable.h	\
	ligmacircle.c			\
	ligmacircle.h			\
	ligmachanneltreeview.c		\
	ligmachanneltreeview.h		\
	ligmaclipboard.c			\
	ligmaclipboard.h			\
	ligmacolorbar.c			\
	ligmacolorbar.h			\
	ligmacolordialog.c		\
	ligmacolordialog.h		\
	ligmacolordisplayeditor.c	\
	ligmacolordisplayeditor.h	\
	ligmacoloreditor.c		\
	ligmacoloreditor.h		\
	ligmacolorframe.c		\
	ligmacolorframe.h		\
	ligmacolorhistory.c		\
	ligmacolorhistory.h		\
	ligmacolormapeditor.c		\
	ligmacolormapeditor.h		\
	ligmacolormapselection.c		\
	ligmacolormapselection.h		\
	ligmacolorpanel.c		\
	ligmacolorpanel.h		\
	ligmacolorselectorpalette.c	\
	ligmacolorselectorpalette.h	\
	ligmacombotagentry.c		\
	ligmacombotagentry.h		\
	ligmacomponenteditor.c		\
	ligmacomponenteditor.h		\
	ligmacompressioncombobox.c	\
	ligmacompressioncombobox.h	\
	ligmacontainerbox.c		\
	ligmacontainerbox.h		\
	ligmacontainercombobox.c		\
	ligmacontainercombobox.h		\
	ligmacontainereditor.c		\
	ligmacontainereditor.h		\
	ligmacontainerentry.c		\
	ligmacontainerentry.h		\
	ligmacontainericonview.c		\
	ligmacontainericonview.h		\
	ligmacontainerpopup.c		\
	ligmacontainerpopup.h		\
	ligmacontainertreestore.c	\
	ligmacontainertreestore.h	\
	ligmacontainertreeview.c		\
	ligmacontainertreeview.h		\
	ligmacontainertreeview-dnd.c	\
	ligmacontainertreeview-dnd.h	\
	ligmacontainertreeview-private.h	\
	ligmacontainerview.c		\
	ligmacontainerview.h		\
	ligmacontainerview-utils.c	\
	ligmacontainerview-utils.h	\
	ligmacontrollereditor.c		\
	ligmacontrollereditor.h		\
	ligmacontrollerinfo.c		\
	ligmacontrollerinfo.h		\
	ligmacontrollerlist.c		\
	ligmacontrollerlist.h		\
	ligmacontrollers.c		\
	ligmacontrollers.h		\
	ligmacontrollerkeyboard.c	\
	ligmacontrollerkeyboard.h	\
	ligmacontrollerwheel.c		\
	ligmacontrollerwheel.h		\
	ligmacriticaldialog.c		\
	ligmacriticaldialog.h		\
	ligmacursor.c			\
	ligmacursor.h			\
	ligmacurveview.c			\
	ligmacurveview.h			\
	ligmadashboard.c			\
	ligmadashboard.h			\
	ligmadasheditor.c		\
	ligmadasheditor.h		\
	ligmadataeditor.c		\
	ligmadataeditor.h		\
	ligmadatafactoryview.c		\
	ligmadatafactoryview.h		\
	ligmadeviceeditor.c		\
	ligmadeviceeditor.h		\
	ligmadeviceinfo.c		\
	ligmadeviceinfo.h		\
	ligmadeviceinfo-coords.c		\
	ligmadeviceinfo-coords.h		\
	ligmadeviceinfoeditor.c		\
	ligmadeviceinfoeditor.h		\
	ligmadevicemanager.c		\
	ligmadevicemanager.h		\
	ligmadevices.c			\
	ligmadevices.h			\
	ligmadevicestatus.c		\
	ligmadevicestatus.h		\
	ligmadial.c			\
	ligmadial.h			\
	ligmadialogfactory.c		\
	ligmadialogfactory.h		\
	ligmadnd.c			\
	ligmadnd.h			\
	ligmadnd-xds.c			\
	ligmadnd-xds.h			\
	ligmadock.c			\
	ligmadock.h			\
	ligmadockcolumns.c		\
	ligmadockcolumns.h		\
	ligmadockable.c			\
	ligmadockable.h			\
	ligmadockbook.c			\
	ligmadockbook.h			\
	ligmadockcontainer.c		\
	ligmadockcontainer.h		\
	ligmadocked.c			\
	ligmadocked.h			\
	ligmadockwindow.c		\
	ligmadockwindow.h		\
	ligmadocumentview.c		\
	ligmadocumentview.h		\
	ligmadoubleaction.c		\
	ligmadoubleaction.h		\
	ligmadrawabletreeview.c		\
	ligmadrawabletreeview.h		\
	ligmadynamicseditor.c		\
	ligmadynamicseditor.h		\
	ligmadynamicsfactoryview.c	\
	ligmadynamicsfactoryview.h	\
	ligmadynamicsoutputeditor.c	\
	ligmadynamicsoutputeditor.h	\
	ligmaeditor.c			\
	ligmaeditor.h			\
	ligmaenumaction.c		\
	ligmaenumaction.h		\
	ligmaerrorconsole.c		\
	ligmaerrorconsole.h		\
	ligmaerrordialog.c		\
	ligmaerrordialog.h		\
	ligmaexportdialog.c		\
	ligmaexportdialog.h		\
	ligmaextensiondetails.c		\
	ligmaextensiondetails.h		\
	ligmaextensionlist.c		\
	ligmaextensionlist.h		\
	ligmafgbgeditor.c		\
	ligmafgbgeditor.h		\
	ligmafgbgview.c			\
	ligmafgbgview.h			\
	ligmafiledialog.c		\
	ligmafiledialog.h		\
	ligmafileprocview.c		\
	ligmafileprocview.h		\
	ligmafilleditor.c		\
	ligmafilleditor.h		\
	ligmafontfactoryview.c		\
	ligmafontfactoryview.h		\
	ligmafontselect.c		\
	ligmafontselect.h		\
	ligmagradienteditor.c		\
	ligmagradienteditor.h		\
	ligmagradientselect.c		\
	ligmagradientselect.h		\
	ligmagrideditor.c		\
	ligmagrideditor.h		\
	ligmahandlebar.c			\
	ligmahandlebar.h			\
	ligmahelp.c			\
	ligmahelp.h			\
	ligmahelp-ids.h			\
	ligmahistogrambox.c		\
	ligmahistogrambox.h		\
	ligmahistogrameditor.c		\
	ligmahistogrameditor.h		\
	ligmahistogramview.c		\
	ligmahistogramview.h		\
	ligmaiconpicker.c		\
	ligmaiconpicker.h		\
	ligmaimagecommenteditor.c	\
	ligmaimagecommenteditor.h	\
	ligmaimageeditor.c		\
	ligmaimageeditor.h		\
	ligmaimageparasiteview.c		\
	ligmaimageparasiteview.h		\
	ligmaimageprofileview.c		\
	ligmaimageprofileview.h		\
	ligmaimagepropview.c		\
	ligmaimagepropview.h		\
	ligmaimageview.c			\
	ligmaimageview.h			\
	ligmaitemtreeview.c		\
	ligmaitemtreeview.h		\
	ligmalanguagecombobox.c		\
	ligmalanguagecombobox.h		\
	ligmalanguageentry.c		\
	ligmalanguageentry.h		\
	ligmalanguagestore.c		\
	ligmalanguagestore.h		\
	ligmalanguagestore-parser.c	\
	ligmalanguagestore-parser.h	\
	ligmalayermodebox.c		\
	ligmalayermodebox.h		\
	ligmalayermodecombobox.c		\
	ligmalayermodecombobox.h		\
	ligmalayertreeview.c		\
	ligmalayertreeview.h		\
	ligmamenudock.c			\
	ligmamenudock.h			\
	ligmamenufactory.c		\
	ligmamenufactory.h		\
	ligmamessagebox.c		\
	ligmamessagebox.h		\
	ligmamessagedialog.c		\
	ligmamessagedialog.h		\
	ligmameter.c			\
	ligmameter.h			\
	ligmamodifierseditor.c		\
	ligmamodifierseditor.h		\
	ligmanavigationview.c		\
	ligmanavigationview.h		\
	ligmaopendialog.c		\
	ligmaopendialog.h		\
	ligmaoverlaybox.c		\
	ligmaoverlaybox.h		\
	ligmaoverlaychild.c		\
	ligmaoverlaychild.h		\
	ligmaoverlaydialog.c		\
	ligmaoverlaydialog.h		\
	ligmaoverlayframe.c		\
	ligmaoverlayframe.h		\
	ligmapaletteeditor.c		\
	ligmapaletteeditor.h		\
	ligmapaletteselect.c		\
	ligmapaletteselect.h		\
	ligmapaletteview.c		\
	ligmapaletteview.h		\
	ligmapanedbox.c			\
	ligmapanedbox.h			\
	ligmapatternfactoryview.c	\
	ligmapatternfactoryview.h	\
	ligmapatternselect.c		\
	ligmapatternselect.h		\
	ligmapdbdialog.c			\
	ligmapdbdialog.h			\
	ligmapickablebutton.c		\
	ligmapickablebutton.h		\
	ligmapickablepopup.c		\
	ligmapickablepopup.h		\
	ligmapivotselector.c		\
	ligmapivotselector.h		\
	ligmapixbuf.c			\
	ligmapixbuf.h			\
	ligmapluginview.c		\
	ligmapluginview.h		\
	ligmapolar.c			\
	ligmapolar.h			\
	ligmapopup.c			\
	ligmapopup.h			\
	ligmaprefsbox.c			\
	ligmaprefsbox.h			\
	ligmaprocedureaction.c		\
	ligmaprocedureaction.h		\
	ligmaprogressbox.c		\
	ligmaprogressbox.h		\
	ligmaprogressdialog.c		\
	ligmaprogressdialog.h		\
	ligmapropwidgets.c		\
	ligmapropwidgets.h		\
	ligmaradioaction.c		\
	ligmaradioaction.h		\
	ligmarender.c			\
	ligmarender.h			\
	ligmasamplepointeditor.c		\
	ligmasamplepointeditor.h		\
	ligmasavedialog.c		\
	ligmasavedialog.h		\
	ligmasearchpopup.c		\
	ligmasearchpopup.h		\
	ligmaselectiondata.c		\
	ligmaselectiondata.h		\
	ligmaselectioneditor.c		\
	ligmaselectioneditor.h		\
	ligmasessioninfo.c		\
	ligmasessioninfo.h		\
	ligmasessioninfo-aux.c		\
	ligmasessioninfo-aux.h		\
	ligmasessioninfo-book.c		\
	ligmasessioninfo-book.h		\
	ligmasessioninfo-dock.c		\
	ligmasessioninfo-dock.h		\
	ligmasessioninfo-dockable.c	\
	ligmasessioninfo-dockable.h	\
	ligmasessioninfo-private.h	\
	ligmasessionmanaged.c		\
	ligmasessionmanaged.h		\
	ligmasettingsbox.c		\
	ligmasettingsbox.h		\
	ligmasettingseditor.c		\
	ligmasettingseditor.h		\
	ligmashortcutbutton.c		\
	ligmashortcutbutton.h		\
	ligmashortcutbutton.c		\
	ligmashortcutbutton.h		\
	ligmasizebox.c			\
	ligmasizebox.h			\
	ligmastringaction.c		\
	ligmastringaction.h		\
	ligmastrokeeditor.c		\
	ligmastrokeeditor.h		\
	ligmasymmetryeditor.c		\
	ligmasymmetryeditor.h		\
	ligmatagentry.c			\
	ligmatagentry.h			\
	ligmatagpopup.c			\
	ligmatagpopup.h			\
	ligmatemplateeditor.c		\
	ligmatemplateeditor.h		\
	ligmatemplateview.c		\
	ligmatemplateview.h		\
	ligmatextbuffer.c		\
	ligmatextbuffer.h		\
	ligmatextbuffer-serialize.c	\
	ligmatextbuffer-serialize.h	\
	ligmatexteditor.c		\
	ligmatexteditor.h		\
	ligmatextproxy.c			\
	ligmatextproxy.h			\
	ligmatextstyleeditor.c		\
	ligmatextstyleeditor.h		\
	ligmatexttag.c			\
	ligmatexttag.h			\
	ligmathumbbox.c			\
	ligmathumbbox.h			\
	ligmatoggleaction.c		\
	ligmatoggleaction.h		\
	ligmatoolbox.c			\
	ligmatoolbox.h			\
	ligmatoolbox-color-area.c	\
	ligmatoolbox-color-area.h	\
	ligmatoolbox-dnd.c		\
	ligmatoolbox-dnd.h		\
	ligmatoolbox-image-area.c	\
	ligmatoolbox-image-area.h	\
	ligmatoolbox-indicator-area.c	\
	ligmatoolbox-indicator-area.h	\
        ligmatoolbutton.c                \
        ligmatoolbutton.h                \
	ligmatooleditor.c		\
	ligmatooleditor.h		\
	ligmatooloptionseditor.c		\
	ligmatooloptionseditor.h		\
	ligmatoolpalette.c		\
	ligmatoolpalette.h		\
	ligmatoolpreseteditor.c		\
	ligmatoolpreseteditor.h		\
	ligmatoolpresetfactoryview.c	\
	ligmatoolpresetfactoryview.h	\
	ligmatranslationstore.c		\
	ligmatranslationstore.h		\
	ligmauimanager.c			\
	ligmauimanager.h			\
	ligmaundoeditor.c		\
	ligmaundoeditor.h		\
	ligmavectorstreeview.c		\
	ligmavectorstreeview.h		\
	ligmaview.c			\
	ligmaview.h			\
	ligmaview-popup.c		\
	ligmaview-popup.h		\
	ligmaviewablebox.c		\
	ligmaviewablebox.h		\
	ligmaviewablebutton.c		\
	ligmaviewablebutton.h		\
	ligmaviewabledialog.c		\
	ligmaviewabledialog.h		\
	ligmaviewrenderer.c		\
	ligmaviewrenderer.h		\
	ligmaviewrenderer-frame.c	\
	ligmaviewrenderer-frame.h	\
	ligmaviewrenderer-utils.c	\
	ligmaviewrenderer-utils.h	\
	ligmaviewrendererbrush.c		\
	ligmaviewrendererbrush.h		\
	ligmaviewrendererbuffer.c	\
	ligmaviewrendererbuffer.h	\
	ligmaviewrendererdrawable.c	\
	ligmaviewrendererdrawable.h	\
	ligmaviewrenderergradient.c	\
	ligmaviewrenderergradient.h	\
	ligmaviewrendererimage.c 	\
	ligmaviewrendererimage.h 	\
	ligmaviewrendererimagefile.c	\
	ligmaviewrendererimagefile.h	\
	ligmaviewrendererlayer.c		\
	ligmaviewrendererlayer.h		\
	ligmaviewrendererpalette.c	\
	ligmaviewrendererpalette.h	\
	ligmaviewrenderervectors.c	\
	ligmaviewrenderervectors.h	\
	ligmawidgets-constructors.c	\
	ligmawidgets-constructors.h	\
	ligmawidgets-utils.c		\
	ligmawidgets-utils.h		\
	ligmawindow.c			\
	ligmawindow.h			\
	ligmawindowstrategy.c		\
	ligmawindowstrategy.h

libappwidgets_a_built_sources = \
	ligma-tool-cursors.c	\
	ligma-tool-cursors.h	\
	widgets-enums.c

libappwidgets_a_SOURCES = \
	$(libappwidgets_a_built_sources)	\
	$(libappwidgets_a_sources)


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = \
	ligma-tool-cursors.c	\
	ligma-tool-cursors.h	\
	stamp-widgets-enums.h	\
	xgen-wec
CLEANFILES = $(gen_sources)

xgen-wec: $(srcdir)/widgets-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-widgets-enums.h\"\n#include \"config.h\"\n#include <gtk/gtk.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"widgets-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-widgets-enums.h: xgen-wec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/widgets-enums.c: xgen-wec stamp-widgets-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi


ligma-tool-cursors.h: $(top_builddir)/cursors/ligma-tool-cursors.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_srcdir)/cursors --generate-header \
	  --target=$@ $(top_builddir)/cursors/ligma-tool-cursors.gresource.xml

ligma-tool-cursors.c: ligma-tool-cursors.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_srcdir)/cursors --generate-source \
	  --target=$@ $(top_builddir)/cursors/ligma-tool-cursors.gresource.xml
