## Process this file with automake to produce Makefile.in

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = help

help_SOURCES = \
	domain.c	\
	domain.h	\
	locales.c	\
	locales.h	\
	help.c		\
	help.h

INCLUDES = \
	-I$(top_srcdir)		\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

LDADD = \
	$(libgimp)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)
