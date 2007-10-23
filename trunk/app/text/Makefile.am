## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Text\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	$(PANGOFT2_CFLAGS)	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libapptext.a

libapptext_a_sources = \
	text-types.h			\
	text-enums.h			\
	gimp-fonts.c			\
	gimp-fonts.h			\
	gimpfont.c			\
	gimpfont.h			\
	gimpfont-utils.c		\
	gimpfont-utils.h		\
	gimpfontlist.c			\
	gimpfontlist.h			\
	gimptext.c			\
	gimptext.h			\
	gimptext-bitmap.c		\
	gimptext-bitmap.h		\
	gimptext-compat.c		\
	gimptext-compat.h		\
	gimptext-parasite.c		\
	gimptext-parasite.h		\
	gimptext-private.h		\
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

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

text-enums.c: $(srcdir)/text-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"text-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (! type)\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/text-enums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
