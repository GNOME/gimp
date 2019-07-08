## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-File-Data\"	\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_builddir)/app			\
	-I$(top_srcdir)/app			\
	$(APPSTREAM_GLIB_CFLAGS)		\
	$(CAIRO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	$(LIBARCHIVE_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = libappfile-data.a

libappfile_data_a_SOURCES = \
	file-data.c	\
	file-data.h	\
	file-data-gbr.c	\
	file-data-gbr.h	\
	file-data-gex.c	\
	file-data-gex.h	\
	file-data-gih.c	\
	file-data-gih.h	\
	file-data-pat.c	\
	file-data-pat.h
