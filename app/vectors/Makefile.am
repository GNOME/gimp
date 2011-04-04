## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Vectors\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(CAIRO_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappvectors.a

libappvectors_a_SOURCES = \
	vectors-enums.h		\
	vectors-types.h		\
	gimpanchor.c		\
	gimpanchor.h		\
	gimpbezierstroke.h	\
	gimpbezierstroke.c	\
	gimpstroke.h		\
	gimpstroke.c		\
	gimpstroke-new.h	\
	gimpstroke-new.c	\
	gimpvectors.c		\
	gimpvectors.h		\
	gimpvectors-compat.c	\
	gimpvectors-compat.h	\
	gimpvectors-export.c	\
	gimpvectors-export.h	\
	gimpvectors-import.c	\
	gimpvectors-import.h	\
	gimpvectors-preview.c	\
	gimpvectors-preview.h	\
	gimpvectors-warp.c	\
	gimpvectors-warp.h	\
	gimpvectorsmodundo.c	\
	gimpvectorsmodundo.h	\
	gimpvectorspropundo.c	\
	gimpvectorspropundo.h	\
	gimpvectorsundo.c	\
	gimpvectorsundo.h

EXTRA_DIST = makefile.msc
