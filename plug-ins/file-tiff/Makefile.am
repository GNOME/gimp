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
file_tiff_RC = file-tiff.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins/file-tiff

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
	file-tiff-io.c		\
	file-tiff-io.h		\
	file-tiff-load.c	\
	file-tiff-load.h	\
	file-tiff-save.c	\
	file-tiff-save.h

file_tiff_LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(TIFF_LIBS)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_tiff_RC)
