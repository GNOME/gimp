## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libgimpoldpreview.a

libgimpoldpreview_a_SOURCES = \
	gimpoldpreview.c	\
	gimpoldpreview.h

INCLUDES = \
	-I..			\
	-I$(srcdir)/..		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/libgimp \
	$(GTK_CFLAGS)		\
	-I$(includedir)
