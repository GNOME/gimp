## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libappbase.la

libappbase_la_SOURCES = \
	base.c			\
	base.h			\
	base-types.h		\
	base-config.c		\
	base-config.h		\
	boundary.c		\
	boundary.h		\
	brush-scale.c		\
	brush-scale.h		\
	gimplut.c		\
	gimplut.h		\
	pixel-processor.c	\
	pixel-processor.h	\
	pixel-region.c		\
	pixel-region.h		\
	pixel-surround.c	\
	pixel-surround.h	\
	temp-buf.c		\
	temp-buf.h		\
	tile.c			\
	tile.h			\
	tile-private.h		\
	tile-cache.c		\
	tile-cache.h		\
	tile-manager.c		\
	tile-manager.h		\
	tile-manager-private.h	\
	tile-swap.c		\
	tile-swap.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Base\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GLIB_CFLAGS)			\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
