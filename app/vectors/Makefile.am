## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Vectors\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(CAIRO_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappvectors.a

libappvectors_a_SOURCES = \
	vectors-enums.h		\
	vectors-types.h		\
	ligmaanchor.c		\
	ligmaanchor.h		\
	ligmabezierstroke.h	\
	ligmabezierstroke.c	\
	ligmastroke.h		\
	ligmastroke.c		\
	ligmastroke-new.h	\
	ligmastroke-new.c	\
	ligmavectors.c		\
	ligmavectors.h		\
	ligmavectors-compat.c	\
	ligmavectors-compat.h	\
	ligmavectors-export.c	\
	ligmavectors-export.h	\
	ligmavectors-import.c	\
	ligmavectors-import.h	\
	ligmavectors-preview.c	\
	ligmavectors-preview.h	\
	ligmavectors-warp.c	\
	ligmavectors-warp.h	\
	ligmavectorsmodundo.c	\
	ligmavectorsmodundo.h	\
	ligmavectorspropundo.c	\
	ligmavectorspropundo.h	\
	ligmavectorsundo.c	\
	ligmavectorsundo.h
