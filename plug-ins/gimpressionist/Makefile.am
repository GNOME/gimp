## Process this file with automake to produce Makefile.in

SUBDIRS = Brushes Paper Presets

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gimpressionist

gimpressionist_SOURCES = @STRIP_BEGIN@ \
	about.c			\
	brush.c			\
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
	sizemap.c		\
	color.c			\
@STRIP_END@

EXTRA_DIST = \
	README		\
	ChangeLog

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DDEFAULTPATH=\""~/$(gimpdir)/gimpressionist:$(gimpdatadir)/gimpressionist"\" \
        -DLOCALEDIR=\""$(localedir)"\"	\
@STRIP_END@


INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
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
