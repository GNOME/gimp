## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la

if OS_WIN32
no_undefined = -no-undefined
endif

libdir = $(gimpplugindir)/modules

INCLUDES = \
        -I$(top_srcdir)		\
        $(GTK_CFLAGS)		\
        -I$(includedir)

EXTRA_DIST = \
	makefile.msc

lib_LTLIBRARIES = \
	libcolorsel_triangle.la		\
	libcolorsel_water.la		\
	libcdisplay_colorblind.la	\
	libcdisplay_gamma.la		\
	libcdisplay_highcontrast.la

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
