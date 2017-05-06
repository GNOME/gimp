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
	gimp-fonts.c			\
	gimp-fonts.h			\
	gimpfont.c			\
	gimpfont.h			\
	gimpfontlist.c			\
	gimpfontlist.h			\
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

$(srcdir)/text-enums.c: $(srcdir)/text-enums.h $(GIMP_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"text-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		text-enums.h) > xgen-tec \
	&& cp xgen-tec $@ \
	&& rm -f xgen-tec
