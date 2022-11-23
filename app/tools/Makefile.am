## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Tools\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = libapptools.a

libapptools_a_sources = \
	tools-enums.h			\
	tools-types.h			\
	ligma-tool-options-manager.c	\
	ligma-tool-options-manager.h	\
	ligma-tools.c			\
	ligma-tools.h			\
	tool_manager.c			\
	tool_manager.h			\
	\
	ligmaairbrushtool.c		\
	ligmaairbrushtool.h		\
	ligmaalignoptions.c		\
	ligmaalignoptions.h		\
	ligmaaligntool.c			\
	ligmaaligntool.h			\
	ligmabrightnesscontrasttool.c	\
	ligmabrightnesscontrasttool.h	\
	ligmabrushtool.c			\
	ligmabrushtool.h			\
	ligmabucketfilloptions.c		\
	ligmabucketfilloptions.h		\
	ligmabucketfilltool.c		\
	ligmabucketfilltool.h		\
	ligmabycolorselecttool.c		\
	ligmabycolorselecttool.h		\
	ligmacageoptions.c		\
	ligmacageoptions.h		\
	ligmacagetool.c			\
	ligmacagetool.h			\
	ligmacloneoptions-gui.c		\
	ligmacloneoptions-gui.h		\
	ligmaclonetool.c			\
	ligmaclonetool.h			\
	ligmacoloroptions.c		\
	ligmacoloroptions.h		\
	ligmacolortool.c			\
	ligmacolortool.h			\
	ligmacolorpickeroptions.c	\
	ligmacolorpickeroptions.h	\
	ligmacolorpickertool.c		\
	ligmacolorpickertool.h		\
	ligmaconvolvetool.c		\
	ligmaconvolvetool.h		\
	ligmacropoptions.c		\
	ligmacropoptions.h		\
	ligmacroptool.c			\
	ligmacroptool.h			\
	ligmacurvestool.c		\
	ligmacurvestool.h		\
	ligmadodgeburntool.c		\
	ligmadodgeburntool.h		\
	ligmadrawtool.c			\
	ligmadrawtool.h			\
	ligmaeditselectiontool.c		\
	ligmaeditselectiontool.h		\
	ligmaellipseselecttool.c		\
	ligmaellipseselecttool.h		\
	ligmaerasertool.c		\
	ligmaerasertool.h		\
	ligmafilteroptions.c		\
	ligmafilteroptions.h		\
	ligmafiltertool.c		\
	ligmafiltertool.h		\
	ligmafiltertool-settings.c	\
	ligmafiltertool-settings.h	\
	ligmafiltertool-widgets.c	\
	ligmafiltertool-widgets.h	\
	ligmaflipoptions.c		\
	ligmaflipoptions.h		\
	ligmafliptool.c			\
	ligmafliptool.h			\
	ligmaforegroundselectoptions.c	\
	ligmaforegroundselectoptions.h	\
	ligmaforegroundselecttool.c	\
	ligmaforegroundselecttool.h	\
	ligmaforegroundselecttoolundo.c	\
	ligmaforegroundselecttoolundo.h	\
	ligmafreeselecttool.c		\
	ligmafreeselecttool.h		\
	ligmafuzzyselecttool.c		\
	ligmafuzzyselecttool.h		\
	ligmagegltool.c			\
	ligmagegltool.h			\
	ligmagenerictransformtool.c	\
	ligmagenerictransformtool.h	\
	ligmagradientoptions.c		\
	ligmagradientoptions.h		\
	ligmagradienttool.c		\
	ligmagradienttool.h		\
	ligmagradienttool-editor.c	\
	ligmagradienttool-editor.h	\
	ligmaguidetool.c			\
	ligmaguidetool.h			\
	ligmahandletransformoptions.c	\
	ligmahandletransformoptions.h	\
	ligmahandletransformtool.c	\
	ligmahandletransformtool.h	\
	ligmahealtool.c			\
	ligmahealtool.h			\
	ligmahistogramoptions.c		\
	ligmahistogramoptions.h		\
	ligmainkoptions-gui.c		\
	ligmainkoptions-gui.h		\
	ligmainktool.c			\
	ligmainktool.h			\
	ligmaiscissorsoptions.c		\
	ligmaiscissorsoptions.h		\
	ligmaiscissorstool.c		\
	ligmaiscissorstool.h		\
	ligmalevelstool.c		\
	ligmalevelstool.h		\
	ligmaoffsettool.c		\
	ligmaoffsettool.h		\
	ligmaoperationtool.c		\
	ligmaoperationtool.h		\
	ligmamagnifyoptions.c		\
	ligmamagnifyoptions.h		\
	ligmamagnifytool.c		\
	ligmamagnifytool.h		\
	ligmameasureoptions.c		\
	ligmameasureoptions.h		\
	ligmameasuretool.c		\
	ligmameasuretool.h		\
	ligmamoveoptions.c		\
	ligmamoveoptions.h		\
	ligmamovetool.c			\
	ligmamovetool.h			\
	ligmamybrushoptions-gui.c	\
	ligmamybrushoptions-gui.h	\
	ligmamybrushtool.c		\
	ligmamybrushtool.h		\
	ligmanpointdeformationoptions.c	\
	ligmanpointdeformationoptions.h	\
	ligmanpointdeformationtool.c	\
	ligmanpointdeformationtool.h	\
	ligmapaintbrushtool.c		\
	ligmapaintbrushtool.h		\
	ligmapaintoptions-gui.c		\
	ligmapaintoptions-gui.h		\
	ligmapaintselectoptions.c	\
	ligmapaintselectoptions.h	\
	ligmapaintselecttool.c		\
	ligmapaintselecttool.h		\
	ligmapainttool.c			\
	ligmapainttool.h			\
	ligmapainttool-paint.c		\
	ligmapainttool-paint.h		\
	ligmapenciltool.c		\
	ligmapenciltool.h		\
	ligmaperspectiveclonetool.c	\
	ligmaperspectiveclonetool.h	\
	ligmaperspectivetool.c		\
	ligmaperspectivetool.h		\
	ligmapolygonselecttool.c		\
	ligmapolygonselecttool.h		\
	ligmarectangleselecttool.c	\
	ligmarectangleselecttool.h	\
	ligmarectangleselectoptions.c	\
	ligmarectangleselectoptions.h	\
	ligmarectangleoptions.c		\
	ligmarectangleoptions.h		\
	ligmaregionselectoptions.c	\
	ligmaregionselectoptions.h	\
	ligmaregionselecttool.c		\
	ligmaregionselecttool.h		\
	ligmarotatetool.c		\
	ligmarotatetool.h		\
	ligmasamplepointtool.c		\
	ligmasamplepointtool.h		\
	ligmascaletool.c			\
	ligmascaletool.h			\
	ligmaseamlesscloneoptions.c	\
	ligmaseamlesscloneoptions.h	\
	ligmaseamlessclonetool.c		\
	ligmaseamlessclonetool.h		\
	ligmaselectionoptions.c		\
	ligmaselectionoptions.h		\
	ligmaselectiontool.c		\
	ligmaselectiontool.h		\
	ligmasheartool.c			\
	ligmasheartool.h			\
	ligmasmudgetool.c		\
	ligmasmudgetool.h		\
	ligmasourcetool.c		\
	ligmasourcetool.h		\
	ligmatextoptions.c		\
	ligmatextoptions.h		\
	ligmatexttool.c			\
	ligmatexttool.h			\
	ligmatexttool-editor.c		\
	ligmatexttool-editor.h		\
	ligmathresholdtool.c		\
	ligmathresholdtool.h		\
	ligmatilehandleriscissors.c	\
	ligmatilehandleriscissors.h	\
	ligmatool.c			\
	ligmatool.h			\
	ligmatool-progress.c		\
	ligmatool-progress.h		\
	ligmatoolcontrol.c		\
	ligmatoolcontrol.h		\
	ligmatooloptions-gui.c		\
	ligmatooloptions-gui.h		\
	ligmatools-utils.c		\
	ligmatools-utils.h		\
	ligmatransform3doptions.c	\
	ligmatransform3doptions.h	\
	ligmatransform3dtool.c		\
	ligmatransform3dtool.h		\
	ligmatransformgridoptions.c     	\
	ligmatransformgridoptions.h     	\
	ligmatransformgridtool.c     	\
	ligmatransformgridtool.h     	\
	ligmatransformgridtoolundo.c	\
	ligmatransformgridtoolundo.h	\
	ligmatransformoptions.c     	\
	ligmatransformoptions.h     	\
	ligmatransformtool.c		\
	ligmatransformtool.h		\
	ligmaunifiedtransformtool.c	\
	ligmaunifiedtransformtool.h	\
	ligmavectoroptions.c		\
	ligmavectoroptions.h		\
	ligmavectortool.c		\
	ligmavectortool.h		\
	ligmawarpoptions.c		\
	ligmawarpoptions.h		\
	ligmawarptool.c			\
	ligmawarptool.h

libapptools_a_built_sources = tools-enums.c

libapptools_a_SOURCES = $(libapptools_a_built_sources) $(libapptools_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = stamp-tools-enums.h xgen-tec
CLEANFILES = $(gen_sources)

xgen-tec: $(srcdir)/tools-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-tools-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"core/core-enums.h\"\n#include \"tools-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-tools-enums.h: xgen-tec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/tools-enums.c: xgen-tec stamp-tools-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
