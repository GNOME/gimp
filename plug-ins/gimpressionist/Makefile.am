## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

SUBDIRS = Brushes Paper Presets

if OS_WIN32
mwindows = -mwindows
endif

AM_CPPFLAGS = \
	-DDEFAULTPATH=\""~/$(gimpdir)/gimpressionist:$(gimpdatadir)/gimpressionist"\"

AM_LDFLAGS = $(mwindows)

INCLUDES = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gimpressionist

gimpressionist_sources = \
	brush.c			\
	color.h			\
	color.c			\
	general.h		\
	general.c		\
	gimp.c			\
	gimpressionist.c	\
	gimpressionist.h	\
	globals.c		\
	orientation.c		\
	orientmap.c		\
	paper.c			\
	placement.h		\
	placement.c		\
	plasma.c		\
	ppmtool.c		\
	ppmtool.h		\
	presets.c		\
	preview.h		\
	preview.c		\
	repaint.c		\
	size.c			\
	sizemap.c		\
	utils.c

gimpressionist_built_sources = logo-pixbuf.h
gimpressionist.c: logo-pixbuf.h

gimpressionist_SOURCES = \
	$(gimppessionist_built_sources) $(gimpressionist_sources)


LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)


$(srcdir)/logo-pixbuf.h: logo.png
	gdk-pixbuf-csource --raw --name=logo_data $< > $(@F)

EXTRA_DIST = \
	README	\
	logo.png

CLEANFILES = logo-pixbuf.h
