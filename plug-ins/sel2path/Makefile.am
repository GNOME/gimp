## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = sel2path

sel2path_SOURCES = \
	bitmap.h		\
	bounding-box.h		\
	curve.c			\
	curve.h			\
	edge.c			\
	edge.h			\
	fit.c			\
	fit.h			\
	global.h		\
	math.c			\
	pxl-outline.c		\
	pxl-outline.h		\
	sel2path.c		\
	sel2path_adv_dialog.c	\
	sel2path.h		\
	spline.c		\
	spline.h		\
	types.h			\
	vector.c		\
	vector.h

EXTRA_DIST = \
	README		\
	README.limn

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(RT_LIBS)							\
	$(INTLLIBS)
