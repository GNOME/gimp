## Process this file with automake to produce Makefile.in

AM_CFLAGS = -DUSE_INTERFACE=1

INCLUDES = \
	-I$(top_srcdir)		\
	$(GIMP_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libftx.a

libftx_a_SOURCES = ftx.c ftx.h
