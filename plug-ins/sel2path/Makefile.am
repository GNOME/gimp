## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = sel2path

sel2path_SOURCES = @STRIP_BEGIN@ \
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
	vector.h		\
@STRIP_END@

EXTRA_DIST = \
	README		\
	README.limn

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DLOCALEDIR=\""$(localedir)"\"	\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

LDADD = @STRIP_BEGIN@ \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(INTLLIBS)							\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
