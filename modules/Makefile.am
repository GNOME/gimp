## Process this file with automake to produce Makefile.in

if OS_WIN32
no_undefined = -no-undefined
gimp_exe_export_lib = -L../app -lgimpapp
endif

libexecdir = $(gimpplugindir)/modules

CPPFLAGS = \
	-DMODULE_COMPILATION

INCLUDES = \
        -I$(top_srcdir)		\
        -I$(top_srcdir)/intl	\
        $(GTK_CFLAGS)		\
        -I$(includedir)

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	module.def		\
	gimpmodregister.c	\
	gimpmodregister.h

libexec_LTLIBRARIES = \
	libcolorsel_gtk.la		\
	libcolorsel_triangle.la		\
	libcolorsel_water.la

libcolorsel_gtk_la_SOURCES = colorsel_gtk.c
libcolorsel_gtk_la_LDFLAGS = -avoid-version -module $(no_undefined) $(gimp_exe_export_lib)
libcolorsel_gtk_la_LIBADD = $(GTK_LIBS) 

libcolorsel_triangle_la_SOURCES = colorsel_triangle.c
libcolorsel_triangle_la_LDFLAGS = -avoid-version -module  $(no_undefined) $(gimp_exe_export_lib)
libcolorsel_triangle_la_LIBADD = $(GTK_LIBS)

libcolorsel_water_la_SOURCES = colorsel_water.c
libcolorsel_water_la_LDFLAGS = -avoid-version -module  $(no_undefined) $(gimp_exe_export_lib)
libcolorsel_water_la_LIBADD = $(GTK_LIBS)

## libcdisplay_gamma_la_SOURCES = cdisplay_gamma.c
## libcdisplay_gamma_la_LDFLAGS = -avoid-version -module
## libcdisplay_gamma_la_LIBADD = $(GTK_LIBS)

## libcdisplay_highcontrast_la_SOURCES = cdisplay_highcontrast.c
## libcdisplay_highcontrast_la_LDFLAGS = -avoid-version -module
## libcdisplay_highcontrast_la_LIBADD = $(GTK_LIBS)
