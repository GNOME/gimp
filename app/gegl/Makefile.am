## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgegl.a

libappgegl_a_SOURCES = \
	gimp-gegl.c			\
	gimp-gegl.h			\
	gimp-gegl-utils.c		\
	gimp-gegl-utils.h		\
	gimpoperationdesaturate.c	\
	gimpoperationdesaturate.h	\
	gimpoperationtilesink.c		\
	gimpoperationtilesink.h		\
	gimpoperationtilesource.c	\
	gimpoperationtilesource.h

INCLUDES = \
	-I.			\
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	-I$(includedir)
