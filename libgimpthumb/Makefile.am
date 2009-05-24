## Process this file with automake to produce Makefile.in

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gimpthumb_def = gimpthumb.def
libgimpthumb_export_symbols = -export-symbols gimpthumb.def

install-libtool-import-lib:
	$(INSTALL) .libs/libgimpthumb-$(GIMP_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) gimpthumb.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libgimpthumb-$(GIMP_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/gimpthumb.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gimpthumb-$(GIMP_API_VERSION).lib

install-ms-lib:
	$(INSTALL) gimpthumb-$(GIMP_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gimpthumb-$(GIMP_API_VERSION).lib

gimpthumb-@GIMP_API_VERSION@.lib: gimpthumb.def
	lib -name:libgimpthumb-$(GIMP_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gimpthumb.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libgimpthumbincludedir = $(includedir)/gimp-$(GIMP_API_VERSION)/libgimpthumb

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibGimpThumb\"

INCLUDES = \
	-I$(top_srcdir)		\
	$(GDK_PIXBUF_CFLAGS)	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

EXTRA_DIST = \
	makefile.msc	\
	gimpthumb.def

lib_LTLIBRARIES = libgimpthumb-2.0.la

libgimpthumb_2_0_la_SOURCES = \
	gimpthumb.h		\
	gimpthumb-enums.c	\
	gimpthumb-enums.h	\
	gimpthumb-error.c	\
	gimpthumb-error.h	\
	gimpthumb-types.h	\
	gimpthumb-utils.c	\
	gimpthumb-utils.h	\
	gimpthumbnail.c		\
	gimpthumbnail.h

libgimpthumbinclude_HEADERS = \
	gimpthumb.h		\
	gimpthumb-enums.h	\
	gimpthumb-error.h	\
	gimpthumb-types.h	\
	gimpthumb-utils.h	\
	gimpthumbnail.h

libgimpthumb_2_0_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libgimpthumb_export_symbols)

libgimpthumb_2_0_la_LIBADD = $(GDK_PIXBUF_LIBS) $(GLIB_LIBS) 


noinst_PROGRAMS = gimp-thumbnail-list

gimp_thumbnail_list_SOURCES = gimp-thumbnail-list.c

gimp_thumbnail_list_LDADD = \
	libgimpthumb-$(GIMP_API_VERSION).la \
	$(GDK_PIXBUF_LIBS) \
	$(GLIB_LIBS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

gimpthumb-enums.c: $(srcdir)/gimpthumb-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"gimpthumb-enums.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    type = g_@type@_register_static (\"@EnumName@\", values);\n\n  return type;\n}\n" \
		$(srcdir)/gimpthumb-enums.h > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
