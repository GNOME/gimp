## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappconfig.a

libappconfig_a_SOURCES = @STRIP_BEGIN@ 	\
	gimpconfig.c			\
	gimpconfig.h			\
	gimpconfig-deserialize.c	\
	gimpconfig-deserialize.h	\
	gimpconfig-serialize.c		\
	gimpconfig-serialize.h		\
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
