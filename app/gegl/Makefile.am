## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-GEGL\"	\
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
	ligma-gegl-enums.h		\
	ligma-gegl-types.h		\
	ligma-babl.c			\
	ligma-babl.h			\
	ligma-babl-compat.c		\
	ligma-babl-compat.h		\
	ligma-gegl.c			\
	ligma-gegl.h			\
	ligma-gegl-apply-operation.c	\
	ligma-gegl-apply-operation.h	\
	ligma-gegl-loops.cc		\
	ligma-gegl-loops.h		\
	ligma-gegl-mask.c		\
	ligma-gegl-mask.h		\
	ligma-gegl-mask-combine.cc	\
	ligma-gegl-mask-combine.h	\
	ligma-gegl-nodes.c		\
	ligma-gegl-nodes.h		\
	ligma-gegl-tile-compat.c		\
	ligma-gegl-tile-compat.h		\
	ligma-gegl-utils.c		\
	ligma-gegl-utils.h		\
	ligmaapplicator.c		\
	ligmaapplicator.h		\
	ligmatilehandlervalidate.c	\
	ligmatilehandlervalidate.h

libappgegl_generic_a_built_sources = ligma-gegl-enums.c

libappgegl_sse2_a_sources = \
	ligma-gegl-loops-sse2.c		\
	ligma-gegl-loops-sse2.h

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
gen_sources = stamp-ligma-gegl-enums.h xgen-ggec
CLEANFILES = $(gen_sources)

xgen-ggec: $(srcdir)/ligma-gegl-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-ligma-gegl-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"core/core-enums.h\"\n#include \"ligma-gegl-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-ligma-gegl-enums.h: xgen-ggec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/ligma-gegl-enums.c: xgen-ggec stamp-ligma-gegl-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
