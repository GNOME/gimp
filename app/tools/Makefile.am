## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libapptools.la

libapptools_la_SOURCES = \
##	airbrush.c		\
##	airbrush.h		\
##	airbrush_blob.c		\
##	airbrush_blob.h		\
##	bezier_select.c		\
##	bezier_select.h		\
##	bezier_selectP.h	\
##	blend.c			\
##	blend.h			\
##	blob.c			\
##	blob.h			\
##	brightness_contrast.c	\
##	brightness_contrast.h	\
##	bucket_fill.c		\
##	bucket_fill.h		\
##	by_color_select.c	\
##	by_color_select.h	\
##	clone.c			\
##	clone.h			\
##	color_balance.c		\
##	color_balance.h		\
	color_picker.c		\
	color_picker.h		\
##	convolve.c		\
##	convolve.h		\
##	crop.c			\
##	crop.h			\
##	curves.c		\
##	curves.h		\
##	dodgeburn.c		\
##	dodgeburn.h		\
##	edit_selection.c	\
##	edit_selection.h	\
##	ellipse_select.c	\
##	ellipse_select.h	\
##	eraser.c		\
##	eraser.h		\
##	flip_tool.c		\
##	flip_tool.h		\
##	free_select.c		\
##	free_select.h		\
##	fuzzy_select.c		\
##	fuzzy_select.h		\
	gimptoolinfo.c		\
	gimptoolinfo.h		\
##	histogram_tool.c	\
##	histogram_tool.h	\
##	hue_saturation.c	\
##	hue_saturation.h	\
##	ink.c			\
##	ink.h			\
##	iscissors.c		\
##	iscissors.h		\
##	levels.c		\
##	levels.h		\
##	magnify.c		\
##	magnify.h		\
	measure.c		\
	measure.h		\
	move.c			\
	move.h			\
##	paintbrush.c		\
##	paintbrush.h		\
##	paint_core.c		\
##	paint_core.h		\
##	paint_core_kernels.h	\
	paint_options.c		\
	paint_options.h		\
##	pencil.c		\
##	pencil.h		\
##	perspective_tool.c	\
##	perspective_tool.h	\
##	posterize.c		\
##	posterize.h		\
##	rect_select.c		\
##	rect_select.h		\
##	rect_selectP.h		\
##	rotate_tool.c		\
##	rotate_tool.h		\
##	scale_tool.c		\
##	scale_tool.h		\
	selection_options.c	\
	selection_options.h	\
##	shear_tool.c		\
##	shear_tool.h		\
##	smudge.c		\
##	smudge.h		\
##	text_tool.c		\
##	text_tool.h		\
##	threshold.c		\
##	threshold.h		\
	tool.c			\
	tool.h			\
	tool_manager.c		\
	tool_manager.h		\
	tool_options.c		\
	tool_options.h		\
	tool_options_dialog.c	\
	tool_options_dialog.h	\
	tools.c			
##	transform_core.c	\
##	transform_core.h	\
##	transform_tool.c	\
##	transform_tool.h

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Tools\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
