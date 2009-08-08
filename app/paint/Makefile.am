## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Paint\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GEGL_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libapppaint.a

libapppaint_a_sources = \
	paint-enums.h			\
	paint-types.h			\
	gimp-paint.c			\
	gimp-paint.h			\
	gimpairbrush.c			\
	gimpairbrush.h			\
	gimpairbrushoptions.c		\
	gimpairbrushoptions.h		\
	gimpbrushcore.c			\
	gimpbrushcore.h			\
	gimpbrushcore-kernels.h		\
	gimpclone.c			\
	gimpclone.h			\
	gimpcloneoptions.c		\
	gimpcloneoptions.h		\
	gimpconvolve.c			\
	gimpconvolve.h			\
	gimpconvolveoptions.c		\
	gimpconvolveoptions.h		\
	gimpdodgeburn.c			\
	gimpdodgeburn.h			\
	gimpdodgeburnoptions.c		\
	gimpdodgeburnoptions.h		\
	gimperaser.c			\
	gimperaser.h			\
	gimperaseroptions.c		\
	gimperaseroptions.h		\
	gimpheal.c			\
	gimpheal.h			\
	gimpink.c			\
	gimpink.h			\
	gimpink-blob.c			\
	gimpink-blob.h			\
	gimpinkoptions.c		\
	gimpinkoptions.h		\
	gimpinkundo.c			\
	gimpinkundo.h			\
	gimppaintcore.c			\
	gimppaintcore.h			\
	gimppaintcore-stroke.c		\
	gimppaintcore-stroke.h		\
	gimppaintcoreundo.c		\
	gimppaintcoreundo.h		\
	gimppaintoptions.c		\
	gimppaintoptions.h		\
	gimpdynamicsoptions.c		\
	gimpdynamicsoptions.h		\
	gimppencil.c			\
	gimppencil.h			\
	gimppenciloptions.c		\
	gimppenciloptions.h		\
	gimppaintbrush.c		\
	gimppaintbrush.h		\
	gimpperspectiveclone.c		\
	gimpperspectiveclone.h		\
	gimpperspectivecloneoptions.c	\
	gimpperspectivecloneoptions.h	\
	gimpsmudge.c			\
	gimpsmudge.h			\
	gimpsmudgeoptions.c		\
	gimpsmudgeoptions.h		\
	gimpsourcecore.c		\
	gimpsourcecore.h		\
	gimpsourceoptions.c		\
	gimpsourceoptions.h

libapppaint_a_built_sources = paint-enums.c

libapppaint_a_SOURCES = $(libapppaint_a_built_sources) $(libapppaint_a_sources)

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-pec
CLEANFILES = $(gen_sources)

paint-enums.c: $(srcdir)/paint-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"paint-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/paint-enums.h > xgen-pec \
	&& cp xgen-pec $(@F) \
	&& rm -f xgen-pec
