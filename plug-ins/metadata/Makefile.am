## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libligmaui = $(top_builddir)/libligma/libligmaui-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la
libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmamath = $(top_builddir)/libligmamath/libligmamath-$(LIGMA_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
metadata_editor_RC = metadata-editor.rc.o
metadata_viewer_RC = metadata-viewer.rc.o
endif

AM_LDFLAGS = $(mwindows)

me_libexecdir = $(ligmaplugindir)/plug-ins/metadata-editor
mv_libexecdir = $(ligmaplugindir)/plug-ins/metadata-viewer

me_libexec_PROGRAMS = metadata-editor
mv_libexec_PROGRAMS = metadata-viewer

metadata_editor_SOURCES = \
	metadata-editor.c	\
	metadata-editor.h	\
	metadata-impexp.c	\
	metadata-impexp.h	\
	metadata-misc.h		\
	metadata-tags.c		\
	metadata-tags.h		\
	metadata-xml.c		\
	metadata-xml.h

metadata_viewer_SOURCES = \
	metadata-viewer.c	\
	metadata-tags.c		\
	metadata-tags.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

metadata_viewer_LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(metadata_viewer_RC)

metadata_editor_LDADD = \
	$(libm)			\
	$(libligmaui)		\
	$(libligmawidgets)	\
	$(libligmaconfig)	\
	$(libligma)		\
	$(libligmacolor)		\
	$(libligmamath)		\
	$(libligmabase)		\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(metadata_editor_RC)

