## Process this file with automake to produce Makefile.in

moduledir = $(gimpplugindir)/modules

CPPFLAGS = \
	-DMODULE_COMPILATION

INCLUDES = \
        -I$(top_srcdir)                 \
        -I$(top_srcdir)/intl            \
        $(GTK_CFLAGS)                   \
        -I$(includedir)

EXTRA_DIST = \
	makefile.msc		\
	module.def

module_LTLIBRARIES = \
	libcolorsel_gtk.la 	\
	libcolorsel_triangle.la	\
	libcolorsel_water.la

libcolorsel_gtk_la_SOURCES = colorsel_gtk.c
libcolorsel_gtk_la_LDFLAGS = -avoid-version -module
libcolorsel_gtk_la_LIBADD = @GTK_LIBS@

libcolorsel_triangle_la_SOURCES = colorsel_triangle.c
libcolorsel_triangle_la_LDFLAGS = -avoid-version -module
libcolorsel_triangle_la_LIBADD = @GTK_LIBS@

libcolorsel_water_la_SOURCES = colorsel_water.c
libcolorsel_water_la_LDFLAGS = -avoid-version -module
libcolorsel_water_la_LIBADD = @GTK_LIBS@
