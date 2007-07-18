## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapppaint-funcs.a

libapppaint_funcs_a_SOURCES = \
	paint-funcs-types.h	\
	paint-funcs.c		\
	paint-funcs.h		\
	paint-funcs-generic.h	\
	sample-funcs.c		\
	sample-funcs.h		\
	scale-funcs.c		\
	scale-funcs.h

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Paint-Funcs\"

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/app

EXTRA_DIST = makefile.msc
