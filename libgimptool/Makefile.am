## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimptool_def = gimptool.def
libgimptool_export_symbols = -export-symbols gimptool.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimptool-1.3.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimptool-1.3.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimptool-1.3.lib

install-ms-lib:
	$(INSTALL) gimptool-1.3.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimptool-1.3.lib

gimptool-1.3.lib: gimptool.def
	lib -name:libgimptool-1.3-@LT_CURRENT_MINUS_AGE@.dll -def:gimptool.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimptoolincludedir = $(includedir)/gimp-$(LT_RELEASE)/libgimptool

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpTool\"	\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GMODULE_CFLAGS) \
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
	gimptoolmodule.h	\
	glue.c

libgimptoolinclude_HEADERS = \
	gimptool.h		\
	gimptooltypes.h		\
	gimptoolenums.h		\
	gimptoolmodule.h

libgimptool_1_3_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	$(no_undefined) \
	$(libgimptool_export_symbols)

libgimptool_1_3_la_LIBADD = $(GLIB_LIBS) $(GMODULE_LIBS)

libgimptool_1_3_la_DEPENDENCIES = $(gimptool_def)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

$(srcdir)/gimptoolenums.c: $(srcdir)/gimptoolenums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"libgimpproxy/gimpproxytypes.h\"\n#include \"gimptoolenums.h\"\n#include \"libgimp/libgimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "\nstatic const GEnumValue @enum_name@_enum_values[] =\n{" \
		--vprod "  { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "  { 0, NULL, NULL }\n};\n\nGType\n@enum_name@_get_type (void)\n{\n  static GType enum_type = 0;\n\n  if (!enum_type)\n    enum_type = g_enum_register_static (\"@EnumName@\", @enum_name@_enum_values);\n\n  return enum_type;\n}\n" \
		$(srcdir)/gimptoolenums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
