## Process this file with automake to produce Makefile.in

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
	color.c			\
	general.c		\
	gimp.c			\
	gimpressionist.c	\
	gimpressionist.h	\
	orientation.c		\
	orientmap.c		\
	paper.c			\
	placement.c		\
	plasma.c		\
	ppmtool.c		\
	ppmtool.h		\
	presets.c		\
	preview.c		\
	repaint.c		\
	size.c			\
	sizemap.c

gimpressionist_built_sources = logo-pixbuf.h
gimpressionist.c: logo-pixbuf.h

gimpressionist_SOURCES = \
	$(gimppessionist_built_sources) $(gimpressionist_sources)


LDADD = \
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GTK_LIBS)							\
	$(RT_LIBS)							\
	$(INTLLIBS)


$(srcdir)/logo-pixbuf.h: logo.png
	gdk-pixbuf-csource --raw --name=logo_data $< > $(@F)

EXTRA_DIST = \
	README	\
	logo.png

CLEANFILES = logo-pixbuf.h
