## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libappwidgets.la

libappwidgets_la_SOURCES = \
	gimpbrushpreview.c		\
	gimpbrushpreview.h		\
	gimpchannellistitem.c		\
	gimpchannellistitem.h		\
	gimpchannellistview.c		\
	gimpchannellistview.h		\
	gimpcolorpanel.c		\
	gimpcolorpanel.h		\
	gimpcomponentlistitem.c		\
	gimpcomponentlistitem.h		\
	gimpcontainergridview.c		\
	gimpcontainergridview.h		\
	gimpcontainerlistview.c		\
	gimpcontainerlistview.h		\
	gimpcontainermenu.c		\
	gimpcontainermenu.h		\
	gimpcontainermenuimpl.c		\
	gimpcontainermenuimpl.h		\
	gimpcontainerview.c		\
	gimpcontainerview.h		\
	gimpcontainerview-utils.c	\
	gimpcontainerview-utils.h	\
	gimpdatafactoryview.c		\
	gimpdatafactoryview.h		\
	gimpdialogfactory.c		\
	gimpdialogfactory.h		\
	gimpdnd.c			\
	gimpdnd.h			\
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
	gimpgradientpreview.c		\
	gimpgradientpreview.h		\
	gimphistogramview.c		\
	gimphistogramview.h		\
	gimpimagedock.c			\
	gimpimagedock.h			\
	gimpimagepreview.c		\
	gimpimagepreview.h		\
	gimplayerlistitem.c		\
	gimplayerlistitem.h		\
	gimplayerlistview.c		\
	gimplayerlistview.h		\
	gimplistitem.c			\
	gimplistitem.h			\
	gimpmenuitem.c			\
	gimpmenuitem.h			\
	gimppalettepreview.c		\
	gimppalettepreview.h		\
	gimppatternpreview.c		\
	gimppatternpreview.h		\
	gimppreview.c			\
	gimppreview.h			\
	gimptoolinfopreview.c		\
	gimptoolinfopreview.h		\
	gtkwrapbox.c			\
	gtkwrapbox.h			\
	gtkhwrapbox.c			\
	gtkhwrapbox.h			\
	gtkvwrapbox.c			\
	gtkvwrapbox.h			\
	gimpconstrainedhwrapbox.c	\
	gimpconstrainedhwrapbox.h	\
	widgets-types.h

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
