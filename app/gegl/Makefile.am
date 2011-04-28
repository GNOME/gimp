## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GEGL\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(CAIRO_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappgegl.a

libappgegl_a_sources = \
	gimp-gegl-enums.h		\
	gimp-gegl-types.h		\
	gimp-gegl.c			\
	gimp-gegl.h			\
	gimp-gegl-utils.c		\
	gimp-gegl-utils.h		\
	gimptilebackendtilemanager.c	\
	gimptilebackendtilemanager.h	\
	\
	gimpbrightnesscontrastconfig.c	\
	gimpbrightnesscontrastconfig.h	\
	gimpcageconfig.c		\
	gimpcageconfig.h		\
	gimpcolorbalanceconfig.c	\
	gimpcolorbalanceconfig.h	\
	gimpcolorizeconfig.c		\
	gimpcolorizeconfig.h		\
	gimpcurvesconfig.c		\
	gimpcurvesconfig.h		\
	gimpdesaturateconfig.c		\
	gimpdesaturateconfig.h		\
	gimphuesaturationconfig.c	\
	gimphuesaturationconfig.h	\
	gimplevelsconfig.c		\
	gimplevelsconfig.h		\
	gimpposterizeconfig.c		\
	gimpposterizeconfig.h		\
	gimpthresholdconfig.c		\
	gimpthresholdconfig.h		\
	\
	gimpoperationtilesink.c		\
	gimpoperationtilesink.h		\
	gimpoperationtilesource.c	\
	gimpoperationtilesource.h	\
	\
	gimpoperationpointfilter.c	\
	gimpoperationpointfilter.h	\
	gimpoperationcolorbalance.c	\
	gimpoperationcolorbalance.h	\
	gimpoperationcolorize.c		\
	gimpoperationcolorize.h		\
	gimpoperationcagecoefcalc.c	\
	gimpoperationcagecoefcalc.h	\
	gimpoperationcagetransform.c	\
	gimpoperationcagetransform.h	\
	gimpoperationcurves.c		\
	gimpoperationcurves.h		\
	gimpoperationdesaturate.c	\
	gimpoperationdesaturate.h	\
	gimpoperationhuesaturation.c	\
	gimpoperationhuesaturation.h	\
	gimpoperationlevels.c		\
	gimpoperationlevels.h		\
	gimpoperationposterize.c	\
	gimpoperationposterize.h	\
	gimpoperationthreshold.c	\
	gimpoperationthreshold.h	\
	\
	gimpoperationpointlayermode.c	\
	gimpoperationpointlayermode.h	\
	gimpoperationdissolvemode.c     \
	gimpoperationdissolvemode.h     \
	gimpoperationbehindmode.c       \
	gimpoperationbehindmode.h       \
	gimpoperationmultiplymode.c     \
	gimpoperationmultiplymode.h     \
	gimpoperationscreenmode.c       \
	gimpoperationscreenmode.h       \
	gimpoperationoverlaymode.c      \
	gimpoperationoverlaymode.h      \
	gimpoperationdifferencemode.c   \
	gimpoperationdifferencemode.h   \
	gimpoperationadditionmode.c     \
	gimpoperationadditionmode.h     \
	gimpoperationsubtractmode.c     \
	gimpoperationsubtractmode.h     \
	gimpoperationdarkenonlymode.c   \
	gimpoperationdarkenonlymode.h   \
	gimpoperationlightenonlymode.c  \
	gimpoperationlightenonlymode.h  \
	gimpoperationhuemode.c          \
	gimpoperationhuemode.h          \
	gimpoperationsaturationmode.c   \
	gimpoperationsaturationmode.h   \
	gimpoperationcolormode.c        \
	gimpoperationcolormode.h        \
	gimpoperationvaluemode.c        \
	gimpoperationvaluemode.h        \
	gimpoperationdividemode.c       \
	gimpoperationdividemode.h       \
	gimpoperationdodgemode.c        \
	gimpoperationdodgemode.h        \
	gimpoperationburnmode.c         \
	gimpoperationburnmode.h         \
	gimpoperationhardlightmode.c    \
	gimpoperationhardlightmode.h    \
	gimpoperationsoftlightmode.c    \
	gimpoperationsoftlightmode.h    \
	gimpoperationgrainextractmode.c \
	gimpoperationgrainextractmode.h \
	gimpoperationgrainmergemode.c   \
	gimpoperationgrainmergemode.h   \
	gimpoperationcolorerasemode.c   \
	gimpoperationcolorerasemode.h   \
	gimpoperationerasemode.c        \
	gimpoperationerasemode.h        \
	gimpoperationreplacemode.c      \
	gimpoperationreplacemode.h      \
	gimpoperationantierasemode.c    \
	gimpoperationantierasemode.h

libappgegl_a_built_sources = gimp-gegl-enums.c

libappgegl_a_SOURCES = $(libappgegl_a_built_sources) $(libappgegl_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

gimp-gegl-enums.c: $(srcdir)/gimp-gegl-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"core/core-enums.h\"\n#include \"gimp-gegl-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/gimp-gegl-enums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
