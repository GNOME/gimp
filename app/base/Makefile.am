## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Base\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(BABL_CFLAGS)		\
	$(CAIRO_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappbase.a

libappbase_a_SOURCES = \
	base.c			\
	base.h			\
	base-types.h		\
	pixel-region.c		\
	pixel-region.h		\
	tile.c			\
	tile.h			\
	tile-private.h		\
	tile-cache.c		\
	tile-cache.h		\
	tile-manager.c		\
	tile-manager.h		\
	tile-manager-preview.c	\
	tile-manager-preview.h	\
	tile-manager-private.h	\
	tile-pyramid.c		\
	tile-pyramid.h		\
	tile-rowhints.c		\
	tile-rowhints.h		\
	tile-swap.c		\
	tile-swap.h
