## Process this file with automake to produce Makefile.in

CURSOR_IMAGES = \
	cursor-bad.png			\
	cursor-color-picker.png		\
	cursor-crosshair.png		\
	cursor-crosshair-small.png	\
	cursor-mouse.png		\
	cursor-zoom.png			\
	\
	modifier-anchor.png		\
	modifier-background.png		\
	modifier-control.png		\
	modifier-foreground.png		\
	modifier-intersect.png		\
	modifier-minus.png		\
	modifier-move.png		\
	modifier-pattern.png		\
	modifier-plus.png		\
	modifier-resize.png		\
	\
	tool-airbrush.png		\
	tool-blend.png			\
	tool-blur.png			\
	tool-bucket-fill.png		\
	tool-burn.png			\
	tool-clone.png			\
	tool-color-picker.png		\
	tool-crop.png			\
	tool-dodge.png			\
	tool-ellipse-select.png		\
	tool-eraser.png			\
	tool-flip-horizontal.png	\
	tool-flip-vertical.png		\
	tool-free-select.png		\
	tool-fuzzy-select.png		\
	tool-hand.png			\
	tool-ink.png			\
	tool-iscissors.png		\
	tool-measure.png		\
	tool-move.png			\
	tool-paintbrush.png		\
	tool-paths.png			\
	tool-pencil.png			\
	tool-perspective.png		\
	tool-rect-select.png		\
	tool-resize.png			\
	tool-rotate.png			\
	tool-shear.png			\
	tool-smudge.png			\
	tool-text.png			\
	tool-zoom.png

EXTRA_DIST = \
	$(CURSOR_IMAGES)		\
	gimp-tool-cursors.xcf		\
	\
	mouse.xbm			\
	mouse_mask.xbm			\
	crosshair.xbm			\
	crosshair_mask.xbm		\
	crosshair_small.xbm		\
	crosshair_small_mask.xbm	\
	bad.xbm				\
	bad_mask.xbm			\
	zoom.xbm			\
	zoom_mask.xbm			\
	dropper.xbm			\
	dropper_mask.xbm		\
	\
	plus.xbm			\
	plus_mask.xbm			\
	minus.xbm			\
	minus_mask.xbm			\
	intersect.xbm			\
	intersect_mask.xbm		\
	move.xbm			\
	move_mask.xbm			\
	resize.xbm			\
	resize_mask.xbm			\
	control.xbm			\
	control_mask.xbm		\
	anchor.xbm			\
	anchor_mask.xbm			\
	foreground.xbm			\
	foreground_mask.xbm		\
	background.xbm			\
	background_mask.xbm		\
	pattern.xbm			\
	pattern_mask.xbm		\
	\
	rect_select_small.xbm		\
	rect_select_small_mask.xbm	\
	ellipse_select_small.xbm	\
	ellipse_select_small_mask.xbm	\
	free_select_small.xbm		\
	free_select_small_mask.xbm	\
	fuzzy_select_small.xbm		\
	fuzzy_select_small_mask.xbm	\
	bezier_select_small.xbm		\
	bezier_select_small_mask.xbm	\
	scissors_small.xbm		\
	scissors_small_mask.xbm		\
	move_small.xbm			\
	move_small_mask.xbm		\
	zoom_small.xbm			\
	zoom_small_mask.xbm		\
	crop_small.xbm			\
	crop_small_mask.xbm		\
	resize_small.xbm		\
	resize_small_mask.xbm		\
	rotate_small.xbm		\
	rotate_small_mask.xbm		\
	shear_small.xbm			\
	shear_small_mask.xbm		\
	perspective_small.xbm		\
	perspective_small_mask.xbm	\
	flip_horizontal_small.xbm	\
	flip_horizontal_small_mask.xbm	\
	flip_vertical_small.xbm		\
	flip_vertical_small_mask.xbm	\
	text_small.xbm			\
	text_small_mask.xbm		\
	dropper_small.xbm		\
	dropper_small_mask.xbm		\
	bucket_fill_small.xbm		\
	bucket_fill_small_mask.xbm	\
	blend_small.xbm			\
	blend_small_mask.xbm		\
	pencil_small.xbm		\
	pencil_small_mask.xbm		\
	paintbrush_small.xbm		\
	paintbrush_small_mask.xbm	\
	eraser_small.xbm		\
	eraser_small_mask.xbm		\
	airbrush_small.xbm		\
	airbrush_small_mask.xbm		\
	clone_small.xbm			\
	clone_small_mask.xbm		\
	blur_small.xbm			\
	blur_small_mask.xbm		\
	ink_small.xbm			\
	ink_small_mask.xbm		\
	dodge_small.xbm			\
	dodge_small_mask.xbm		\
	burn_small.xbm			\
	burn_small_mask.xbm		\
	smudge_small.xbm		\
	smudge_small_mask.xbm		\
	measure_small.xbm		\
	measure_small_mask.xbm		\
	hand_small.xbm			\
	hand_small_mask.xbm

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
