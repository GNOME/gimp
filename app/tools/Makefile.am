## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapptools.a

libapptools_a_SOURCES = @STRIP_BEGIN@ \
	tools-enums.h			\
	tools-enums.c			\
	tools-types.h			\
	tools.c				\
	tools.h				\
	gimpairbrushtool.c		\
	gimpairbrushtool.h		\
	gimpbezierselecttool.c  	\
	gimpbezierselecttool.h  	\
	gimpblendtool.c			\
	gimpblendtool.h			\
	gimpbrightnesscontrasttool.c	\
	gimpbrightnesscontrasttool.h	\
	gimpbucketfilltool.c		\
	gimpbucketfilltool.h		\
	gimpbycolorselecttool.c		\
	gimpbycolorselecttool.h		\
	gimpclonetool.c			\
	gimpclonetool.h			\
	gimpcolorbalancetool.c		\
	gimpcolorbalancetool.h		\
	gimpcolorbalancetool-transfer.c	\
	gimpcolorbalancetool-transfer.h	\
	gimpcolorpickertool.c		\
	gimpcolorpickertool.h		\
	gimpconvolvetool.c		\
	gimpconvolvetool.h		\
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
	gimpfliptool.c			\
	gimpfliptool.h			\
	gimpfreeselecttool.c		\
	gimpfreeselecttool.h		\
	gimpfuzzyselecttool.c		\
	gimpfuzzyselecttool.h		\
	gimphistogramtool.c		\
	gimphistogramtool.h		\
	gimphuesaturationtool.c		\
	gimphuesaturationtool.h		\
	gimpimagemaptool.c		\
	gimpimagemaptool.h		\
	gimpinktool.c			\
	gimpinktool.h			\
	gimpinktool-blob.c		\
	gimpinktool-blob.h		\
	gimpiscissorstool.c		\
	gimpiscissorstool.h		\
	gimplevelstool.c		\
	gimplevelstool.h		\
	gimpmagnifytool.c		\
	gimpmagnifytool.h		\
	gimpmeasuretool.c		\
	gimpmeasuretool.h		\
	gimpmovetool.c			\
	gimpmovetool.h			\
	gimppaintbrushtool.c		\
	gimppaintbrushtool.h		\
	gimppainttool.c			\
	gimppainttool.h			\
	gimppathtool.c			\
	gimppathtool.h			\
	gimppenciltool.c		\
	gimppenciltool.h		\
	gimpperspectivetool.c		\
	gimpperspectivetool.h		\
	gimpposterizetool.c		\
	gimpposterizetool.h		\
	gimprectselecttool.c		\
	gimprectselecttool.h		\
	gimprotatetool.c		\
	gimprotatetool.h		\
	gimpscaletool.c			\
	gimpscaletool.h			\
	gimpselectiontool.c		\
	gimpselectiontool.h		\
	gimpsheartool.c			\
	gimpsheartool.h			\
	gimpsmudgetool.c		\
	gimpsmudgetool.h		\
	gimptexttool.c			\
	gimptexttool.h			\
	gimpthresholdtool.c		\
	gimpthresholdtool.h		\
	gimptool.c			\
	gimptool.h			\
	gimptoolmodule.c		\
	gimptoolmodule.h		\
	gimptransformtool.c		\
	gimptransformtool.h		\
	gimpvectortool.c		\
	gimpvectortool.h		\
	\
	transform_options.c     	\
	transform_options.h     	\
	paint_options.c			\
	paint_options.h			\
	selection_options.c		\
	selection_options.h		\
	tool_options.c			\
	tool_options.h			\
	\
	tool_manager.c			\
	tool_manager.h			\
	path_tool.h			\
	path_tool.c			\
	path_toolP.h			\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Tools\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	$(PANGOFT2_CFLAGS)		\
	-I$(includedir)			\
@STRIP_END@

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

$(srcdir)/tools-enums.c: $(srcdir)/tools-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"tools-enums.h\"\n#include \"libgimp/gimpintl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/tools-enums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
