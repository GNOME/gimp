## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libappwidgets.la

libappwidgets_la_SOURCES = \
	gimpbrushpreview.c		\
	gimpbrushpreview.h		\
	gimpcontainergridview.h		\
	gimpcontainergridview.c		\
	gimpcontainerlistview.h		\
	gimpcontainerlistview.c		\
	gimpcontainerview.h		\
	gimpcontainerview.c		\
	gimpdatafactoryview.h		\
	gimpdatafactoryview.c		\
	gimpdock.c			\
	gimpdock.h			\
	gimpdockable.c			\
	gimpdockable.h			\
	gimpdockbook.c			\
	gimpdockbook.h			\
	gimpdrawablelistitem.c		\
	gimpdrawablelistitem.h		\
	gimpdrawablelistview.c		\
	gimpdrawablelistview.h		\
	gimpdrawablepreview.c		\
	gimpdrawablepreview.h		\
	gimpgradientpreview.h		\
	gimpgradientpreview.c		\
	gimphistogramview.c		\
	gimphistogramview.h		\
	gimpimagepreview.h		\
	gimpimagepreview.c		\
	gimplayerlistitem.h		\
	gimplayerlistitem.c		\
	gimplayerlistview.h		\
	gimplayerlistview.c		\
	gimplistitem.h			\
	gimplistitem.c			\
	gimppalettepreview.h		\
	gimppalettepreview.c		\
	gimppatternpreview.h		\
	gimppatternpreview.c		\
	gimppreview.h			\
	gimppreview.c			\
	gimptoolinfopreview.h		\
	gimptoolinfopreview.c		\
	gtkwrapbox.c			\
	gtkwrapbox.h			\
	gtkhwrapbox.c			\
	gtkhwrapbox.h			\
	gtkvwrapbox.c			\
	gtkvwrapbox.h			\
	gimpconstrainedhwrapbox.c	\
	gimpconstrainedhwrapbox.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Widgets\"

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
