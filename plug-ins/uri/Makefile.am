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

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = uri

if HAVE_GNOMEVFS
backend_sources = uri-backend-gnomevfs.c
if HAVE_GNOMEUI
backend_cflags = $(GNOMEVFS_CFLAGS) $(GNOMEUI_CFLAGS)
backend_libs = $(GNOMEVFS_LIBS) $(GNOMEUI_LIBS)
else
backend_cflags = $(GNOMEVFS_CFLAGS)
backend_libs = $(GNOMEVFS_LIBS)
endif
else
backend_sources = uri-backend-wget.c
endif

uri_SOURCES = \
	uri.c			\
	uri-backend.h		\
	$(backend_sources)

INCLUDES = \
	-I$(top_srcdir)		\
	$(GTK_CFLAGS)		\
	$(backend_cflags)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimpmath)		\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(GTK_LIBS)		\
	$(backend_libs)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
