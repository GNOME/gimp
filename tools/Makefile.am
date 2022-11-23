## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

bin_PROGRAMS = \
	ligmatool-@LIGMA_TOOL_VERSION@	\
	ligma-test-clipboard-@LIGMA_TOOL_VERSION@

if OS_WIN32

bin_PROGRAMS += ligma-debug-resume

ligma_debug_resume_SOURCES = ligma-debug-resume.c

endif

EXTRA_PROGRAMS = \
	kernelgen


ligmatool_@LIGMA_TOOL_VERSION@_SOURCES = ligmatool.c

ligmatool_@LIGMA_TOOL_VERSION@_LDADD = \
	$(libligmabase)	\
	$(GTK_LIBS)


ligma_test_clipboard_@LIGMA_TOOL_VERSION@_SOURCES = ligma-test-clipboard.c

ligma_test_clipboard_@LIGMA_TOOL_VERSION@_LDADD = \
	$(libligmabase)	\
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

#noinst_PROGRAMS = compute-svg-viewbox

#DISTCLEANFILES = compute-svg-viewbox$(BUILD_EXEEXT)
endif

# Build tools which must be built for the build platform.
colorsvg2png$(BUILD_EXEEXT): colorsvg2png.c
	$(CC_FOR_BUILD) -o $@ $< $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) \
			         $(NATIVE_GLIB_LIBS) $(NATIVE_GLIB_CFLAGS) $(NATIVE_SVG_LIBS) $(NATIVE_SVG_CFLAGS)

all-local: colorsvg2png$(BUILD_EXEEXT)

DISTCLEANFILES = colorsvg2png$(BUILD_EXEEXT)

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
	$(xnone)

EXTRA_DIST = \
	colorsvg2png.c				\
	defcheck.py				\
	generate-welcome-dialog-data.py		\
	ligma-mkenums				\
	ligmapath2svg.py				\
	mnemonic-clashes			\
	performance-log-close-tags.py		\
	performance-log-coalesce.py		\
	performance-log-deduce.py		\
	performance-log-expand.py		\
	performance-log-progressive-coalesce.py	\
	performance-log-resolve.py		\
	performance-log-viewer			\
	performance-log-viewer.py
