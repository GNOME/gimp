## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DSTANDALONE=0      	\
	-DUSE_INTERFACE=1	\
	-DUSE_MATH=1		\
	-DUSE_ASCII_NAMES=0	\
	-DUSE_STRLWR=0		\
	-DUSE_STRCASECMP=1

INCLUDES = \
	-I$(top_srcdir)		\
	$(GIMP_CFLAGS)		\
	-I$(includedir)

noinst_LIBRARIES = libtinyscheme.a

libtinyscheme_a_SOURCES = \
	scheme.c		\
	opdefines.h		\
	scheme-private.h	\
	scheme.h
