## Process this file with automake to produce Makefile.in

libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

AM_CPPFLAGS = -DDATADIR=\""$(gimpdatadir)"\"

AM_LDFLAGS = $(mwindows)

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
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimp)	\
	$(libgimpbase)	\
	$(GLIB_LIBS)	\
	$(RT_LIBS)	\
	$(INTLLIBS)


noinst_PROGRAMS = gimp-help-lookup

gimp_help_lookup_SOURCES = \
	domain.c	\
	domain.h	\
	locales.c	\
	locales.h	\
	help.h		\
	gimp-help-lookup.c

gimp_help_lookup_LDADD = $(GLIB_LIBS)
