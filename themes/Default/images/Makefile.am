## Process this file with automake to produce Makefile.in

SUBDIRS = tools

themedatadir = $(gimpdatadir)/themes/Default/images

STOCK_IMAGES = @STRIP_BEGIN@ \
	stock-button-anchor.png			\
	stock-button-delete.png			\
	stock-button-duplicate.png		\
	stock-button-edit.png			\
	stock-button-eye.png			\
	stock-button-linked.png			\
	stock-button-lower.png			\
	stock-button-new.png			\
	stock-button-paste.png			\
	stock-button-paste-as-new.png		\
	stock-button-paste-into.png		\
	stock-button-stroke.png			\
	stock-button-raise.png			\
	stock-button-refresh.png		\
	stock-button-to-path.png		\
	stock-button-to-selection.png		\
	stock-button-selection-replace.png	\
	stock-button-selection-add.png		\
	stock-button-selection-subtract.png	\
	stock-button-selection-intersect.png	\
	stock-menu-navigation.png		\
	stock-menu-qmask-off.png		\
	stock-menu-qmask-on.png			\
@STRIP_END@

WILBER_IMAGES = @STRIP_BEGIN@ \
	stock-wilber-16x16.png			\
	stock-wilber-32x32.png			\
	stock-wilber-48x48.png			\
	stock-wilber-64x64.png			\
@STRIP_END@

themedata_DATA = $(STOCK_IMAGES)

EXTRA_DIST = $(themedata_DATA) $(WILBER_IMAGES)
