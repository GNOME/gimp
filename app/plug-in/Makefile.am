## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Plug-In\"	\
	@GTHREAD_CFLAGS@

INCLUDES = \
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app	\
	$(GDK_PIXBUF_CFLAGS)	\
	-I$(includedir)

noinst_LIBRARIES = libappplug-in.a

libappplug_in_a_SOURCES = \
	plug-in-enums.h 	\
	plug-in-types.h 	\
	plug-in.c		\
	plug-in.h		\
	plug-in-context.c	\
	plug-in-context.h	\
	plug-in-debug.c		\
	plug-in-debug.h		\
	plug-in-def.c		\
	plug-in-def.h		\
	plug-in-message.c	\
	plug-in-message.h	\
	plug-in-params.c	\
	plug-in-params.h	\
	plug-in-proc-def.c	\
	plug-in-proc-def.h	\
	plug-in-proc-frame.c	\
	plug-in-proc-frame.h	\
	plug-in-progress.c	\
	plug-in-progress.h	\
	plug-in-rc.c		\
	plug-in-rc.h		\
	plug-in-run.c		\
	plug-in-run.h		\
	plug-in-shm.c		\
	plug-in-shm.h		\
	plug-ins.c		\
	plug-ins.h		\
	plug-ins-query.c	\
	plug-ins-query.h

EXTRA_DIST = makefile.msc
