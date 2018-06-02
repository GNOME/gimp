## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Text\"	\
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
	gimpfont.c			\
	gimpfont.h			\
	gimpfontfactory.c		\
	gimpfontfactory.h		\
	gimptext.c			\
	gimptext.h			\
	gimptext-compat.c		\
	gimptext-compat.h		\
	gimptext-parasite.c		\
	gimptext-parasite.h		\
	gimptext-vectors.c		\
	gimptext-vectors.h		\
	gimptext-xlfd.c			\
	gimptext-xlfd.h			\
	gimptextlayer.c			\
	gimptextlayer.h			\
	gimptextlayer-transform.c	\
	gimptextlayer-transform.h	\
	gimptextlayer-xcf.c		\
	gimptextlayer-xcf.h		\
	gimptextlayout.c		\
	gimptextlayout.h		\
	gimptextlayout-render.c		\
	gimptextlayout-render.h		\
	gimptextundo.c			\
	gimptextundo.h

libapptext_a_built_sources = text-enums.c

libapptext_a_SOURCES = $(libapptext_a_built_sources) $(libapptext_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

xgen-tec: $(srcdir)/text-enums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"text-enums.h\"\n#include \"gimp-intl.h\"" \
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
$(srcdir)/text-enums.c: xgen-tec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
