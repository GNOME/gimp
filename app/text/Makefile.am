## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Text\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	$(PANGOFT2_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libapptext.a

libapptext_a_SOURCES = \
	text-types.h		\
	gimptext.c		\
	gimptext.h		\
	gimptext-compat.c	\
	gimptext-compat.h	\
	gimptext-render.c	\
	gimptext-render.h	\
	gimptextlayer.c		\
	gimptextlayer.h


