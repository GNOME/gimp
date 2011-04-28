## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Base\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(CAIRO_CFLAGS)		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappbase.a

libappbase_a_SOURCES = \
	base.c			\
	base.h			\
	base-enums.c		\
	base-enums.h		\
	base-types.h		\
	base-utils.c		\
	base-utils.h		\
	boundary.c		\
	boundary.h		\
	color-balance.c		\
	color-balance.h		\
	colorize.c		\
	colorize.h		\
	cpercep.c		\
	cpercep.h		\
	curves.c		\
	curves.h		\
	desaturate.c		\
	desaturate.h		\
	gimphistogram.c		\
	gimphistogram.h		\
	gimplut.c		\
	gimplut.h		\
	hue-saturation.c	\
	hue-saturation.h	\
	levels.c		\
	levels.h		\
	lut-funcs.c		\
	lut-funcs.h		\
	pixel-processor.c	\
	pixel-processor.h	\
	pixel-region.c		\
	pixel-region.h		\
	pixel-surround.c	\
	pixel-surround.h	\
	siox.c			\
	siox.h			\
	temp-buf.c		\
	temp-buf.h		\
	threshold.c		\
	threshold.h		\
	tile.c			\
	tile.h			\
	tile-private.h		\
	tile-cache.c		\
	tile-cache.h		\
	tile-manager.c		\
	tile-manager.h		\
	tile-manager-preview.c	\
	tile-manager-preview.h	\
	tile-manager-private.h	\
	tile-pyramid.c		\
	tile-pyramid.h		\
	tile-rowhints.c		\
	tile-rowhints.h		\
	tile-swap.c		\
	tile-swap.h

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-bec
CLEANFILES = $(gen_sources)

base-enums.c: $(srcdir)/base-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"base-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/base-enums.h > xgen-bec \
	&& cp xgen-bec $(@F) \
	&& rm -f xgen-bec
