## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappconfig.a

libappconfig_a_SOURCES = \
	config-types.h			\
	gimpconfig.c			\
	gimpconfig.h			\
	gimpconfig-deserialize.c	\
	gimpconfig-deserialize.h	\
	gimpconfig-params.c		\
	gimpconfig-params.h		\
	gimpconfig-serialize.c		\
	gimpconfig-serialize.h		\
	gimpconfig-substitute.c		\
	gimpconfig-substitute.h		\
	gimpconfig-serialize.h		\
	gimpconfig-types.c		\
	gimpconfig-types.h		\
	gimpconfig-utils.c		\
	gimpconfig-utils.h		\
	gimpbaseconfig.c		\
	gimpbaseconfig.h		\
	gimpcoreconfig.c		\
	gimpcoreconfig.h		\
	gimpdisplayconfig.c		\
	gimpdisplayconfig.h		\
	gimpguiconfig.c			\
	gimpguiconfig.h			\
	gimprc.c			\
	gimprc.h			\
	gimpscanner.c			\
	gimpscanner.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Config\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)		\
	-I$(top_srcdir)/app	\
	$(GLIB_CFLAGS)		\
	-I$(includedir)


#
# test programs, not to be built by default and never installed
#

TESTS = test-config

EXTRA_PROGRAMS = test-config

test_config_DEPENDENCIES = \
	libappconfig.a							\
        ../base/libappbase.a						\
        ../core/libappcore.a						\
        ../display/libappdisplay.a					\
        ../widgets/libappwidgets.a					\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la

test_config_LDADD = \
	$(GLIB_LIBS) 			\
	$(test_config_DEPENDENCIES)
