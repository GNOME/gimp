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
	gimpconfig-types.c		\
	gimpconfig-types.h		\
	gimpbaseconfig.c		\
	gimpbaseconfig.h		\
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

test_config_DEPENDENCIES = \
	libappconfig.a		\
        ../base/libappbase.a

test_config_LDADD = $(GLIB_LIBS) $(test_config_DEPENDENCIES)
