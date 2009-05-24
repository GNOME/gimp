## Process this file with automake to produce Makefile.in

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la
libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la

libgimphelp = $(top_builddir)/plug-ins/help/libgimphelp.a

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = help-browser

INCLUDES = \
	-I$(top_srcdir)		\
	$(WEBKIT_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimphelp)		\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(WEBKIT_LIBS)		\
	$(GIO_LIBS)		\
	$(GLIB_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)

help_browser_SOURCES = \
	gimpthrobber.c		\
	gimpthrobber.h		\
	gimpthrobberaction.c	\
	gimpthrobberaction.h	\
	help-browser.c		\
	dialog.c		\
	dialog.h		\
	uri.c			\
	uri.h
