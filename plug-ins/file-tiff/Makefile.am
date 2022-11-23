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
file_tiff_RC = file-tiff.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-tiff

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(EXIF_CFLAGS)		\
	$(LCMS_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GEXIV2_CFLAGS)	\
	-I$(includedir)

libexec_PROGRAMS = file-tiff

file_tiff_SOURCES = \
	file-tiff.c		\
	file-tiff.h		\
	file-tiff-io.c		\
	file-tiff-io.h		\
	file-tiff-load.c	\
	file-tiff-load.h	\
	file-tiff-save.c	\
	file-tiff-save.h

file_tiff_LDADD = \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(TIFF_LIBS)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_tiff_RC)
