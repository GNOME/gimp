## Process this file with automake to produce Makefile.in

libgimptoolincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimptool

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpTool\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

lib_LTLIBRARIES = libgimptool-1.3.la

libgimptool_1_3_la_SOURCES = \
	gimptooltypes.h		\
        gimptoolenums.h         \
        gimptoolenums.c         \
	gimptool.c		\
	gimptool.h		\
	gimptoolcontrol.h	\
	gimptoolmodule.c	\
	gimptoolmodule.h

libgimptoolinclude_HEADERS = \
	gimptool.h		\
	gimptooltypes.h		\
	gimptoolenums.h		\
	gimptoolmodule.h

libgimptool_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

libgimptool_1_3_la_LIBADD = $(GLIB_LIBS) 

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

$(srcdir)/gimptoolenums.c: $(srcdir)/gimptoolenums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpproxy/gimpproxytypes.h\"\n#include \"gimptoolenums.h\"\n#include \"libgimp/gimpintl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/gimptoolenums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
