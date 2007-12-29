## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgegl.a

libappgegl_a_SOURCES = \
	gimpoptilesink.c	\
	gimpoptilesource.c	\
	gimpops.c

INCLUDES = \
	-I.			\
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GTK_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	-I$(includedir)
