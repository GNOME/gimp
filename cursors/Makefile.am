## Process this file with automake to produce Makefile.in

CURSOR_IMAGES = \
	cursor-bad.png				\
	cursor-bad-bw.png			\
	cursor-color-picker.png			\
	cursor-color-picker-bw.png		\
	cursor-corner-bottom-left.png		\
	cursor-corner-bottom-left-bw.png	\
	cursor-corner-bottom-right.png		\
	cursor-corner-bottom-right-bw.png	\
	cursor-corner-top-left.png		\
	cursor-corner-top-left-bw.png		\
	cursor-corner-top-right.png		\
	cursor-corner-top-right-bw.png		\
	cursor-crosshair.png			\
	cursor-crosshair-bw.png			\
	cursor-crosshair-small.png		\
	cursor-crosshair-small-bw.png		\
	cursor-mouse.png			\
	cursor-mouse-bw.png			\
	cursor-move.png				\
	cursor-move-bw.png			\
	cursor-none.png				\
	cursor-none-bw.png			\
	cursor-side-bottom.png			\
	cursor-side-bottom-bw.png		\
	cursor-side-left.png			\
	cursor-side-left-bw.png			\
	cursor-side-right.png			\
	cursor-side-right-bw.png		\
	cursor-side-top.png			\
	cursor-side-top-bw.png			\
	cursor-zoom.png				\
	cursor-zoom-bw.png			\
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
	tool-airbrush-bw.png			\
	tool-blend.png				\
	tool-blend-bw.png			\
	tool-blur.png				\
	tool-blur-bw.png			\
	tool-bucket-fill.png			\
	tool-bucket-fill-bw.png			\
	tool-burn.png				\
	tool-burn-bw.png			\
	tool-clone.png				\
	tool-clone-bw.png			\
	tool-color-picker.png			\
	tool-color-picker-bw.png		\
	tool-crop.png				\
	tool-crop-bw.png			\
	tool-dodge.png				\
	tool-dodge-bw.png			\
	tool-ellipse-select.png			\
	tool-ellipse-select-bw.png		\
	tool-eraser.png				\
	tool-eraser-bw.png			\
	tool-flip-horizontal.png		\
	tool-flip-horizontal-bw.png		\
	tool-flip-vertical.png			\
	tool-flip-vertical-bw.png		\
	tool-free-select.png			\
	tool-free-select-bw.png			\
	tool-fuzzy-select.png			\
	tool-fuzzy-select-bw.png		\
	tool-hand.png				\
	tool-hand-bw.png			\
	tool-heal.png				\
	tool-heal-bw.png			\
	tool-ink.png				\
	tool-ink-bw.png				\
	tool-iscissors.png			\
	tool-iscissors-bw.png			\
	tool-measure.png			\
	tool-measure-bw.png			\
	tool-move.png				\
	tool-move-bw.png			\
	tool-paintbrush.png			\
	tool-paintbrush-bw.png			\
	tool-paths.png				\
	tool-paths-bw.png			\
	tool-paths-anchor.png			\
	tool-paths-anchor-bw.png		\
	tool-paths-control.png			\
	tool-paths-control-bw.png		\
	tool-paths-segment.png			\
	tool-paths-segment-bw.png		\
	tool-pencil.png				\
	tool-pencil-bw.png			\
	tool-perspective.png			\
	tool-perspective-bw.png			\
	tool-polygon-select.png			\
	tool-polygon-select-bw.png		\
	tool-rect-select.png			\
	tool-rect-select-bw.png			\
	tool-resize.png				\
	tool-resize-bw.png			\
	tool-rotate.png				\
	tool-rotate-bw.png			\
	tool-shear.png				\
	tool-shear-bw.png			\
	tool-smudge.png				\
	tool-smudge-bw.png			\
	tool-text.png				\
	tool-text-bw.png			\
	tool-zoom.png				\
	tool-zoom-bw.png

EXTRA_DIST = \
	$(CURSOR_IMAGES)			\
	gimp-tool-cursors.xcf			\
	makefile.msc

noinst_DATA = gimp-tool-cursors.h

CLEANFILES = gimp-tool-cursors.h tool-cursors.list

tool-cursors.list: $(CURSOR_IMAGES) Makefile.am
	( rm -f $@; \
	  for image in $(CURSOR_IMAGES); do \
	    echo $$image | \
	      sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/gimp-tool-cursors.h: tool-cursors.list
	$(GDK_PIXBUF_CSOURCE) --raw --build-list `cat tool-cursors.list` > $(@F)
