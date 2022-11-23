## Process this file with automake to produce Makefile.in

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
file_webp_RC = file-webp.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-webp

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(EXIF_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GEXIV2_CFLAGS)	\
	$(WEBP_CFLAGS)	\
	$(WEBPMUX_CFLAGS)	\
	$(WEBPDEMUX_CFLAGS)	\
	-I$(includedir)

libexec_PROGRAMS = file-webp

file_webp_SOURCES = \
	file-webp.c		\
	file-webp.h		\
	file-webp-dialog.c		\
	file-webp-dialog.h		\
	file-webp-load.c	\
	file-webp-load.h	\
	file-webp-save.c	\
	file-webp-save.h

file_webp_LDADD = \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(webp_LIBS)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(WEBP_LIBS)		\
	$(WEBPMUX_LIBS)		\
	$(WEBPDEMUX_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_webp_RC)
