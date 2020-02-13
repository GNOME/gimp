## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

libapp = $(top_builddir)/app/libapp.a
libappwidgets = $(top_builddir)/app/widgets/libappwidgets.a
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
framework_appkit = -framework AppKit
endif

if OS_WIN32
bin_PROGRAMS = gimp-debug-tool-@GIMP_TOOL_VERSION@
else

if PLATFORM_OSX
bin_PROGRAMS = gimp-debug-tool-@GIMP_TOOL_VERSION@
else
libexec_PROGRAMS = gimp-debug-tool-@GIMP_TOOL_VERSION@
endif

endif


gimp_debug_tool_@GIMP_TOOL_VERSION@_SOURCES = \
	gimp-debug-tool.c

gimp_debug_tool_@GIMP_TOOL_VERSION@_CPPFLAGS = \
	-DCC_VERSION=\""$(CC_VERSION)"\"	\
	-I$(top_srcdir)/app			\
	-I$(top_builddir)/app			\
	$(AM_CPPFLAGS)				\
	$(GIO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GTK_CFLAGS)				\
	$(FONTCONFIG_CFLAGS)

gimp_debug_tool_@GIMP_TOOL_VERSION@_LDADD = \
	$(libappwidgets)	\
	$(libapp)		\
	$(libgimpbase)						\
	$(GIO_LIBS)		\
	$(GEGL_LIBS)		\
	$(GTK_LIBS)		\
	$(FONTCONFIG_LIBS)


AM_CPPFLAGS = \
	-DGIMP_APP_VERSION=\"@GIMP_APP_VERSION@\"		\
	-DLOCALEDIR=\""$(gimplocaledir)"\"			\
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
	-DGIMPPLUGINDIR=\""$(gimpplugindir)"\"			\
	-DGIMPDATADIR=\""$(gimpdatadir)"\"			\
	-DCC=\""$(CC)"\"					\
	-DGIMPDIR=\""$(gimpdir)"\"				\
	-DGIMP_PLUGIN_VERSION=\""$(GIMP_PLUGIN_VERSION)"\"	\
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
