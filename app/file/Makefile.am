## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-File\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GEGL_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappfile.a

libappfile_a_SOURCES = \
	file-import.c	\
	file-import.h	\
	file-open.c	\
	file-open.h	\
	file-remote.c	\
	file-remote.h	\
	file-save.c	\
	file-save.h	\
	file-utils.c	\
	file-utils.h	\
	gimp-file.h
