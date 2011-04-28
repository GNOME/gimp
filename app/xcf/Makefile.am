## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-XCF\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(CAIRO_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappxcf.a

libappxcf_a_SOURCES = \
	xcf.c		\
	xcf.h		\
	xcf-load.c	\
	xcf-load.h	\
	xcf-read.c	\
	xcf-read.h	\
	xcf-private.h	\
	xcf-save.c	\
	xcf-save.h	\
	xcf-seek.c	\
	xcf-seek.h	\
	xcf-write.c	\
	xcf-write.h

EXTRA_DIST = makefile.msc
