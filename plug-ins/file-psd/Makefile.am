## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
file_psd_RC = file-psd.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins/file-psd

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
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(JPEG_LIBS)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(EXIF_LIBS)		\
	$(IPTCDATA_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(Z_LIBS)		\
	$(file_psd_RC)
