## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la

if OS_WIN32
no_undefined = -no-undefined
endif

libdir = $(gimpplugindir)/modules

INCLUDES = \
        -I$(top_srcdir)	\
        $(GTK_CFLAGS)	\
        -I$(includedir)

EXTRA_DIST = \
	makefile.msc

if HAVE_LCMS
cdisplay_proof_module = libcdisplay_proof.la
endif

lib_LTLIBRARIES = \
	libcolorsel_cmyk.la		\
	libcolorsel_triangle.la		\
	libcolorsel_water.la		\
	libcdisplay_colorblind.la	\
	libcdisplay_gamma.la		\
	libcdisplay_highcontrast.la	\
	$(cdisplay_proof_module)


libcolorsel_cmyk_la_SOURCES = colorsel_cmyk.c
libcolorsel_cmyk_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_cmyk_la_LIBADD = $(libgimpcolor) $(libgimpwidgets) $(GTK_LIBS)

libcolorsel_triangle_la_SOURCES = colorsel_triangle.c
libcolorsel_triangle_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_triangle_la_LIBADD = $(libgimpcolor) $(libgimpwidgets) $(GTK_LIBS)

libcolorsel_water_la_SOURCES = colorsel_water.c
libcolorsel_water_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_water_la_LIBADD = $(libgimpcolor) $(libgimpwidgets) $(GTK_LIBS)

libcdisplay_colorblind_la_SOURCES = cdisplay_colorblind.c
libcdisplay_colorblind_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_colorblind_la_LIBADD = $(libgimpbase) $(libgimpwidgets) $(GTK_LIBS)

libcdisplay_gamma_la_SOURCES = cdisplay_gamma.c
libcdisplay_gamma_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_gamma_la_LIBADD = $(libgimpbase) $(libgimpwidgets) $(GTK_LIBS)

libcdisplay_highcontrast_la_SOURCES = cdisplay_highcontrast.c
libcdisplay_highcontrast_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_highcontrast_la_LIBADD = $(libgimpbase) $(libgimpwidgets) $(GTK_LIBS)

libcdisplay_proof_la_SOURCES = cdisplay_proof.c
libcdisplay_proof_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_proof_la_LIBADD = $(libgimpbase) $(libgimpwidgets) $(GTK_LIBS) $(LCMS_LIBS)
