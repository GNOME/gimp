## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = fractalexplorer-examples

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = FractalExplorer

FractalExplorer_SOURCES = \
	Dialogs.h		\
	Dialogs.c		\
	Events.h		\
	Events.c		\
	FractalExplorer.c	\
	FractalExplorer.h	\
	Globals.c		\
	logo.h			\
	pix_data.h

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
