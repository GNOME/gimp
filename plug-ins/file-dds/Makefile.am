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
libm     = -lm
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
file_dds_RC = file-dds.rc.o
endif

AM_LDFLAGS = $(mwindows)
AM_CFLAGS = -fno-strict-aliasing

libexecdir = $(ligmaplugindir)/plug-ins/file-dds

libexec_PROGRAMS = file-dds

file_dds_SOURCES = \
	dds.c		\
	dds.h		\
	color.c		\
	color.h		\
	ddsread.c	\
	ddsread.h	\
	ddswrite.c	\
	ddswrite.h	\
	dxt.c		\
	dxt.h		\
	dxt_tables.h	\
	endian_rw.h	\
	imath.h		\
	mipmap.c	\
	mipmap.h	\
	misc.c		\
	misc.h		\
	vec.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(libm)			\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_dds_RC)
