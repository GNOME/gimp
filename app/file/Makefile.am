## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappfile.a

libappfile_a_SOURCES = \
	file-open.c		\
	file-open.h		\
	file-save.c		\
	file-save.h		\
	file-utils.c		\
	file-utils.h		\
	gimprecentitem.c	\
	gimprecentitem.h	\
	gimprecentlist.c	\
	gimprecentlist.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-File\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

EXTRA_DIST = makefile.msc
