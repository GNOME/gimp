## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	-DUSE_INTERFACE=1	\
	-I$(srcdir)/..		\
	$(GLIB_CFLAGS)

# Is "libtool convenience" not installed and not versioned
noinst_LTLIBRARIES = libftx.la

libftx_la_SOURCES = ftx.c ftx.h

EXTRA_DIST = \
	LICENSE			\
	README			\
	ftx-functions.txt	\
	listhome.scm
