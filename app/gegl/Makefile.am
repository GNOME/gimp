## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappgegl.a

libappgegl_a_SOURCES = \
	gimp-gegl.c			\
	gimp-gegl.h			\
	gimp-gegl-utils.c		\
	gimp-gegl-utils.h		\
	\
	gimpcolorizeconfig.c		\
	gimpcolorizeconfig.h		\
	gimphuesaturationconfig.c	\
	gimphuesaturationconfig.h	\
	gimplevelsconfig.c		\
	gimplevelsconfig.h		\
	\
	gimpoperationcolorbalance.c	\
	gimpoperationcolorbalance.h	\
	gimpoperationcolorize.c		\
	gimpoperationcolorize.h		\
	gimpoperationdesaturate.c	\
	gimpoperationdesaturate.h	\
	gimpoperationhuesaturation.c	\
	gimpoperationhuesaturation.h	\
	gimpoperationlevels.c		\
	gimpoperationlevels.h		\
	gimpoperationposterize.c	\
	gimpoperationposterize.h	\
	gimpoperationthreshold.c	\
	gimpoperationthreshold.h	\
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
	$(GEGL_CFLAGS)		\
	-I$(includedir)
