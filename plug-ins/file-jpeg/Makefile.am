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
file_jpeg_RC = file-jpeg.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-jpeg

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(EXIF_CFLAGS)		\
	$(LCMS_CFLAGS)		\
	$(GEGL_CFLAGS)		\
	$(GEXIV2_CFLAGS)	\
	-I$(includedir)

libexec_PROGRAMS = file-jpeg

file_jpeg_SOURCES = \
	jpeg.c		\
	jpeg.h		\
	jpeg-icc.c	\
	jpeg-icc.h	\
	jpeg-load.c	\
	jpeg-load.h	\
	jpeg-save.c	\
	jpeg-save.h	\
	jpeg-quality.c  \
	jpeg-quality.h  \
	jpeg-settings.c \
	jpeg-settings.h

file_jpeg_LDADD = \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(JPEG_LIBS)		\
	$(LCMS_LIBS)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_jpeg_RC)

noinst_PROGRAMS = jpegqual

jpegqual_SOURCES = \
	jpeg-quality.c  \
	jpeg-quality.h  \
	jpegqual.c

jpegqual_LDADD = \
	$(JPEG_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)

EXTRA_DIST = \
	COPYING.ijg
