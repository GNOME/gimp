## Process this file with automake to produce Makefile.in

if OS_WIN32
no_undefined = -no-undefined
conditional_libgimpbase_la = ../libgimpbase/libgimpbase-1.3.la
conditional_libgimpcolor_la = ../libgimpcolor/libgimpcolor-1.3.la
conditional_libgimpwidgets_la = ../libgimpwidgets/libgimpwidgets-1.3.la
endif

libdir = $(gimpplugindir)/modules

INCLUDES = \
        -I$(top_srcdir)		\
        $(GTK_CFLAGS)		\
        -I$(includedir)

EXTRA_DIST = \
	makefile.msc		\
	module.def		\
	colorsel_water.c

lib_LTLIBRARIES = \
	libcolorsel_triangle.la		\
	libcolorsel_water.la		\
	\
	libcdisplay_colorblind.la	\
	libcdisplay_gamma.la		\
	libcdisplay_highcontrast.la

EXTRA_LTLIBRARIES =

libcolorsel_triangle_la_SOURCES = colorsel_triangle.c
libcolorsel_triangle_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_triangle_la_LIBADD = $(conditional_libgimpcolor_la) $(conditional_libgimpwidgets_la) $(GTK_LIBS)

libcolorsel_water_la_SOURCES = colorsel_water.c
libcolorsel_water_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_water_la_LIBADD = $(conditional_libgimpcolor_la) $(conditional_libgimpwidgets_la) $(GTK_LIBS)

libcdisplay_colorblind_la_SOURCES = cdisplay_colorblind.c
libcdisplay_colorblind_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_colorblind_la_LIBADD = $(conditional_libgimpbase_la) $(conditional_libgimpwidgets_la) $(GTK_LIBS)

libcdisplay_gamma_la_SOURCES = cdisplay_gamma.c
libcdisplay_gamma_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_gamma_la_LIBADD = $(conditional_libgimpbase_la) $(conditional_libgimpwidgets_la) $(GTK_LIBS)

libcdisplay_highcontrast_la_SOURCES = cdisplay_highcontrast.c
libcdisplay_highcontrast_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_highcontrast_la_LIBADD = $(conditional_libgimpbase_la) $(conditional_libgimpwidgets_la) $(GTK_LIBS)
