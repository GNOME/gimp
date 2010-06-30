## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Tools\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GEGL_CFLAGS)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libapptools.a

libapptools_a_sources = \
	tools-enums.h			\
	tools-types.h			\
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
	gimpblendoptions.c		\
	gimpblendoptions.h		\
	gimpblendtool.c			\
	gimpblendtool.h			\
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
	gimpcageoptions.c			\
	gimpcageoptions.h			\
	gimpcagetool.c				\
	gimpcagetool.h				\
	gimpclonetool.c			\
	gimpclonetool.h			\
	gimpcoloroptions.c		\
	gimpcoloroptions.h		\
	gimpcolortool.c			\
	gimpcolortool.h			\
	gimpcolorbalancetool.c		\
	gimpcolorbalancetool.h		\
	gimpcolorizetool.c		\
	gimpcolorizetool.h		\
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
	gimpdesaturatetool.c		\
	gimpdesaturatetool.h		\
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
	gimpflipoptions.c		\
	gimpflipoptions.h		\
	gimpfliptool.c			\
	gimpfliptool.h			\
	gimpfreeselecttool.c		\
	gimpfreeselecttool.h		\
	gimpforegroundselectoptions.c	\
	gimpforegroundselectoptions.h	\
	gimpforegroundselecttool.c	\
	gimpforegroundselecttool.h	\
	gimpforegroundselecttoolundo.c	\
	gimpforegroundselecttoolundo.h	\
	gimpfuzzyselecttool.c		\
	gimpfuzzyselecttool.h		\
	gimpgegltool.c			\
	gimpgegltool.h			\
	gimphealtool.c			\
	gimphealtool.h			\
	gimphistogramoptions.c		\
	gimphistogramoptions.h		\
	gimphuesaturationtool.c		\
	gimphuesaturationtool.h		\
	gimpimagemapoptions.c		\
	gimpimagemapoptions.h		\
	gimpimagemaptool.c		\
	gimpimagemaptool.h		\
	gimpimagemaptool-settings.c	\
	gimpimagemaptool-settings.h	\
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
	gimppaintbrushtool.c		\
	gimppaintbrushtool.h		\
	gimppaintoptions-gui.c		\
	gimppaintoptions-gui.h		\
	gimppainttool.c			\
	gimppainttool.h			\
	gimppenciltool.c		\
	gimppenciltool.h		\
	gimpperspectiveclonetool.c	\
	gimpperspectiveclonetool.h	\
	gimpperspectivetool.c		\
	gimpperspectivetool.h		\
	gimpposterizetool.c		\
	gimpposterizetool.h		\
	gimprectangleselecttool.c	\
	gimprectangleselecttool.h	\
	gimprectangleselectoptions.c	\
	gimprectangleselectoptions.h	\
	gimprectangletool.c		\
	gimprectangletool.h		\
	gimprectangleoptions.c		\
	gimprectangleoptions.h		\
	gimpregionselectoptions.c	\
	gimpregionselectoptions.h	\
	gimpregionselecttool.c		\
	gimpregionselecttool.h		\
	gimprotatetool.c		\
	gimprotatetool.h		\
	gimpscaletool.c			\
	gimpscaletool.h			\
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
	gimptool.c			\
	gimptool.h			\
	gimptoolcontrol.c		\
	gimptoolcontrol.h		\
	gimptooloptions-gui.c		\
	gimptooloptions-gui.h		\
	gimptransformoptions.c     	\
	gimptransformoptions.h     	\
	gimptransformtool.c		\
	gimptransformtool.h		\
	gimptransformtoolundo.c		\
	gimptransformtoolundo.h		\
	gimpvectoroptions.c		\
	gimpvectoroptions.h		\
	gimpvectortool.c		\
	gimpvectortool.h

libapptools_a_built_sources = tools-enums.c

libapptools_a_SOURCES = $(libapptools_a_built_sources) $(libapptools_a_sources)

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

tools-enums.c: $(srcdir)/tools-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"core/core-enums.h\"\n#include \"tools-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/tools-enums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
