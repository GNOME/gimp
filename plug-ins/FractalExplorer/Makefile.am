## Process this file with automake to produce Makefile.in

SUBDIRS = fractalexplorer-examples

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = FractalExplorer

FractalExplorer_SOURCES = @STRIP_BEGIN@ \
	Dialogs.h		\
	Dialogs.c		\
	Events.h		\
	Events.c		\
	FractalExplorer.c	\
	FractalExplorer.h	\
	Globals.c		\
	logo.h			\
	pix_data.h		\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)		\
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
