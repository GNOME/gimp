## Process this file with automake to produce Makefile.in

SUBDIRS = preferences

themedatadir = $(gimpdatadir)/themes/Default/images


## Icons that override GTK+ default stock icons

DIALOG_IMAGES = @STRIP_BEGIN@ \
	gimp-dialog-error.png			\
	gimp-dialog-info.png			\
	gimp-dialog-question.png		\
	gimp-dialog-warning.png			\
@STRIP_END@

GTK_STOCK_IMAGES = @STRIP_BEGIN@ \
	stock-button-delete.png			\
	stock-button-lower.png			\
	stock-button-new.png			\
	stock-button-paste.png			\
	stock-button-raise.png			\
	stock-button-refresh.png		\
@STRIP_END@


## Wilber icons for the window managaer, compiled in

WILBER_IMAGES = @STRIP_BEGIN@ \
	stock-wilber-16x16.png			\
	stock-wilber-32x32.png			\
	stock-wilber-48x48.png			\
	stock-wilber-64x64.png			\
@STRIP_END@

WILBER_VARIABLES = @STRIP_BEGIN@ \
	stock_wilber_16_16  $(srcdir)/stock-wilber-16x16.png	\
	stock_wilber_32_32  $(srcdir)/stock-wilber-32x32.png	\
	stock_wilber_48_48  $(srcdir)/stock-wilber-48x48.png	\
	stock_wilber_64_64  $(srcdir)/stock-wilber-64x64.png	\
@STRIP_END@


## Below are compiled in stock icons 

STOCK_BUTTON_IMAGES = @STRIP_BEGIN@ \
	stock-button-anchor.png			\
	stock-button-duplicate.png		\
	stock-button-edit.png			\
	stock-button-eye.png			\
	stock-button-linked.png			\
	stock-button-paste-as-new.png		\
	stock-button-paste-into.png		\
	stock-button-reset.png			\
	stock-button-selection-replace.png	\
	stock-button-selection-add.png		\
	stock-button-selection-subtract.png	\
	stock-button-selection-intersect.png	\
	stock-button-selection-stroke.png	\
	stock-button-selection-to-path.png	\
	stock-button-path-stroke.png		\
	stock-button-hchain.png			\
	stock-button-hchain-broken.png		\
	stock-button-vchain.png			\
	stock-button-vchain-broken.png		\
@STRIP_END@

STOCK_MENU_IMAGES = @STRIP_BEGIN@ \
	stock-menu-convert-grayscale.png	\
	stock-menu-convert-indexed.png		\
	stock-menu-convert-rgb.png		\
	stock-menu-merge-down.png		\
	stock-menu-reshow-filter.png		\
	stock-menu-rotate-180.png		\
	stock-menu-rotate-270.png		\
	stock-menu-rotate-90.png		\
	stock-menu-resize.png			\
	stock-menu-scale.png			\
	\
	stock-menu-navigation.png		\
	stock-menu-qmask-off.png		\
	stock-menu-qmask-on.png			\
@STRIP_END@

STOCK_WILBER_IMAGES = @STRIP_BEGIN@ \
	stock-wilber-eek.png			\
@STRIP_END@

STOCK_TOOL_IMAGES = @STRIP_BEGIN@ \
	tools/stock-tool-button-airbrush.png		\
	tools/stock-tool-button-bezier-select.png	\
	tools/stock-tool-button-blend.png		\
	tools/stock-tool-button-blur.png		\
	tools/stock-tool-button-brightness-contrast.png	\
	tools/stock-tool-button-bucket-fill.png		\
	tools/stock-tool-button-by-color-select.png	\
	tools/stock-tool-button-clone.png		\
	tools/stock-tool-button-color-balance.png	\
	tools/stock-tool-button-color-picker.png	\
	tools/stock-tool-button-crop.png		\
	tools/stock-tool-button-curves.png		\
	tools/stock-tool-button-dodge.png		\
	tools/stock-tool-button-ellipse-select.png	\
	tools/stock-tool-button-eraser.png		\
	tools/stock-tool-button-flip.png		\
	tools/stock-tool-button-free-select.png		\
	tools/stock-tool-button-fuzzy-select.png	\
	tools/stock-tool-button-histogram.png		\
	tools/stock-tool-button-hue-saturation.png	\
	tools/stock-tool-button-ink.png			\
	tools/stock-tool-button-iscissors.png		\
	tools/stock-tool-button-levels.png		\
	tools/stock-tool-button-measure.png		\
	tools/stock-tool-button-move.png		\
	tools/stock-tool-button-paintbrush.png		\
	tools/stock-tool-button-path.png		\
	tools/stock-tool-button-pencil.png		\
	tools/stock-tool-button-perspective.png		\
	tools/stock-tool-button-posterize.png		\
	tools/stock-tool-button-rect-select.png		\
	tools/stock-tool-button-rotate.png		\
	tools/stock-tool-button-scale.png		\
	tools/stock-tool-button-shear.png		\
	tools/stock-tool-button-smudge.png		\
	tools/stock-tool-button-text.png		\
	tools/stock-tool-button-threshold.png		\
	tools/stock-tool-button-zoom.png		\
@STRIP_END@

STOCK_IMAGES = \
	$(STOCK_BUTTON_IMAGES)	\
	$(STOCK_MENU_IMAGES) 	\
	$(STOCK_TOOL_IMAGES)	\
	$(STOCK_WILBER_IMAGES)

themedata_DATA = $(DIALOG_IMAGES) $(GTK_STOCK_IMAGES) $(STOCK_IMAGES)

EXTRA_DIST = \
	$(themedata_DATA)	\
	$(WILBER_IMAGES)	\
	gimp-dialog-icons.xcf.gz


noinst_DATA = gimp-stock-pixbufs.h gimp-wilber-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo -n $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

gimp-stock-pixbufs.h: stock-icons.list
	gdk-pixbuf-csource --raw --build-list `cat stock-icons.list` >$(srcdir)/gimp-stock-pixbufs.h


gimp-wilber-pixbufs.h: $(WILBER_IMAGES)
	gdk-pixbuf-csource --raw --build-list $(WILBER_VARIABLES) >$(srcdir)/gimp-wilber-pixbufs.h
