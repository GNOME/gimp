## Process this file with automake to produce Makefile.in

SUBDIRS = preferences

themedatadir = $(gimpdatadir)/themes/Default/images


## Wilber icons for the window managaer, compiled in

WILBER_IMAGES = \
	stock-wilber-16.png	\
	stock-wilber-32.png	\
	stock-wilber-48.png	\
	stock-wilber-64.png

WILBER_VARIABLES = \
	stock_wilber_16  $(srcdir)/stock-wilber-16.png	\
	stock_wilber_32  $(srcdir)/stock-wilber-32.png	\
	stock_wilber_48  $(srcdir)/stock-wilber-48.png	\
	stock_wilber_64  $(srcdir)/stock-wilber-64.png


## Below are compiled in stock icons 

STOCK_BUTTON_IMAGES = \
	stock-anchor-16.png			\
	stock-char-picker-22.png		\
	stock-color-picker-black-18.png		\
	stock-color-picker-gray-18.png		\
	stock-color-picker-white-18.png		\
	stock-color-triangle-16.png		\
	stock-curve-free-16.png			\
	stock-curve-smooth-16.png		\
	stock-duplicate-16.png			\
	stock-edit-16.png			\
	stock-eye-20.png			\
	stock-gravity-east-24.png		\
	stock-gravity-north-24.png		\
	stock-gravity-north-east-24.png		\
	stock-gravity-north-west-24.png		\
	stock-gravity-south-24.png		\
	stock-gravity-south-east-24.png		\
	stock-gravity-south-west-24.png		\
	stock-gravity-west-24.png		\
	stock-line-spacing-22.png		\
	stock-linked-20.png			\
	stock-letter-spacing-22.png		\
	stock-paste-as-new-16.png		\
	stock-paste-into-16.png			\
	stock-reset-16.png			\
	stock-selection-replace-16.png		\
	stock-selection-add-16.png		\
	stock-selection-subtract-16.png		\
	stock-selection-intersect-16.png	\
	stock-selection-stroke-16.png		\
	stock-selection-to-channel-16.png	\
	stock-selection-to-path-16.png		\
	stock-path-stroke-16.png		\
	stock-hchain-24.png			\
	stock-hchain-broken-24.png		\
	stock-vchain-24.png			\
	stock-vchain-broken-24.png		\
	stock-info-24.png			\
	stock-warning-24.png

STOCK_MENU_IMAGES = \
	stock-convert-grayscale-16.png		\
	stock-convert-indexed-16.png		\
	stock-convert-rgb-16.png		\
	stock-invert-16.png			\
	stock-merge-down-16.png			\
	stock-layer-to-imagesize-16.png		\
	stock-plugin-16.png			\
	stock-reshow-filter-16.png		\
	stock-rotate-180-16.png			\
	stock-rotate-270-16.png			\
	stock-rotate-90-16.png			\
	stock-resize-16.png			\
	stock-scale-16.png			\
	stock-images-16.png			\
	stock-layers-16.png			\
	stock-channels-16.png			\
	stock-paths-16.png			\
	stock-selection-all-16.png		\
	stock-selection-none-16.png		\
	stock-selection-grow-16.png		\
	stock-selection-shrink-16.png		\
	stock-navigation-16.png			\
	stock-qmask-off-16.png			\
	stock-qmask-on-16.png			\
	stock-default-colors-12.png		\
	stock-swap-colors-12.png		\
	stock-tool-options-16.png		\
	stock-device-status-16.png		\
	stock-info-16.png			\
	stock-warning-16.png			\
	stock-wilber-16.png

STOCK_DIALOG_IMAGES = \
	stock-error-64.png			\
	stock-info-64.png			\
	stock-question-64.png			\
	stock-warning-64.png			\
	stock-wilber-64.png			\
	stock-wilber-eek-64.png

STOCK_TOOL_IMAGES = \
	tools/stock-tool-airbrush-22.png		\
	tools/stock-tool-bezier-select-22.png		\
	tools/stock-tool-blend-22.png			\
	tools/stock-tool-blur-22.png			\
	tools/stock-tool-brightness-contrast-22.png	\
	tools/stock-tool-bucket-fill-22.png		\
	tools/stock-tool-by-color-select-22.png		\
	tools/stock-tool-clone-22.png			\
	tools/stock-tool-color-balance-22.png		\
	tools/stock-tool-color-picker-22.png		\
	tools/stock-tool-crop-22.png			\
	tools/stock-tool-curves-22.png			\
	tools/stock-tool-dodge-22.png			\
	tools/stock-tool-ellipse-select-22.png		\
	tools/stock-tool-eraser-22.png			\
	tools/stock-tool-flip-22.png			\
	tools/stock-tool-free-select-22.png		\
	tools/stock-tool-fuzzy-select-22.png		\
	tools/stock-tool-histogram-22.png		\
	tools/stock-tool-hue-saturation-22.png		\
	tools/stock-tool-ink-22.png			\
	tools/stock-tool-iscissors-22.png		\
	tools/stock-tool-levels-22.png			\
	tools/stock-tool-measure-22.png			\
	tools/stock-tool-move-22.png			\
	tools/stock-tool-paintbrush-22.png		\
	tools/stock-tool-path-22.png			\
	tools/stock-tool-pencil-22.png			\
	tools/stock-tool-perspective-22.png		\
	tools/stock-tool-posterize-22.png		\
	tools/stock-tool-rect-select-22.png		\
	tools/stock-tool-rotate-22.png			\
	tools/stock-tool-scale-22.png			\
	tools/stock-tool-shear-22.png			\
	tools/stock-tool-smudge-22.png			\
	tools/stock-tool-text-22.png			\
	tools/stock-tool-threshold-22.png		\
	tools/stock-tool-zoom-22.png

STOCK_IMAGES = \
	$(STOCK_BUTTON_IMAGES)	\
	$(STOCK_MENU_IMAGES) 	\
	$(STOCK_DIALOG_IMAGES)	\
	$(STOCK_TOOL_IMAGES)

themedata_DATA = $(STOCK_IMAGES)

EXTRA_DIST = \
	$(themedata_DATA)	\
	$(WILBER_IMAGES)	\
	gimp-dialog-icons.xcf.gz


noinst_DATA = gimp-stock-pixbufs.h gimp-wilber-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/gimp-stock-pixbufs.h: stock-icons.list
	gdk-pixbuf-csource --raw --build-list `cat stock-icons.list` > $(@F)


$(srcdir)/gimp-wilber-pixbufs.h: $(WILBER_IMAGES)
	gdk-pixbuf-csource --raw --build-list $(WILBER_VARIABLES) > $(@F)
