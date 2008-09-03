## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	-DUSE_INTERFACE=1	\
	-I$(srcdir)/..		\
	$(GLIB_CFLAGS)

noinst_LIBRARIES = libftx.a

libftx_a_SOURCES = ftx.c ftx.h

EXTRA_DIST = \
	LICENSE			\
	README			\
	ftx-functions.txt	\
	listhome.scm
