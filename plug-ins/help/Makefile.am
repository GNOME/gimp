## Process this file with automake to produce Makefile.in

libgimphelp = libgimphelp.a
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
help_RC = help.rc.o
endif

AM_CPPFLAGS = -DDATADIR=\""$(gimpdatadir)"\"

AM_LDFLAGS = $(mwindows)

noinst_LIBRARIES = libgimphelp.a

libgimphelp_a_SOURCES = \
	gimphelptypes.h		\
	gimphelp.c		\
	gimphelp.h		\
	gimphelpdomain.c	\
	gimphelpdomain.h	\
	gimphelpitem.c		\
	gimphelpitem.h		\
	gimphelplocale.c	\
	gimphelplocale.h	\
	gimphelpprogress.c	\
	gimphelpprogress.h	\
	gimphelpprogress-private.h

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = help

help_SOURCES = help.c

INCLUDES = \
	-I$(top_srcdir)		\
	$(GIO_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimphelp)		\
	$(libgimp)		\
	$(libgimpconfig)	\
	$(libgimpcolor)		\
	$(libgimpbase)		\
	$(libgimpmath)		\
	$(GIO_LIBS)		\
	$(GDK_PIXBUF_LIBS)	\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(help_RC)


noinst_PROGRAMS = gimp-help-lookup

gimp_help_lookup_SOURCES = gimp-help-lookup.c

gimp_help_lookup_LDADD = \
	$(libgimphelp)	\
	$(libgimpbase)	\
	$(GIO_LIBS)
