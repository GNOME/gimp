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
	gimpconfig-path.c		\
	gimpconfig-path.h		\
	gimpconfig-serialize.c		\
	gimpconfig-serialize.h		\
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
	gimprc-blurbs.h			\
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


gimpconfig_libs = \
	libappconfig.a							\
        ../base/libappbase.a						\
        ../core/libappcore.a						\
        ../display/libappdisplay.a					\
        ../widgets/libappwidgets.a					\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la


#
# extra programs, not to be built by default and never installed
#

EXTRA_PROGRAMS = gimpconfig-dump test-config

gimpconfig_dump_SOURCES = gimpconfig-dump.c

gimpconfig_dump_LDADD = $(GLIB_LIBS) $(gimpconfig_libs)

TESTS = test-config

test_config_DEPENDENCIES = $(gimpconfig_dependencies)

test_config_LDADD =  $(GLIB_LIBS) $(gimpconfig_libs)
