## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = Brushes Paper Presets

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gimpressionist

gimpressionist_SOURCES =\
	brush.c			\
	color.c			\
	general.c		\
	gimp.c			\
	gimpressionist.c	\
	gimpressionist.h	\
	logo-pixbuf.h		\
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

$(srcdir)/logo-pixbuf.h: logo.png
	gdk-pixbuf-csource --raw --name=logo_data $< > $(@F)

EXTRA_DIST = \
	README	\
	logo.png

CLEANFILES = logo-pixbuf.h


AM_CPPFLAGS = \
	-DDEFAULTPATH=\""~/$(gimpdir)/gimpressionist:$(gimpdatadir)/gimpressionist"\"


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
	$(INTLLIBS)
