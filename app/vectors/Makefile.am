## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Vectors\"	\
	@GTHREAD_CFLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	$(LIBART_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappvectors.a

libappvectors_a_SOURCES = \
	vectors-enums.h		\
	vectors-types.h		\
	gimpanchor.c		\
	gimpanchor.h		\
	gimpbezierstroke.h	\
	gimpbezierstroke.c	\
	gimpstroke.h		\
	gimpstroke.c		\
	gimpvectors.c		\
	gimpvectors.h		\
	gimpvectors-compat.c	\
	gimpvectors-compat.h	\
	gimpvectors-export.c	\
	gimpvectors-export.h	\
	gimpvectors-import.c	\
	gimpvectors-import.h	\
	gimpvectors-preview.c	\
	gimpvectors-preview.h	\
	gimpvectors-warp.c	\
	gimpvectors-warp.h

libappvectors_a_built_sources = \
	vectors-enums.c

EXTRA_DIST = makefile.msc

$(srcdir)/vectors-enums.c: $(srcdir)/vectors-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"vectors-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (! type)\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/vectors-enums.h > xgen-cec \
	&& cp xgen-cec $(@F) \
	&& rm -f xgen-cec

