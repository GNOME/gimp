## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappconfig.a

libappconfig_a_SOURCES = @STRIP_BEGIN@ 	\
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
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ 		\
	-DG_LOG_DOMAIN=\"Gimp-Config\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ 		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GLIB_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

#
# test programs, not to be installed
#
noinst_PROGRAMS = test-config

test_config_DEPENDENCIES = @STRIP_BEGIN@ 				\
	libappconfig.a							\
        ../base/libappbase.a						\
        ../core/libappcore.a						\
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
@STRIP_END@

test_config_LDADD = @STRIP_BEGIN@ 	\
	$(GLIB_LIBS) 			\
	$(test_config_DEPENDENCIES)	\
@STRIP_END@
