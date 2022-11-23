## Process this file with automake to produce Makefile.in

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
file_faxg3_RC = file-faxg3.rc.o
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-faxg3

libexec_PROGRAMS = file-faxg3

file_faxg3_SOURCES = \
	faxg3.c		\
	g3.c		\
	g3.h

AM_CPPFLAGS = \
	-I$(top_srcdir)		\
	$(CAIRO_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	$(GEGL_CFLAGS) \
	-I$(includedir)

LDADD = \
	$(libligma)		\
	$(libligmaconfig)	\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(libligmamath)		\
	$(CAIRO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_faxg3_RC)
