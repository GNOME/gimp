## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libgimpcomposite.a

libgimpcomposite_a_sources = \
	gimp-composite.c \
	gimp-composite-generic.c \
	gimp-composite-generic.h \
	gimp-composite.h \
	gimp-composite-mmx.c \
	gimp-composite-mmx.h \
	gimp-composite-util.h

libgimpcomposite_a_built_sources = gimp-composite-dispatch.c

libgimpcomposite_a_SOURCES = $(libgimpcomposite_a_built_sources) $(libgimpcomposite_a_sources)

INCLUDES = \
	-I$(top_srcdir)/app	\
	-I$(top_srcdir)/app/composite	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Compositing\"	\
	@GIMP_THREAD_FLAGS@ 			\
	@GIMP_MP_FLAGS@

AM_CCASFLAGS = \
	-I$(top_builddir)	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/app

EXTRA_DIST = makefile.msc

gimp-composite.c: gimp-composite-dispatch.c

gimp-composite-dispatch.c: gimp-composite-generic.o make-gimp-composite-dispatch.py
	./make-gimp-composite-dispatch.py gimp-composite-generic.o > gimp-composite-dispatch.c
