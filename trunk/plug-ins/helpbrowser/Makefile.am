## Process this file with automake to produce Makefile.in

libgimphelp = $(top_builddir)/plug-ins/help/libgimphelp.a
libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = helpbrowser

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTKHTML2_CFLAGS)	\
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
	$(GTKHTML2_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)

helpbrowser_sources = \
	gimpthrobber.c		\
	gimpthrobber.h		\
	gimpthrobberaction.c	\
	gimpthrobberaction.h	\
	helpbrowser.c		\
	dialog.c		\
	dialog.h		\
	queue.c			\
	queue.h			\
	uri.c			\
	uri.h

helpbrowser_built_sources = logo-pixbuf.h
dialog.c: $(helpbrowser_built_sources)

helpbrowser_SOURCES = \
	$(helpbrowser_built_sources) $(helpbrowser_sources)


$(srcdir)/logo-pixbuf.h: wilber-reading.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=logo_data $< > $(@F)

EXTRA_DIST = wilber-reading.png

CLEANFILES = logo-pixbuf.h
