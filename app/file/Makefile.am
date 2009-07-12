## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-File\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GEGL_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappfile.a

libappfile_a_SOURCES = \
	file-open.c		\
	file-open.h		\
	file-procedure.c	\
	file-procedure.h	\
	file-save.c		\
	file-save.h		\
	file-utils.c		\
	file-utils.h		\
	gimp-file.h

EXTRA_DIST = makefile.msc
