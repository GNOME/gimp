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

# if HAVE_WINDRES
# include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
# file_exr_RC = file-exr.rc.o
# endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(ligmaplugindir)/plug-ins/file-exr

AM_CPPFLAGS = \
	-I$(top_srcdir)	  \
	$(GTK_CFLAGS)	  \
	$(GEGL_CFLAGS)    \
	$(OPENEXR_CFLAGS) \
	$(LCMS_CFLAGS) \
	-I$(includedir)

libexec_PROGRAMS = file-exr

file_exr_SOURCES = \
	exr-attribute-blob.h	\
	file-exr.c		\
	openexr-wrapper.cc	\
	openexr-wrapper.h

file_exr_LDADD = \
	$(OPENEXR_LIBS)		\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(LCMS_LIBS)		\
	$(file_exr_RC)
