## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libappwidgets.a

libappwidgets_a_SOURCES = @STRIP_BEGIN@ \
	widgets-types.h			\
	gimpbrushfactoryview.c		\
	gimpbrushfactoryview.h		\
	gimpbrushpreview.c		\
	gimpbrushpreview.h		\
	gimpbufferpreview.c		\
	gimpbufferpreview.h		\
	gimpbufferview.c		\
	gimpbufferview.h		\
	gimpchannellistitem.c		\
	gimpchannellistitem.h		\
	gimpchannellistview.c		\
	gimpchannellistview.h		\
	gimpcolorpanel.c		\
	gimpcolorpanel.h		\
	gimpcomponentlistitem.c		\
	gimpcomponentlistitem.h		\
	gimpcontainereditor.c		\
	gimpcontainereditor.h		\
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
	gimpcursor.c			\
	gimpcursor.h			\
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
	gimpfontselection.c		\
	gimpfontselection.h		\
	gimpfontselection-dialog.c	\
	gimpfontselection-dialog.h	\
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
	gimpnavigationpreview.c		\
	gimpnavigationpreview.h		\
	gimppalettepreview.c		\
	gimppalettepreview.h		\
	gimppatternpreview.c		\
	gimppatternpreview.h		\
	gimppreview.c			\
	gimppreview.h			\
	gimptoolinfopreview.c		\
	gimptoolinfopreview.h		\
	gimpwidgets-constructors.c	\
	gimpwidgets-constructors.h	\
	gimpwidgets-utils.c		\
	gimpwidgets-utils.h		\
	gtkwrapbox.c			\
	gtkwrapbox.h			\
	gtkhwrapbox.c			\
	gtkhwrapbox.h			\
	gtkvwrapbox.c			\
	gtkvwrapbox.h			\
	gimpconstrainedhwrapbox.c	\
	gimpconstrainedhwrapbox.h	\
@STRIP_END@

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Widgets\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	$(PANGOFT2_CFLAGS)		\
	-I$(includedir)			\
@STRIP_END@

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
