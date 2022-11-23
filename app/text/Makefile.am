## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Text\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GEGL_CFLAGS)			\
	$(PANGOCAIRO_CFLAGS)		\
	$(HARFBUZZ_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libapptext.a

libapptext_a_sources = \
	text-types.h			\
	text-enums.h			\
	ligmafont.c			\
	ligmafont.h			\
	ligmafontfactory.c		\
	ligmafontfactory.h		\
	ligmatext.c			\
	ligmatext.h			\
	ligmatext-compat.c		\
	ligmatext-compat.h		\
	ligmatext-parasite.c		\
	ligmatext-parasite.h		\
	ligmatext-vectors.c		\
	ligmatext-vectors.h		\
	ligmatext-xlfd.c			\
	ligmatext-xlfd.h			\
	ligmatextlayer.c			\
	ligmatextlayer.h			\
	ligmatextlayer-transform.c	\
	ligmatextlayer-transform.h	\
	ligmatextlayer-xcf.c		\
	ligmatextlayer-xcf.h		\
	ligmatextlayout.c		\
	ligmatextlayout.h		\
	ligmatextlayout-render.c		\
	ligmatextlayout-render.h		\
	ligmatextundo.c			\
	ligmatextundo.h

libapptext_a_built_sources = text-enums.c

libapptext_a_SOURCES = $(libapptext_a_built_sources) $(libapptext_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = stamp-text-enums.h xgen-tec
CLEANFILES = $(gen_sources)

xgen-tec: $(srcdir)/text-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-text-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"text-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-text-enums.h: xgen-tec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/text-enums.c: xgen-tec stamp-text-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
