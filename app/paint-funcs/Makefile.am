## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Paint-Funcs\"

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/app

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(CAIRO_CFLAGS)		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libapppaint-funcs.a

libapppaint_funcs_a_SOURCES = \
	paint-funcs.c		\
	paint-funcs.h		\
	paint-funcs-generic.h	\
	paint-funcs-types.h	\
	paint-funcs-utils.h	\
	reduce-region.c		\
	reduce-region.h		\
	scale-region.c		\
	scale-region.h		\
	subsample-region.c	\
	subsample-region.h

EXTRA_DIST = makefile.msc
