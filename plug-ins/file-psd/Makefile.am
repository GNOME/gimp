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
file_psd_load_RC = file-psd-load.rc.o
file_psd_save_RC = file-psd-save.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = \
	file-psd-load	\
	file-psd-save

file_psd_load_SOURCES = \
	psd.c			\
	psd.h			\
	psd-util.c		\
	psd-util.h		\
	psd-load.c		\
	psd-load.h		\
	psd-thumb-load.c	\
	psd-thumb-load.h	\
	psd-image-res-load.c	\
	psd-image-res-load.h	\
	psd-layer-res-load.c	\
	psd-layer-res-load.h		

file_psd_save_SOURCES = \
	psd-save.c

EXTRA_DIST = \
	TODO.txt		\
	new-resource-ids.txt

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(JPEG_LIBS)		\
	$(GTK_LIBS)		\
	$(EXIF_LIBS)		\
	$(IPTCDATA_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)

file_psd_load_LDADD = \
	$(LDADD)		\
	$(file_psd_load_RC) 

file_psd_save_LDADD = \
	$(LDADD)		\
	$(file_psd_save_RC) 
