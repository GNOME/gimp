## Process this file with automake to produce Makefile.in

SUBDIRS = gfig-examples images

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = gfig

gfig_SOURCES = \
	gfig.c		\
	gfig-stock.c	\
	gfig-stock.h	\
	pix_data.h

EXTRA_DIST = README

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
