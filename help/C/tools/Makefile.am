## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/tools

helpdata_DATA = \
        airbrush.html \
        bezier_select.html \
        blend.html \
        brightness_contrast.html \
        bucket_fill.html \
        by_color_select.html \
        clone.html \
        color_balance.html \
        color_picker.html \
        convolve.html \
        crop.html \
        curves.html \
        dodgeburn.html \
        ellipse_select.html \
        eraser.html \
        flip.html \
        free_select.html \
        fuzzy_select.html \
        histogram.html \
        hue_saturation.html \
        index.html \
        ink.html \
        intelligent_scissors.html \
        levels.html \
        magnify.html \
        measure.html \
        move.html \
        paintbrush.html \
        path.html \
        pencil.html \
        posterize.html \
        rect_select.html \
        smudge.html \
        text.html \
        threshold.html \
        transform.html \
        transform_perspective.html \
        transform_rotate.html \
        transform_scale.html \
        transform_shear.html \
        xinput_airbrush.html 

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
