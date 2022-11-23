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
else
libm = -lm
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
file_psd_RC = file-psd.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-psd

libexec_PROGRAMS = \
	file-psd

file_psd_SOURCES = \
	psd.c			\
	psd.h			\
	psd-util.c		\
	psd-util.h		\
	psd-load.c		\
	psd-load.h		\
	psd-save.c		\
	psd-save.h		\
	psd-thumb-load.c	\
	psd-thumb-load.h	\
	psd-image-res-load.c	\
	psd-image-res-load.h	\
	psd-layer-res-load.c	\
	psd-layer-res-load.h

EXTRA_DIST = \
	TODO.txt		\
	new-resource-ids.txt

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"file-psd\"	\
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	$(EXIF_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	-I$(includedir)

file_psd_LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(JPEG_LIBS)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(EXIF_LIBS)		\
	$(IPTCDATA_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(Z_LIBS)		\
	$(file_psd_RC)
