## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappfile.a

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-File\"

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

libappfile_a_SOURCES = \
	file-open.c		\
	file-open.h		\
	file-procedure.c	\
	file-procedure.h	\
	file-save.c		\
	file-save.h		\
	file-utils.c		\
	file-utils.h		\
	gimprecentitem.c	\
	gimprecentitem.h	\
	gimprecentlist.c	\
	gimprecentlist.h

EXTRA_DIST = makefile.msc
