## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Compositing\"	\
	@GIMP_THREAD_FLAGS@			\
	@GIMP_MP_FLAGS@

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/app

INCLUDES = \
	-I$(top_srcdir)/app		\
	-I$(top_srcdir)/app/composite	\
	$(GLIB_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = libappcomposite.a

libappcomposite_a_sources = \
	gimp-composite.c		\
	gimp-composite-generic.c	\
	gimp-composite-generic.h	\
	gimp-composite.h		\
	gimp-composite-mmx.c		\
	gimp-composite-mmx.h		\
	gimp-composite-util.h

libappcomposite_a_built_sources = gimp-composite-dispatch.c

libappcomposite_a_SOURCES = $(libappcomposite_a_built_sources) $(libappcomposite_a_sources)

EXTRA_DIST = make-gimp-composite-dispatch.py

gimp-composite.c: gimp-composite-dispatch.c

gimp-composite-dispatch.c: gimp-composite-generic.o make-gimp-composite-dispatch.py
	./make-gimp-composite-dispatch.py gimp-composite-generic.o > gimp-composite-dispatch.c
