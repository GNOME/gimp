## Process this file with automake to produce Makefile.in

themedatadir = $(gimpdatadir)/themes/Default/images/tools

STOCK_TOOL_IMAGES = @STRIP_BEGIN@ \
	stock-tool-button-airbrush.png			\
	stock-tool-button-bezier-select.png		\
	stock-tool-button-blend.png			\
	stock-tool-button-blur.png			\
	stock-tool-button-brightness-contrast.png	\
	stock-tool-button-bucket-fill.png		\
	stock-tool-button-by-color-select.png		\
	stock-tool-button-clone.png			\
	stock-tool-button-color-balance.png		\
	stock-tool-button-color-picker.png		\
	stock-tool-button-crop.png			\
	stock-tool-button-curves.png			\
	stock-tool-button-dodge.png			\
	stock-tool-button-ellipse-select.png		\
	stock-tool-button-eraser.png			\
	stock-tool-button-flip.png			\
	stock-tool-button-free-select.png		\
	stock-tool-button-fuzzy-select.png		\
	stock-tool-button-histogram.png			\
	stock-tool-button-hue-saturation.png		\
	stock-tool-button-ink.png			\
	stock-tool-button-iscissors.png			\
	stock-tool-button-levels.png			\
	stock-tool-button-measure.png			\
	stock-tool-button-move.png			\
	stock-tool-button-paintbrush.png		\
	stock-tool-button-path.png			\
	stock-tool-button-pencil.png			\
	stock-tool-button-perspective.png		\
	stock-tool-button-posterize.png			\
	stock-tool-button-rect-select.png		\
	stock-tool-button-rotate.png			\
	stock-tool-button-scale.png			\
	stock-tool-button-shear.png			\
	stock-tool-button-smudge.png			\
	stock-tool-button-text.png			\
	stock-tool-button-threshold.png			\
	stock-tool-button-zoom.png			\
@STRIP_END@

themedata_DATA = $(STOCK_TOOL_IMAGES)

EXTRA_DIST = $(themedata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
