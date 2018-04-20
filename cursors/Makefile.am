## Process this file with automake to produce Makefile.in

CURSOR_IMAGES = \
	cursor-bad.png				\
	cursor-color-picker.png			\
	cursor-corner-bottom-left.png		\
	cursor-corner-bottom-right.png		\
	cursor-corner-bottom.png		\
	cursor-corner-left.png			\
	cursor-corner-right.png			\
	cursor-corner-top-left.png		\
	cursor-corner-top-right.png		\
	cursor-corner-top.png			\
	cursor-crosshair-small.png		\
	cursor-crosshair.png			\
	cursor-mouse.png			\
	cursor-move.png				\
	cursor-none.png				\
	cursor-side-bottom-left.png		\
	cursor-side-bottom-right.png		\
	cursor-side-bottom.png			\
	cursor-side-left.png			\
	cursor-side-right.png			\
	cursor-side-top-left.png		\
	cursor-side-top-right.png		\
	cursor-side-top.png			\
	cursor-zoom.png				\
	\
	modifier-anchor.png			\
	modifier-bad.png			\
	modifier-background.png			\
	modifier-control.png			\
	modifier-foreground.png			\
	modifier-intersect.png			\
	modifier-join.png			\
	modifier-minus.png			\
	modifier-move.png			\
	modifier-pattern.png			\
	modifier-plus.png			\
	modifier-resize.png			\
	modifier-rotate.png			\
	modifier-select.png			\
	modifier-zoom.png			\
	\
	tool-airbrush.png			\
	tool-blur.png				\
	tool-bucket-fill.png			\
	tool-burn.png				\
	tool-clone.png				\
	tool-color-picker.png			\
	tool-crop.png				\
	tool-dodge.png				\
	tool-ellipse-select.png			\
	tool-eraser.png				\
	tool-flip-horizontal.png		\
	tool-flip-vertical.png			\
	tool-free-select.png			\
	tool-fuzzy-select.png			\
	tool-gradient.png			\
	tool-hand.png				\
	tool-heal.png				\
	tool-ink.png				\
	tool-iscissors.png			\
	tool-measure.png			\
	tool-move.png				\
	tool-paintbrush.png			\
	tool-paths.png				\
	tool-paths-anchor.png			\
	tool-paths-control.png			\
	tool-paths-segment.png			\
	tool-pencil.png				\
	tool-perspective.png			\
	tool-polygon-select.png			\
	tool-rect-select.png			\
	tool-resize.png				\
	tool-rotate.png				\
	tool-shear.png				\
	tool-smudge.png				\
	tool-text.png				\
	tool-warp.png				\
	tool-zoom.png

COLOR_PICKER_IMAGES = \
	cursor-color-picker.png

EXTRA_DIST = \
	$(CURSOR_IMAGES)			\
	gimp-tool-cursors.xcf

noinst_DATA = \
	gimp-tool-cursors.c			\
	gimp-color-picker-cursors.c

CLEANFILES = \
	$(noinst_DATA)				\
	gimp-tool-cursors.gresource.xml		\
	gimp-color-picker-cursors.gresource.xml


gimp-tool-cursors.gresource.xml: $(CURSOR_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/gimp/tool-cursors">' >> $@; \
	  for image in $(CURSOR_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )

gimp-tool-cursors.c: gimp-tool-cursors.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ gimp-tool-cursors.gresource.xml


gimp-color-picker-cursors.gresource.xml: $(CURSOR_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  echo '<?xml version="1.0" encoding="UTF-8"?>' > $@; \
	  echo '<gresources>' >> $@; \
	  echo '  <gresource prefix="/org/gimp/color-picker-cursors">' >> $@; \
	  for image in $(COLOR_PICKER_IMAGES); do \
	    echo "    <file preprocess=\"to-pixdata\">$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '  <gresource prefix="/org/gimp/color-picker-cursors-raw">' >> $@; \
	  for image in $(COLOR_PICKER_IMAGES); do \
	    echo "    <file>$$image</file>" >> $@; \
	  done; \
	  echo '  </gresource>' >> $@; \
	  echo '</gresources>' >> $@ )

gimp-color-picker-cursors.c: gimp-color-picker-cursors.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(srcdir) --generate-source \
	  --target=$@ gimp-color-picker-cursors.gresource.xml
