## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
selection_to_path_RC = selection-to-path.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/selection-to-path

libexec_PROGRAMS = selection-to-path

selection_to_path_SOURCES = \
	bitmap.h			\
	bounding-box.h			\
	curve.c				\
	curve.h				\
	edge.c				\
	edge.h				\
	fit.c				\
	fit.h				\
	global.h			\
	math.c				\
	pxl-outline.c			\
	pxl-outline.h			\
	selection-to-path.c		\
	selection-to-path.h		\
	selection-to-path-dialog.c	\
	spline.c			\
	spline.h			\
	types.h				\
	vector.c			\
	vector.h

EXTRA_DIST = \
	README		\
	README.limn

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligmamath)		\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(selection_to_path_RC)
