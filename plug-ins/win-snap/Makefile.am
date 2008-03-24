## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

winsnap_LDFLAGS = -mwindows

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = winsnap

winsnap_SOURCES = \
	winsnap.c	\
	winsnap.h

EXTRA_DIST = \
	resource.h	\
	select.cur	\
	small.ico	\
	winsnap.ico	\
	winsnap.rc

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	-I$(includedir)

LDADD = \
	winsnap-win32res.o	\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpmath)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(RT_LIBS)		\
	$(GTK_LIBS)		\
	$(INTLLIBS)

winsnap-win32res.o: winsnap.rc select.cur small.ico winsnap.ico
	$(WINDRES) winsnap.rc winsnap-win32res.o
