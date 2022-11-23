## Process this file with automake to produce Makefile.in

libligmahelp = libligmahelp.a
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
help_RC = help.rc.o
endif

AM_CPPFLAGS = \
	-DDATADIR=\""$(ligmadatadir)"\"	\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GIO_CFLAGS)			\
	$(GEGL_CFLAGS)			\
	$(GDK_PIXBUF_CFLAGS)		\
	-I$(includedir)

AM_LDFLAGS = $(mwindows)

noinst_LIBRARIES = libligmahelp.a

libligmahelp_a_SOURCES = \
	ligmahelptypes.h		\
	ligmahelp.c		\
	ligmahelp.h		\
	ligmahelpdomain.c	\
	ligmahelpdomain.h	\
	ligmahelpitem.c		\
	ligmahelpitem.h		\
	ligmahelplocale.c	\
	ligmahelplocale.h	\
	ligmahelpprogress.c	\
	ligmahelpprogress.h	\
	ligmahelpprogress-private.h

libexecdir = $(ligmaplugindir)/plug-ins/help

libexec_PROGRAMS = help

help_SOURCES = help.c

LDADD = \
	$(libligmahelp)		\
	$(libligma)		\
	$(libligmaconfig)	\
	$(libligmacolor)		\
	$(libligmabase)		\
	$(libligmamath)		\
	$(CAIRO_LIBS)		\
	$(GIO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(help_RC)


noinst_PROGRAMS = ligma-help-lookup

ligma_help_lookup_SOURCES = ligma-help-lookup.c

ligma_help_lookup_LDADD = \
	$(libligmahelp)	\
	$(libligmabase)	\
	$(GIO_LIBS)
