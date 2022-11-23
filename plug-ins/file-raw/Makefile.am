## Process this file with automake to produce Makefile.in

libligma = $(top_builddir)/libligma/libligma-$(LIGMA_API_VERSION).la
libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la

filerawdatadir = $(ligmadatadir)/file-raw

if OS_WIN32
mwindows = -mwindows
endif

# if HAVE_WINDRES
# include $(top_srcdir)/build/windows/ligmarc-plug-ins.rule
# file_darktable_RC = file-darktable.rc.o
# file_rawtherapee_RC = file-rawtherapee.rc.o
# file_raw_placeholder_RC = file-raw-placeholder.rc.o
# endif

AM_LDFLAGS = $(mwindows)

dt_libexecdir = $(ligmaplugindir)/plug-ins/file-darktable
rt_libexecdir = $(ligmaplugindir)/plug-ins/file-rawtherapee
rp_libexecdir = $(ligmaplugindir)/plug-ins/file-raw-placeholder

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
	$(libligma)		\
	$(libligmabase)		\
	$(libligmacolor)		\
	$(libligmaconfig)	\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_darktable_RC)

file_rawtherapee_LDADD = \
	$(libligma)		\
	$(libligmabase)		\
	$(libligmacolor)		\
	$(libligmaconfig)	\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_rawtherapee_RC)

file_raw_placeholder_LDADD = \
	$(libligma)		\
	$(libligmabase)		\
	$(libligmacolor)		\
	$(libligmaconfig)	\
	$(CAIRO_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_raw_placeholder_RC)

filerawdata_DATA = \
	file-darktable-export-on-exit.lua	\
	file-darktable-get-size.lua

EXTRA_DIST = $(filerawdata_DATA)
