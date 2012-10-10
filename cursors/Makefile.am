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
	modifier-select.png			\
	\
	tool-airbrush.png			\
	tool-blend.png				\
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
	tool-zoom.png

COLOR_PICKER_IMAGES = \
	cursor-color-picker.png

EXTRA_DIST = \
	$(CURSOR_IMAGES)			\
	gimp-tool-cursors.xcf

noinst_DATA = \
	gimp-tool-cursors.h			\
	gimp-color-picker-cursors.h

CLEANFILES = \
	gimp-tool-cursors.h			\
	tool-cursors.list			\
	gimp-color-picker-cursors.h		\
	color-picker-cursors.list

tool-cursors.list: $(CURSOR_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  for image in $(CURSOR_IMAGES); do \
	    echo $$image | \
	      sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

color-picker-cursors.list: $(COLOR_PICKER_IMAGES) Makefile.am
	$(AM_V_GEN) ( rm -f $@; \
	  for image in $(COLOR_PICKER_IMAGES); do \
	    echo $$image | \
	      sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/gimp-tool-cursors.h: tool-cursors.list
	$(AM_V_GEN) $(GDK_PIXBUF_CSOURCE) --raw --build-list `cat tool-cursors.list` > $(@F)

$(srcdir)/gimp-color-picker-cursors.h: color-picker-cursors.list
	$(AM_V_GEN) $(GDK_PIXBUF_CSOURCE) --raw --build-list `cat color-picker-cursors.list` > $(@F)
