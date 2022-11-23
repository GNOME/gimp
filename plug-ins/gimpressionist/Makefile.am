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

SUBDIRS = Brushes Paper Presets

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
ligmaressionist_RC = ligmaressionist.rc.o
endif

AM_CPPFLAGS = \
	-DDEFAULTPATH=\""~/$(ligmadir)/ligmaressionist:$(ligmadatadir)/ligmaressionist"\"	\
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

AM_LDFLAGS = $(mwindows)


libexecdir = $(ligmaplugindir)/plug-ins/ligmaressionist

libexec_PROGRAMS = ligmaressionist

ligmaressionist_SOURCES = \
	brush.c			\
	brush.h			\
	color.c			\
	color.h			\
	general.c		\
	general.h		\
	ligma.c			\
	ligmaressionist.c	\
	ligmaressionist.h	\
	globals.c		\
	infile.h        	\
	orientation.h		\
	orientation.c		\
	orientmap.h     \
	orientmap.c		\
	paper.c			\
	paper.h			\
	placement.c		\
	placement.h		\
	plasma.c		\
	ppmtool.c		\
	ppmtool.h		\
	presets.c		\
	presets.h		\
	preview.c		\
	preview.h		\
	random.h        	\
	repaint.c		\
	size.h          	\
	size.c			\
	sizemap.c		\
	utils.c

LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(ligmaressionist_RC)

EXTRA_DIST = README
