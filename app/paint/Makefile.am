## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Paint\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libapppaint.a

libapppaint_a_sources = \
	paint-enums.h		\
	paint-types.h		\
	gimp-paint.c		\
	gimp-paint.h		\
	gimpairbrush.c		\
	gimpairbrush.h		\
	gimpairbrushoptions.c	\
	gimpairbrushoptions.h	\
	gimpclone.c		\
	gimpclone.h		\
	gimpcloneoptions.c	\
	gimpcloneoptions.h	\
	gimpconvolve.c		\
	gimpconvolve.h		\
	gimpconvolveoptions.c	\
	gimpconvolveoptions.h	\
	gimpdodgeburn.c		\
	gimpdodgeburn.h		\
	gimpdodgeburnoptions.c	\
	gimpdodgeburnoptions.h	\
	gimperaser.c		\
	gimperaser.h		\
	gimperaseroptions.c	\
	gimperaseroptions.h	\
	gimppaintcore.c		\
	gimppaintcore.h		\
	gimppaintcore-kernels.h	\
	gimppaintcore-stroke.c	\
	gimppaintcore-stroke.h	\
	gimppaintcore-undo.c	\
	gimppaintcore-undo.h	\
	gimppaintoptions.c	\
	gimppaintoptions.h	\
	gimppencil.c		\
	gimppencil.h		\
	gimppenciloptions.c	\
	gimppenciloptions.h	\
	gimppaintbrush.c	\
	gimppaintbrush.h	\
	gimpsmudge.c		\
	gimpsmudge.h		\
	gimpsmudgeoptions.c	\
	gimpsmudgeoptions.h

libapppaint_a_built_sources = paint-enums.c

libapppaint_a_SOURCES = $(libapppaint_a_built_sources) $(libapppaint_a_sources)

EXTRA_DIST = makefile.msc

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-pec
CLEANFILES = $(gen_sources)

$(srcdir)/paint-enums.c: $(srcdir)/paint-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"paint-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/paint-enums.h > xgen-pec \
	&& cp xgen-pec $(@F) \
	&& rm -f xgen-pec
