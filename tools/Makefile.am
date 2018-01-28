## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if OS_WIN32

bin_PROGRAMS = \
	       gimptool-2.0 \
	       gimpdebug-2.0 \
	       gimp-debug-resume

gimp_debug_resume_SOURCES = gimp-debug-resume.c

else

bin_PROGRAMS = gimptool-2.0

if PLATFORM_OSX
bin_PROGRAMS += gimpdebug-2.0
else
libexec_PROGRAMS = gimpdebug-2.0
endif

endif

noinst_PROGRAMS = test-clipboard

EXTRA_PROGRAMS = \
	kernelgen

gimpdebug_2_0_SOURCES = gimpdebug.c                         \
			../app/widgets/gimpcriticaldialog.h \
			../app/widgets/gimpcriticaldialog.c \
			../app/gimp-version.h               \
			../app/gimp-version.c

gimpdebug_2_0_CPPFLAGS = \
	-DCC_VERSION=\""$(CC_VERSION)"\" \
	-I$(top_srcdir)/app              \
	-I$(top_builddir)/app            \
	$(AM_CPPFLAGS)                   \
	$(GIO_CFLAGS)	                 \
	$(GEGL_CFLAGS)	                 \
	$(GTK_CFLAGS)                    \
	$(FONTCONFIG_CFLAGS)

gimpdebug_2_0_LDADD =   \
	$(GIO_LIBS)	\
	$(GEGL_LIBS)	\
	$(GTK_LIBS)     \
	$(FONTCONFIG_LIBS)


gimptool_2_0_SOURCES = gimptool.c

gimptool_2_0_LDADD = \
	$(libgimpbase)	\
	$(GTK_LIBS)


kernelgen_SOURCES = kernelgen.c


test_clipboard_SOURCES = test-clipboard.c

test_clipboard_LDADD = $(GTK_LIBS)


if ENABLE_VECTOR_ICONS
invert-svg$(BUILD_EXEEXT): invert-svg.c
	$(CC_FOR_BUILD) -fPIC -o $@ $< $(NATIVE_GLIB_LIBS) $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $(NATIVE_GLIB_CFLAGS)


# compute_svg_viewbox is not built or used because librsvg is just too buggy
# right now. But we keep the code around. The goal of this build tool will be
# to be able to extract SVG icons from a single SVG file at build time, rather
# than having to export and commit them manually.
#compute_svg_viewbox_SOURCES = compute-svg-viewbox.c

#compute_svg_viewbox_CFLAGS = $(SVG_CFLAGS)

#compute_svg_viewbox_LDADD = $(SVG_LIBS)

# Build tools which must be built for the host platform.
all: invert-svg$(BUILD_EXEEXT)

DISTCLEANFILES = invert-svg$(BUILD_EXEEXT)
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
	gimppath2svg.py	\
	invert-svg.c
