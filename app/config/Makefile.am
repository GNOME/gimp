## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

noinst_LIBRARIES = libappconfig.a

libappconfig_a_SOURCES = \
	config-types.h			\
	gimpconfig.c			\
	gimpconfig.h			\
	gimpconfig-deserialize.c	\
	gimpconfig-deserialize.h	\
	gimpconfig-dump.c		\
	gimpconfig-dump.h		\
	gimpconfig-error.c		\
	gimpconfig-error.h		\
	gimpconfig-params.c		\
	gimpconfig-params.h		\
	gimpconfig-path.c		\
	gimpconfig-path.h		\
	gimpconfig-serialize.c		\
	gimpconfig-serialize.h		\
	gimpconfig-serialize.h		\
	gimpconfig-types.c		\
	gimpconfig-types.h		\
	gimpconfig-utils.c		\
	gimpconfig-utils.h		\
	gimpconfigwriter.c		\
	gimpconfigwriter.h		\
	gimpbaseconfig.c		\
	gimpbaseconfig.h		\
	gimpcoreconfig.c		\
	gimpcoreconfig.h		\
	gimpdisplayconfig.c		\
	gimpdisplayconfig.h		\
	gimpguiconfig.c			\
	gimpguiconfig.h			\
	gimppluginconfig.c		\
	gimppluginconfig.h		\
	gimprc.c			\
	gimprc.h			\
	gimprc-blurbs.h			\
	gimpscanner.c			\
	gimpscanner.h			\
	gimpxmlparser.c			\
	gimpxmlparser.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Config\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)


EXTRA_PROGRAMS = test-config

EXTRA_DIST = makefile.msc

#
# unit tests for the GimpConfig system
#

TESTS = test-config

test_config_DEPENDENCIES = $(gimpconfig_libs)

test_config_LDADD = \
	../base/libappbase.a	\
	../core/libappcore.a	\
	libappconfig.a		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(GLIB_LIBS)


CLEANFILES = $(EXTRA_PROGRAMS) foorc
