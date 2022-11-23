## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Paint\"		\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_builddir)/app			\
	-I$(top_srcdir)/app			\
	$(CAIRO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	$(LIBMYPAINT_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libapppaint.a

libapppaint_a_sources = \
	paint-enums.h			\
	paint-types.h			\
	ligma-paint.c			\
	ligma-paint.h			\
	ligmaairbrush.c			\
	ligmaairbrush.h			\
	ligmaairbrushoptions.c		\
	ligmaairbrushoptions.h		\
	ligmabrushcore.c			\
	ligmabrushcore.h			\
	ligmabrushcore-loops.cc		\
	ligmabrushcore-loops.h		\
	ligmabrushcore-kernels.h		\
	ligmaclone.c			\
	ligmaclone.h			\
	ligmacloneoptions.c		\
	ligmacloneoptions.h		\
	ligmaconvolve.c			\
	ligmaconvolve.h			\
	ligmaconvolveoptions.c		\
	ligmaconvolveoptions.h		\
	ligmadodgeburn.c			\
	ligmadodgeburn.h			\
	ligmadodgeburnoptions.c		\
	ligmadodgeburnoptions.h		\
	ligmaeraser.c			\
	ligmaeraser.h			\
	ligmaeraseroptions.c		\
	ligmaeraseroptions.h		\
	ligmaheal.c			\
	ligmaheal.h			\
	ligmaink.c			\
	ligmaink.h			\
	ligmaink-blob.c			\
	ligmaink-blob.h			\
	ligmainkoptions.c		\
	ligmainkoptions.h		\
	ligmainkundo.c			\
	ligmainkundo.h			\
	ligmamybrushcore.c		\
	ligmamybrushcore.h		\
	ligmamybrushoptions.c		\
	ligmamybrushoptions.h		\
	ligmamybrushsurface.c		\
	ligmamybrushsurface.h		\
	ligmapaintcore.c			\
	ligmapaintcore.h			\
	ligmapaintcore-loops.cc		\
	ligmapaintcore-loops.h		\
	ligmapaintcore-stroke.c		\
	ligmapaintcore-stroke.h		\
	ligmapaintcoreundo.c		\
	ligmapaintcoreundo.h		\
	ligmapaintoptions.c		\
	ligmapaintoptions.h		\
	ligmapencil.c			\
	ligmapencil.h			\
	ligmapenciloptions.c		\
	ligmapenciloptions.h		\
	ligmapaintbrush.c		\
	ligmapaintbrush.h		\
	ligmaperspectiveclone.c		\
	ligmaperspectiveclone.h		\
	ligmaperspectivecloneoptions.c	\
	ligmaperspectivecloneoptions.h	\
	ligmasmudge.c			\
	ligmasmudge.h			\
	ligmasmudgeoptions.c		\
	ligmasmudgeoptions.h		\
	ligmasourcecore.c		\
	ligmasourcecore.h		\
	ligmasourceoptions.c		\
	ligmasourceoptions.h

libapppaint_a_built_sources = paint-enums.c

libapppaint_a_SOURCES = $(libapppaint_a_built_sources) $(libapppaint_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = stamp-paint-enums.h xgen-pec
CLEANFILES = $(gen_sources)

xgen-pec: $(srcdir)/paint-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-paint-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"paint-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-paint-enums.h: xgen-pec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/paint-enums.c: xgen-pec stamp-paint-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
