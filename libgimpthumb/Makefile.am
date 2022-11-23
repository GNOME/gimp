## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_cocoa = -framework Cocoa
endif

if OS_WIN32
ligmathumb_def = ligmathumb.def
libligmathumb_export_symbols = -export-symbols $(srcdir)/ligmathumb.def

install-libtool-import-lib:
	$(INSTALL) .libs/libligmathumb-$(LIGMA_API_VERSION).dll.a $(DESTDIR)$(libdir)
	$(INSTALL) $(srcdir)/ligmathumb.def $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libligmathumb-$(LIGMA_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/ligmathumb.def
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = ligmathumb-$(LIGMA_API_VERSION).lib

install-ms-lib:
	$(INSTALL) ligmathumb-$(LIGMA_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/ligmathumb-$(LIGMA_API_VERSION).lib

ligmathumb-@LIGMA_API_VERSION@.lib: ligmathumb.def
	lib -name:libligmathumb-$(LIGMA_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:ligmathumb.def -out:$@

else
install-ms-lib:
uninstall-ms-lib:
endif

libligmathumbincludedir = $(includedir)/ligma-$(LIGMA_API_VERSION)/libligmathumb

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"LibLigmaThumb\"	\
	-DLIGMA_THUMB_COMPILATION	\
	-I$(top_srcdir)			\
	$(GDK_PIXBUF_CFLAGS)		\
	$(GIO_CFLAGS)			\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

EXTRA_DIST = \
	ligmathumb.def

lib_LTLIBRARIES = libligmathumb-@LIGMA_API_VERSION@.la

AUTOMAKE_OPTIONS = subdir-objects
include $(top_srcdir)/libligmathumb/Makefile.gi

libligmathumb_@LIGMA_API_VERSION@_la_SOURCES = \
	ligmathumb.h		\
	ligmathumb-enums.c	\
	$(libligmathumb_introspectable)

libligmathumbinclude_HEADERS = \
	ligmathumb.h		\
	$(libligmathumb_introspectable_headers)

libligmathumb_@LIGMA_API_VERSION@_la_LDFLAGS = \
	-version-info $(LT_VERSION_INFO)	\
	$(no_undefined)				\
	$(libligmabase)				\
	$(libligmathumb_export_symbols)		\
	$(framework_cocoa)			\
	$(xnone)

libligmathumb_@LIGMA_API_VERSION@_la_LIBADD = \
	$(GDK_PIXBUF_LIBS)	\
	$(GIO_LIBS)


noinst_PROGRAMS = ligma-thumbnail-list

ligma_thumbnail_list_SOURCES = ligma-thumbnail-list.c

ligma_thumbnail_list_LDADD = \
	libligmathumb-$(LIGMA_API_VERSION).la \
	$(GDK_PIXBUF_LIBS) \
	$(GIO_LIBS)


install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-tec
CLEANFILES = $(gen_sources)

ligmathumb-enums.c: $(srcdir)/ligmathumb-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <glib-object.h>\n#include \"ligmathumb-enums.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, @valuedesc@, \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    type = g_@type@_register_static (\"@EnumName@\", values);\n\n  return type;\n}\n" \
		$< > xgen-tec \
	&& cp xgen-tec $(@F) \
	&& rm -f xgen-tec
