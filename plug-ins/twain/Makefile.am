## Process this file with automake to produce Makefile.in

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

libexecdir = $(ligmaplugindir)/plug-ins/twain

libexec_PROGRAMS = twain


if OS_WIN32
twain_LDFLAGS = -mwindows -luser32

twainplatform = tw_win.c
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
twain_RC = twain.rc.o
endif


twain_SOURCES = \
	tw_func.c	\
	tw_func.h	\
	tw_util.c	\
	tw_util.h	\
	twain.c		\
	twain.h		\
	tw_local.h	\
	tw_platform.h	\
	$(twainplatform)

EXTRA_DIST = \
	README		\
	ligma-twain.png	\
	tw_dump.c	\
	tw_dump.h	\
	tw_win.c

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libligma)	\
	$(libligmacolor)	\
	$(libligmabase)	\
	$(GLIB_LIBS)	\
	$(GEGL_LIBS)	\
	$(RT_LIBS)	\
	$(INTLLIBS)	\
	$(twain_LIBS)	\
	$(twain_RC)
