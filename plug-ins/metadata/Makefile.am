## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
else
libm = -lm
endif

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
metadata_editor_RC = metadata-editor.rc.o
metadata_viewer_RC = metadata-viewer.rc.o
endif

AM_LDFLAGS = $(mwindows)

me_libexecdir = $(gimpplugindir)/plug-ins/metadata-editor
mv_libexecdir = $(gimpplugindir)/plug-ins/metadata-viewer

me_libexec_PROGRAMS = metadata-editor
mv_libexec_PROGRAMS = metadata-viewer

metadata_editor_SOURCES = \
	metadata-editor.c	\
	metadata-editor.h	\
	metadata-impexp.c	\
	metadata-impexp.h	\
	metadata-misc.h		\
	metadata-tags.h		\
	metadata-xml.c		\
	metadata-xml.h

metadata_viewer_SOURCES = \
	metadata-viewer.c

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

metadata_viewer_LDADD = \
	$(libm)			\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(metadata_viewer_RC)

metadata_editor_LDADD = \
	$(libm)			\
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GEXIV2_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(metadata_editor_RC)

