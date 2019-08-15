## Process this file with automake to produce Makefile.in

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

# if HAVE_WINDRES
# include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
# file_exr_RC = file-exr.rc.o
# endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins/file-exr

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
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(LCMS_LIBS)		\
	$(file_exr_RC)
