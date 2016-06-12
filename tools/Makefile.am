## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if ENABLE_VECTOR_ICONS
svg_icon_tools = compute-svg-viewbox
else
svg_icon_tools =
endif

if WITH_PDBGEN
D_pdbgen = pdbgen
else
D_pdbgen =
endif

SUBDIRS = $(D_pdbgen)

if OS_WIN32

bin_PROGRAMS = \
	gimptool-2.0 \
	gimp-debug-resume

gimp_debug_resume_SOURCES = gimp-debug-resume.c

else

bin_PROGRAMS = gimptool-2.0

endif

noinst_PROGRAMS = test-clipboard $(svg_icon_tools)

EXTRA_PROGRAMS = \
	kernelgen

gimptool_2_0_SOURCES = \
	gimptool.c

gimptool_2_0_LDADD = \
	$(libgimpbase)			\
	$(GTK_LIBS)

kernelgen_SOURCES = kernelgen.c

test_clipboard_SOURCES = test-clipboard.c

test_clipboard_LDADD = \
	$(GTK_LIBS)

if ENABLE_VECTOR_ICONS
invert-svg$(BUILD_EXEEXT): invert-svg.c
	$(CC_FOR_BUILD) -o $@ $< $(NATIVE_GLIB_LIBS) $(NATIVE_GLIB_CFLAGS)

compute_svg_viewbox_SOURCES = compute-svg-viewbox.c

compute_svg_viewbox_CFLAGS = \
	$(SVG_CFLAGS)

compute_svg_viewbox_LDADD = \
	$(SVG_LIBS)

all: invert-svg$(BUILD_EXEEXT)
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

EXTRA_DIST = \
	defcheck.py	\
	gimp-mkenums	\
	gimppath2svg.py
