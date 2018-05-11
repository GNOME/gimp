## Process this file with automake to produce Makefile.in

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la

filerawdatadir = $(gimpdatadir)/file-raw

if OS_WIN32
mwindows = -mwindows
endif

# if HAVE_WINDRES
# include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
# file_darktable_RC = file-darktable.rc.o
# file_rawtherapee_RC = file-rawtherapee.rc.o
# file_raw_placeholder_RC = file-raw-placeholder.rc.o
# endif

AM_LDFLAGS = $(mwindows)

dt_libexecdir = $(gimpplugindir)/plug-ins/file-darktable
rt_libexecdir = $(gimpplugindir)/plug-ins/file-rawtherapee
rp_libexecdir = $(gimpplugindir)/plug-ins/file-raw-placeholder

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	-I$(includedir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)

dt_libexec_PROGRAMS = file-darktable
rt_libexec_PROGRAMS = file-rawtherapee
rp_libexec_PROGRAMS = file-raw-placeholder

file_darktable_SOURCES = \
	file-darktable.c	\
	file-raw-formats.h	\
	file-raw-utils.c	\
	file-raw-utils.h

file_rawtherapee_SOURCES = \
	file-rawtherapee.c	\
	file-raw-formats.h	\
	file-raw-utils.c	\
	file-raw-utils.h

file_raw_placeholder_SOURCES = \
	file-raw-formats.h	\
	file-raw-placeholder.c

file_darktable_LDADD = \
	$(libgimp)		\
	$(libgimpbase)		\
	$(libgimpcolor)		\
	$(libgimpconfig)	\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_darktable_RC)

file_rawtherapee_LDADD = \
	$(libgimp)		\
	$(libgimpbase)		\
	$(libgimpcolor)		\
	$(libgimpconfig)	\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_rawtherapee_RC)

file_raw_placeholder_LDADD = \
	$(libgimp)		\
	$(libgimpbase)		\
	$(libgimpcolor)		\
	$(libgimpconfig)	\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_raw_placeholder_RC)

filerawdata_DATA = \
	file-darktable-export-on-exit.lua	\
	file-darktable-get-size.lua

EXTRA_DIST = $(filerawdata_DATA)
