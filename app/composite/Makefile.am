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
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libappcomposite.a

libappcomposite_a_sources = \
	gimp-composite.c		\
	gimp-composite.h		\
	gimp-composite-generic.c	\
	gimp-composite-generic.h	\
	gimp-composite-mmx.c		\
	gimp-composite-mmx.h		\
	gimp-composite-sse.c		\
	gimp-composite-sse.h		\
	gimp-composite-util.h

libappcomposite_a_built_sources = \
	gimp-composite-dispatch.c	\
	gimp-composite-dispatch.h

libappcomposite_a_SOURCES = $(libappcomposite_a_built_sources) $(libappcomposite_a_sources)

regenerate:
	$(srcdir)/make-gimp-composite-dispatch.py gimp-composite-generic.o

EXTRA_DIST = \
	make-gimp-composite-dispatch.py	\
	ns.py

EXTRA_PROGRAMS = test-composite gimp-composite-test

#
# unit tests and meta tools for GimpComposite
#

TESTS = test-composite gimp-composite-test

test_composite_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	test-composite.c

test_composite_DEPENDENCIES = $(gimpcomposite_dependencies)

test_composite_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(GLIB_LIBS)


gimp_composite_test_SOURCES = \
	gimp-composite-regression.c	\
	gimp-composite-regression.h	\
	gimp-composite-test.c

gimp_composite_test_DEPENDENCIES = $(gimpcomposite_dependencies)

gimp_composite_test_LDADD = \
	libappcomposite.a					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(GLIB_LIBS)


CLEANFILES = $(EXTRA_PROGRAMS)
