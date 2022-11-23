## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

libapp = $(top_builddir)/app/libapp.a
libappwidgets = $(top_builddir)/app/widgets/libappwidgets.a
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_appkit = -framework AppKit
endif

if OS_WIN32
bin_PROGRAMS = ligma-debug-tool-@LIGMA_TOOL_VERSION@
else

if PLATFORM_OSX
bin_PROGRAMS = ligma-debug-tool-@LIGMA_TOOL_VERSION@
else
libexec_PROGRAMS = ligma-debug-tool-@LIGMA_TOOL_VERSION@
endif

endif


ligma_debug_tool_@LIGMA_TOOL_VERSION@_SOURCES = \
	ligma-debug-tool.c

ligma_debug_tool_@LIGMA_TOOL_VERSION@_CPPFLAGS = \
	-DCC_VERSION=\""$(CC_VERSION)"\"	\
	-I$(top_srcdir)/app			\
	-I$(top_builddir)/app			\
	$(AM_CPPFLAGS)				\
	$(GIO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GTK_CFLAGS)				\
	$(FONTCONFIG_CFLAGS)

ligma_debug_tool_@LIGMA_TOOL_VERSION@_LDADD = \
	$(libappwidgets)	\
	$(libapp)		\
	$(libligmabase)		\
	$(GIO_LIBS)		\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(FONTCONFIG_LIBS)


AM_CPPFLAGS = \
	-DLIGMA_APP_VERSION=\"@LIGMA_APP_VERSION@\"		\
	-DLOCALEDIR=\""$(ligmalocaledir)"\"			\
	-DPREFIX=\""$(prefix)"\"				\
	-DEXEC_PREFIX=\""$(exec_prefix)"\"			\
	-DBINDIR=\""$(bindir)"\"				\
	-DSBINDIR=\""$(sbindir)"\"				\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	-DDATADIR=\""$(datadir)"\"				\
	-DDATAROOTDIR=\""$(datarootdir)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"			\
	-DSHAREDSTATEDIR=\""$(sharedstatedir)"\"		\
	-DLOCALSTATEDIR=\""$(localstatedir)"\"			\
	-DLIBDIR=\""$(libdir)"\"				\
	-DINFODIR=\""$(infodir)"\"				\
	-DMANDIR=\""$(mandir)"\"				\
	-DLIGMAPLUGINDIR=\""$(ligmaplugindir)"\"			\
	-DLIGMADATADIR=\""$(ligmadatadir)"\"			\
	-DCC=\""$(CC)"\"					\
	-DLIGMADIR=\""$(ligmadir)"\"				\
	-DLIGMA_PLUGIN_VERSION=\""$(LIGMA_PLUGIN_VERSION)"\"	\
	-I$(top_srcdir)						\
	$(GTK_CFLAGS)						\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(framework_appkit)	\
	$(xnone)
