## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libappcore.la

libappcore_la_SOURCES = \
	core-types.h			\
	gimpbrush.c			\
	gimpbrush.h			\
	gimpbrush-header.h		\
	gimpbrushgenerated.c		\
	gimpbrushgenerated.h		\
	gimpbrushpipe.c			\
	gimpbrushpipe.h			\
	gimpchannel.c			\
	gimpchannel.h			\
	gimpcontainer.c			\
	gimpcontainer.h			\
	gimpcontext.c			\
	gimpcontext.h			\
	gimpdata.c			\
	gimpdata.h			\
	gimpdatafactory.c		\
	gimpdatafactory.h		\
	gimpdatalist.c			\
	gimpdatalist.h			\
	gimpdrawable.c			\
	gimpdrawable.h			\
	gimpdrawable-desaturate.c	\
	gimpdrawable-desaturate.h	\
	gimpdrawable-equalize.c		\
	gimpdrawable-equalize.h		\
	gimpdrawable-invert.c		\
	gimpdrawable-invert.h		\
	gimpdrawable-offset.c		\
	gimpdrawable-offset.h		\
	gimpdrawable-preview.c		\
	gimpdrawable-preview.h		\
	gimpgradient.c			\
	gimpgradient.h			\
	gimpimage.c			\
	gimpimage.h			\
	gimpimage-convert.c		\
	gimpimage-convert.h		\
	gimpimage-duplicate.c		\
	gimpimage-duplicate.h		\
	gimpimage-mask.c		\
	gimpimage-mask.h		\
	gimpimage-undo.c		\
	gimpimage-undo.h		\
	gimplayer.c			\
	gimplayer.h			\
	gimplayermask.c			\
	gimplayermask.h			\
	gimplist.c			\
	gimplist.h			\
	gimpmarshal.c			\
	gimpmarshal.h			\
	gimpobject.c			\
	gimpobject.h			\
	gimppattern.c			\
	gimppattern.h			\
	gimppattern-header.h		\
	gimppalette.c			\
	gimppalette.h			\
	gimppalette-import.c		\
	gimppalette-import.h		\
	gimptoolinfo.c			\
	gimptoolinfo.h			\
	gimpundo.c			\
	gimpundo.h			\
	gimpundostack.c			\
	gimpundostack.h			\
	gimpviewable.c			\
	gimpviewable.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Core\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
