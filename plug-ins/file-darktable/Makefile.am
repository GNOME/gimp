## Process this file with automake to produce Makefile.in

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

filedarktabledatadir = $(gimpdatadir)/file-darktable

if OS_WIN32
mwindows = -mwindows
endif

# if HAVE_WINDRES
# include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
# file_darktable_RC = file-darktable.rc.o
# endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	-I$(includedir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)

libexec_PROGRAMS = file-darktable

file_darktable_SOURCES = \
	file-darktable.c

file_darktable_LDADD = \
	$(libgimp)		\
	$(libgimpbase)		\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_darktable_RC)

filedarktabledata_DATA = \
	export-on-exit.lua	\
	get-size.lua

EXTRA_DIST = $(filedarktabledata_DATA)
