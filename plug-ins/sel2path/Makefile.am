## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

LDFLAGS = @LDFLAGS@ $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

EXTRA_DIST = README \
	README.limn

libexec_PROGRAMS = sel2path

sel2path_SOURCES = \
	bitmap.h	\
	bounding-box.h	\
	curve.c		\
	curve.h		\
	edge.c		\
	edge.h		\
	fit.c		\
	fit.h		\
	global.h	\
	math.c		\
	pxl-outline.c	\
	pxl-outline.h	\
	sel2path.c	\
	sel2path_adv_dialog.c	\
	sel2path.h	\
	spline.c	\
	spline.h	\
	types.h		\
	vector.c	\
	vector.h

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(localedir)"\"

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la		\
	$(top_builddir)/libgimp/libgimp.la		\
	$(GTK_LIBS)					\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
