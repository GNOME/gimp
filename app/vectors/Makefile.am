## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Vectors\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	$(LIBART_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappvectors.a

libappvectors_a_SOURCES = \
	vectors-types.h		\
	gimpanchor.c		\
	gimpanchor.h		\
	gimpbezierstroke.h	\
	gimpbezierstroke.c	\
	gimpstroke.h		\
	gimpstroke.c		\
	gimpvectors.c		\
	gimpvectors.h		\
	gimpvectors-compat.c	\
	gimpvectors-compat.h	\
	gimpvectors-export.c	\
	gimpvectors-export.h	\
	gimpvectors-import.c	\
	gimpvectors-import.h	\
	gimpvectors-preview.c	\
	gimpvectors-preview.h

EXTRA_DIST = makefile.msc
