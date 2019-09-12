## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

bin_PROGRAMS = \
	gimptool-@GIMP_TOOL_VERSION@	\
	gimp-test-clipboard-@GIMP_TOOL_VERSION@

if OS_WIN32

bin_PROGRAMS += gimp-debug-resume

gimp_debug_resume_SOURCES = gimp-debug-resume.c

endif

EXTRA_PROGRAMS = \
	kernelgen


gimptool_@GIMP_TOOL_VERSION@_SOURCES = gimptool.c

gimptool_@GIMP_TOOL_VERSION@_LDADD = \
	$(libgimpbase)	\
	$(GTK_LIBS)


gimp_test_clipboard_@GIMP_TOOL_VERSION@_SOURCES = gimp-test-clipboard.c

gimp_test_clipboard_@GIMP_TOOL_VERSION@_LDADD = \
	$(libgimpbase)	\
	$(GTK_LIBS)


kernelgen_SOURCES = kernelgen.c



if ENABLE_VECTOR_ICONS
# compute_svg_viewbox is not built or used because librsvg is just too buggy
# right now. But we keep the code around. The goal of this build tool will be
# to be able to extract SVG icons from a single SVG file at build time, rather
# than having to export and commit them manually.
#compute_svg_viewbox_SOURCES = compute-svg-viewbox.c

#compute_svg_viewbox_CFLAGS = $(SVG_CFLAGS)

#compute_svg_viewbox_LDADD = $(SVG_LIBS)

# Build tools which must be built for the host platform.
#all-local: compute-svg-viewbox$(BUILD_EXEEXT)

#DISTCLEANFILES = compute-svg-viewbox$(BUILD_EXEEXT)
endif

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
	$(xnone)

EXTRA_DIST = \
	defcheck.py			\
	gimp-mkenums			\
	gimppath2svg.py			\
	mnemonic-clashes		\
	performance-log-coalesce.py	\
	performance-log-deduce.py	\
	performance-log-expand.py	\
	performance-log-resolve.py	\
	performance-log-viewer		\
	performance-log-viewer.py
