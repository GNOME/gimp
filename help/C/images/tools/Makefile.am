## Process this file with automake to produce Makefile.in

helpdatadir = $(gimpdatadir)/help/C/images/tools

helpdata_DATA = \
		tool_airbrush.png \
		tool_apartheid.png \
		tool_bezier.png \
		tool_blend.png \
		tool_burndodge.png \
		tool_convolver.png \
		tool_crop.png \
		tool_dodgeburn.png \
		tool_ellipse_sel.png \
		tool_eraser.png \
		tool_fill.png \
		tool_flip.png \
		tool_free_sel.png \
		tool_fuzzy_sel.png \
		tool_gradient.png \
		tool_ink.png \
		tool_measure.png \
		tool_move.png \
		tool-opt-increment.png \
		tool-opt-nonincrement.png \
		tool_paintbrush.png \
		tool_pencil.png \
		tool_picker.png \
		tool_rect_sel.png \
		tool_scissors.png \
		tool_smudge.png \
		tool_smuge.png \
		tool_text.png \
		tool_transform.png \
		tool_zoom.png

EXTRA_DIST = $(helpdata_DATA)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

