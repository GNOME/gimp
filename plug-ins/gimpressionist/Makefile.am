## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

SUBDIRS = Brushes Paper Presets

EXTRA_DIST = README ChangeLog

libexec_PROGRAMS = gimpressionist

gimpressionist_SOURCES = \
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
	sizemap.c

AM_CPPFLAGS = \
	-DDEFAULTPATH=\""~/$(gimpdir)/gimpressionist:$(gimpdatadir)/gimpressionist"\"

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	$(top_builddir)/libgimp/libgimpui.la	\
	$(top_builddir)/libgimp/libgimp.la	\
	$(GTK_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
