## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Plug-In\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

noinst_LIBRARIES = libappplug-in.a

libappplug_in_a_SOURCES = @STRIP_BEGIN@ \
	plug-in-types.h \
	plug-in.c	\
	plug-in.h	\
	plug-in-proc.c	\
	plug-in-proc.h	\
	plug-in-rc.c	\
	plug-in-rc.h	\
@STRIP_END@
