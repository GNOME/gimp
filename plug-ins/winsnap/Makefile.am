## Process this file with automake to produce Makefile.in

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
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

LDADD = \
	winsnap-win32res.o						\
	$(top_builddir)/libgimp/libgimpui-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimp-$(LT_RELEASE).la		\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(RT_LIBS)							\
	$(GTK_LIBS)							\
	$(INTLLIBS)

winsnap-win32res.o: winsnap.rc select.cur small.ico winsnap.ico
	windres winsnap.rc winsnap-win32res.o
