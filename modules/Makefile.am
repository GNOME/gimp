## Process this file with automake to produce Makefile.in

moduledir = $(gimpplugindir)/modules

INCLUDES = \
        -I$(top_srcdir)                 \
        -I$(top_srcdir)/intl            \
        $(GTK_CFLAGS)                   \
        -I$(includedir)

module_LTLIBRARIES = libcolorsel_gtk.la libcolorsel_triangle.la

libcolorsel_gtk_la_SOURCES = colorsel_gtk.c

libcolorsel_triangle_la_SOURCES = colorsel_triangle.c

