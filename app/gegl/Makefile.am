## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-GEGL\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(CAIRO_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappgegl.a

libappgegl_a_sources = \
	gimp-gegl-enums.h		\
	gimp-gegl-types.h		\
	gimp-babl.c			\
	gimp-babl.h			\
	gimp-babl-compat.c		\
	gimp-babl-compat.h		\
	gimp-gegl.c			\
	gimp-gegl.h			\
	gimp-gegl-apply-operation.c	\
	gimp-gegl-apply-operation.h	\
	gimp-gegl-config.c		\
	gimp-gegl-config.h		\
	gimp-gegl-loops.c		\
	gimp-gegl-loops.h		\
	gimp-gegl-mask.c		\
	gimp-gegl-mask.h		\
	gimp-gegl-mask-combine.c	\
	gimp-gegl-mask-combine.h	\
	gimp-gegl-nodes.c		\
	gimp-gegl-nodes.h		\
	gimp-gegl-tile-compat.c		\
	gimp-gegl-tile-compat.h		\
	gimp-gegl-utils.c		\
	gimp-gegl-utils.h		\
	gimpapplicator.c		\
	gimpapplicator.h		\
	gimptilehandlervalidate.c	\
	gimptilehandlervalidate.h

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
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"core/core-enums.h\"\n#include \"gimp-gegl-enums.h\"\n#include \"gimp-intl.h\"" \
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
