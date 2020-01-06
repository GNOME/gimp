## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Tools\"	\
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
	gimp-tool-options-manager.c	\
	gimp-tool-options-manager.h	\
	gimp-tools.c			\
	gimp-tools.h			\
	tool_manager.c			\
	tool_manager.h			\
	\
	gimpairbrushtool.c		\
	gimpairbrushtool.h		\
	gimpalignoptions.c		\
	gimpalignoptions.h		\
	gimpaligntool.c			\
	gimpaligntool.h			\
	gimpbrightnesscontrasttool.c	\
	gimpbrightnesscontrasttool.h	\
	gimpbrushtool.c			\
	gimpbrushtool.h			\
	gimpbucketfilloptions.c		\
	gimpbucketfilloptions.h		\
	gimpbucketfilltool.c		\
	gimpbucketfilltool.h		\
	gimpbycolorselecttool.c		\
	gimpbycolorselecttool.h		\
	gimpcageoptions.c		\
	gimpcageoptions.h		\
	gimpcagetool.c			\
	gimpcagetool.h			\
	gimpcloneoptions-gui.c		\
	gimpcloneoptions-gui.h		\
	gimpclonetool.c			\
	gimpclonetool.h			\
	gimpcoloroptions.c		\
	gimpcoloroptions.h		\
	gimpcolortool.c			\
	gimpcolortool.h			\
	gimpcolorpickeroptions.c	\
	gimpcolorpickeroptions.h	\
	gimpcolorpickertool.c		\
	gimpcolorpickertool.h		\
	gimpconvolvetool.c		\
	gimpconvolvetool.h		\
	gimpcropoptions.c		\
	gimpcropoptions.h		\
	gimpcroptool.c			\
	gimpcroptool.h			\
	gimpcurvestool.c		\
	gimpcurvestool.h		\
	gimpdodgeburntool.c		\
	gimpdodgeburntool.h		\
	gimpdrawtool.c			\
	gimpdrawtool.h			\
	gimpeditselectiontool.c		\
	gimpeditselectiontool.h		\
	gimpellipseselecttool.c		\
	gimpellipseselecttool.h		\
	gimperasertool.c		\
	gimperasertool.h		\
	gimpfilteroptions.c		\
	gimpfilteroptions.h		\
	gimpfiltertool.c		\
	gimpfiltertool.h		\
	gimpfiltertool-settings.c	\
	gimpfiltertool-settings.h	\
	gimpfiltertool-widgets.c	\
	gimpfiltertool-widgets.h	\
	gimpflipoptions.c		\
	gimpflipoptions.h		\
	gimpfliptool.c			\
	gimpfliptool.h			\
	gimpforegroundselectoptions.c	\
	gimpforegroundselectoptions.h	\
	gimpforegroundselecttool.c	\
	gimpforegroundselecttool.h	\
	gimpforegroundselecttoolundo.c	\
	gimpforegroundselecttoolundo.h	\
	gimpfreeselecttool.c		\
	gimpfreeselecttool.h		\
	gimpfuzzyselecttool.c		\
	gimpfuzzyselecttool.h		\
	gimpgegltool.c			\
	gimpgegltool.h			\
	gimpgenerictransformtool.c	\
	gimpgenerictransformtool.h	\
	gimpgradientoptions.c		\
	gimpgradientoptions.h		\
	gimpgradienttool.c		\
	gimpgradienttool.h		\
	gimpgradienttool-editor.c	\
	gimpgradienttool-editor.h	\
	gimpguidetool.c			\
	gimpguidetool.h			\
	gimphandletransformoptions.c	\
	gimphandletransformoptions.h	\
	gimphandletransformtool.c	\
	gimphandletransformtool.h	\
	gimphealtool.c			\
	gimphealtool.h			\
	gimphistogramoptions.c		\
	gimphistogramoptions.h		\
	gimpinkoptions-gui.c		\
	gimpinkoptions-gui.h		\
	gimpinktool.c			\
	gimpinktool.h			\
	gimpiscissorsoptions.c		\
	gimpiscissorsoptions.h		\
	gimpiscissorstool.c		\
	gimpiscissorstool.h		\
	gimplevelstool.c		\
	gimplevelstool.h		\
	gimpoffsettool.c		\
	gimpoffsettool.h		\
	gimpoperationtool.c		\
	gimpoperationtool.h		\
	gimpmagnifyoptions.c		\
	gimpmagnifyoptions.h		\
	gimpmagnifytool.c		\
	gimpmagnifytool.h		\
	gimpmeasureoptions.c		\
	gimpmeasureoptions.h		\
	gimpmeasuretool.c		\
	gimpmeasuretool.h		\
	gimpmoveoptions.c		\
	gimpmoveoptions.h		\
	gimpmovetool.c			\
	gimpmovetool.h			\
	gimpmybrushoptions-gui.c	\
	gimpmybrushoptions-gui.h	\
	gimpmybrushtool.c		\
	gimpmybrushtool.h		\
	gimpnpointdeformationoptions.c	\
	gimpnpointdeformationoptions.h	\
	gimpnpointdeformationtool.c	\
	gimpnpointdeformationtool.h	\
	gimppaintbrushtool.c		\
	gimppaintbrushtool.h		\
	gimppaintoptions-gui.c		\
	gimppaintoptions-gui.h		\
	gimppainttool.c			\
	gimppainttool.h			\
	gimppainttool-paint.c		\
	gimppainttool-paint.h		\
	gimppenciltool.c		\
	gimppenciltool.h		\
	gimpperspectiveclonetool.c	\
	gimpperspectiveclonetool.h	\
	gimpperspectivetool.c		\
	gimpperspectivetool.h		\
	gimppolygonselecttool.c		\
	gimppolygonselecttool.h		\
	gimprectangleselecttool.c	\
	gimprectangleselecttool.h	\
	gimprectangleselectoptions.c	\
	gimprectangleselectoptions.h	\
	gimprectangleoptions.c		\
	gimprectangleoptions.h		\
	gimpregionselectoptions.c	\
	gimpregionselectoptions.h	\
	gimpregionselecttool.c		\
	gimpregionselecttool.h		\
	gimprotatetool.c		\
	gimprotatetool.h		\
	gimpsamplepointtool.c		\
	gimpsamplepointtool.h		\
	gimpscaletool.c			\
	gimpscaletool.h			\
	gimpseamlesscloneoptions.c	\
	gimpseamlesscloneoptions.h	\
	gimpseamlessclonetool.c		\
	gimpseamlessclonetool.h		\
	gimpselectionoptions.c		\
	gimpselectionoptions.h		\
	gimpselectiontool.c		\
	gimpselectiontool.h		\
	gimpsheartool.c			\
	gimpsheartool.h			\
	gimpsmudgetool.c		\
	gimpsmudgetool.h		\
	gimpsourcetool.c		\
	gimpsourcetool.h		\
	gimptextoptions.c		\
	gimptextoptions.h		\
	gimptexttool.c			\
	gimptexttool.h			\
	gimptexttool-editor.c		\
	gimptexttool-editor.h		\
	gimpthresholdtool.c		\
	gimpthresholdtool.h		\
	gimptilehandleriscissors.c	\
	gimptilehandleriscissors.h	\
	gimptool.c			\
	gimptool.h			\
	gimptool-progress.c		\
	gimptool-progress.h		\
	gimptoolcontrol.c		\
	gimptoolcontrol.h		\
	gimptooloptions-gui.c		\
	gimptooloptions-gui.h		\
	gimptools-utils.c		\
	gimptools-utils.h		\
	gimptransform3doptions.c	\
	gimptransform3doptions.h	\
	gimptransform3dtool.c		\
	gimptransform3dtool.h		\
	gimptransformgridoptions.c     	\
	gimptransformgridoptions.h     	\
	gimptransformgridtool.c     	\
	gimptransformgridtool.h     	\
	gimptransformgridtoolundo.c	\
	gimptransformgridtoolundo.h	\
	gimptransformoptions.c     	\
	gimptransformoptions.h     	\
	gimptransformtool.c		\
	gimptransformtool.h		\
	gimpunifiedtransformtool.c	\
	gimpunifiedtransformtool.h	\
	gimpvectoroptions.c		\
	gimpvectoroptions.h		\
	gimpvectortool.c		\
	gimpvectortool.h		\
	gimpwarpoptions.c		\
	gimpwarpoptions.h		\
	gimpwarptool.c			\
	gimpwarptool.h

libapptools_a_built_sources = tools-enums.c

libapptools_a_SOURCES = $(libapptools_a_built_sources) $(libapptools_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

xgen-tec: $(srcdir)/tools-enums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"core/core-enums.h\"\n#include \"tools-enums.h\"\n#include \"gimp-intl.h\"" \
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
$(srcdir)/tools-enums.c: xgen-tec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
