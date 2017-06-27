## Process this file with automake to produce Makefile.in

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

filerawtherapeedatadir = $(gimpdatadir)/file-rawtherapee

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

libexecdir = $(gimpplugindir)/plug-ins

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	-I$(includedir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)

libexec_PROGRAMS = file-rawtherapee

file_rawtherapee_SOURCES = \
	file-rawtherapee.c	\
	file-formats.h

file_rawtherapee_LDADD = \
	$(libgimp)		\
	$(libgimpbase)
