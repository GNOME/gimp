## Process this file with automake to produce Makefile.in

AM_CFLAGS = \
	-DSTANDALONE=0		\
	-DUSE_INTERFACE=1	\
	-DUSE_MATH=1		\
	-DUSE_ASCII_NAMES=0	\
	-DUSE_STRLWR=0		\
	-I$(top_srcdir)		\
	$(GLIB_CFLAGS)        

noinst_LIBRARIES = libtinyscheme.a

libtinyscheme_a_SOURCES = \
	scheme.c		\
	opdefines.h		\
	scheme-private.h	\
	scheme.h

EXTRA_DIST = \
	BUILDING		\
	CHANGES			\
	COPYING			\
	hack.txt		\
	Manual.txt		\
	MiniSCHEMETribute.txt
