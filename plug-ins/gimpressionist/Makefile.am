## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

SUBDIRS = Brushes Paper Presets

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
gimpressionist_RC = gimpressionist.rc.o
endif

AM_CPPFLAGS = \
	-DDEFAULTPATH=\""~/$(gimpdir)/gimpressionist:$(gimpdatadir)/gimpressionist"\"	\
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

AM_LDFLAGS = $(mwindows)


libexecdir = $(gimpplugindir)/plug-ins/gimpressionist

libexec_PROGRAMS = gimpressionist

gimpressionist_SOURCES = \
	brush.c			\
	brush.h			\
	color.c			\
	color.h			\
	general.c		\
	general.h		\
	gimp.c			\
	gimpressionist.c	\
	gimpressionist.h	\
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
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(gimpressionist_RC)

EXTRA_DIST = README
