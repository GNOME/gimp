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

noinst_LIBRARIES = \
	libappgegl-generic.a	\
	libappgegl-sse2.a	\
	libappgegl.a

libappgegl_generic_a_sources = \
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
	gimp-gegl-loops.cc		\
	gimp-gegl-loops.h		\
	gimp-gegl-mask.c		\
	gimp-gegl-mask.h		\
	gimp-gegl-mask-combine.cc	\
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

libappgegl_generic_a_built_sources = gimp-gegl-enums.c

libappgegl_sse2_a_sources = \
	gimp-gegl-loops-sse2.c		\
	gimp-gegl-loops-sse2.h

libappgegl_generic_a_SOURCES = $(libappgegl_generic_a_built_sources) $(libappgegl_generic_a_sources)

libappgegl_sse2_a_SOURCES = $(libappgegl_sse2_a_sources)

libappgegl_sse2_a_CFLAGS = $(SSE2_EXTRA_CFLAGS)

libappgegl_a_SOURCES =


libappgegl.a: libappgegl-generic.a \
	      libappgegl-sse2.a
	$(AR) $(ARFLAGS) libappgegl.a \
	  $(libappgegl_generic_a_OBJECTS) \
	  $(libappgegl_sse2_a_OBJECTS)
	$(RANLIB) libappgegl.a


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-ggec
CLEANFILES = $(gen_sources)

xgen-ggec: $(srcdir)/gimp-gegl-enums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"core/core-enums.h\"\n#include \"gimp-gegl-enums.h\"\n#include \"gimp-intl.h\"" \
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
$(srcdir)/gimp-gegl-enums.c: xgen-ggec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
