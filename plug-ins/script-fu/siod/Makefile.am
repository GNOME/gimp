## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libsiod.a

libsiod_a_SOURCES = \
	regex.c		\
	slib.c		\
	sliba.c		\
	trace.c		\
	siod.h		\
	siodp.h

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	-I$(includedir)
