## Process this file with automake to produce Makefile.in

SUBDIRS = pdb tools

scriptdata =

if HAVE_GLIBC_REGEX
regex_sources =
else
regex_sources = regexrepl.c regexrepl.h
endif

bin_PROGRAMS = gimp

gimp_SOURCES = \
	about_dialog.c		\
	about_dialog.h		\
	app_procs.c		\
	app_procs.h		\
	appenums.h		\
	appenv.h		\
	apptypes.h		\
	asupsample.c		\
	asupsample.h		\
	authors.h		\
	batch.c			\
	batch.h			\
	boundary.c		\
	boundary.h		\
	brush_edit.c		\
	brush_edit.h		\
	brush_header.h		\
	brush_scale.c		\
	brush_scale.h		\
	brush_select.c		\
	brush_select.h		\
	channel.c		\
	channel.h		\
	channels_dialog.c	\
	channels_dialog.h	\
	channel_ops.c		\
	channel_ops.h		\
	color_area.c		\
	color_area.h		\
	color_notebook.c	\
	color_notebook.h	\
	color_panel.c		\
	color_panel.h		\
	color_select.c		\
	color_select.h		\
	color_transfer.c	\
	color_transfer.h	\
	colormaps.c		\
	colormaps.h		\
	colormap_dialog.c	\
	colormap_dialog.h	\
	commands.c		\
	commands.h		\
	context_manager.c	\
	context_manager.h	\
	convert.c		\
	convert.h		\
	cursorutil.c		\
	cursorutil.h		\
	datafiles.c		\
	datafiles.h		\
	desaturate.c		\
	desaturate.h		\
	devices.c		\
	devices.h		\
	dialog_types.h		\
	dialog_handler.c	\
	dialog_handler.h	\
	disp_callbacks.c	\
	disp_callbacks.h	\
	docindex.c		\
	docindex.h		\
	draw_core.c		\
	draw_core.h		\
	drawable.c		\
	drawable.h		\
	drawable_pvt.h		\
	edit_selection.c	\
	edit_selection.h	\
	errorconsole.c		\
	errorconsole.h		\
	errors.c		\
	errors.h		\
	equalize.c		\
	equalize.h		\
	fileops.c		\
	fileops.h		\
	fileopsP.h		\
	floating_sel.c		\
	floating_sel.h		\
	file_new_dialog.c	\
	file_new_dialog.h	\
	fsdither.h		\
	gdisplay.c		\
	gdisplay.h		\
	gdisplay_ops.c		\
	gdisplay_ops.h		\
	general.c		\
	general.h		\
	gimage.c		\
	gimage.h		\
	gimage_mask.c		\
	gimage_mask.h		\
	gimpbrush.c		\
	gimpbrush.h		\
	gimpbrushgenerated.c	\
	gimpbrushgenerated.h	\
	gimpbrushlist.c		\
	gimpbrushlist.h		\
	gimpbrushpipe.c		\
	gimpbrushpipe.h		\
	gimpcontext.c		\
	gimpcontext.h		\
	gimpcontextpreview.c	\
	gimpcontextpreview.h	\
	gimpdnd.c		\
	gimpdnd.h		\
	gimpdrawable.c		\
	gimpdrawable.h		\
	gimphelp.c		\
	gimphelp.h		\
	gimphistogram.c		\
	gimphistogram.h		\
	gimpimage.c		\
	gimpimage.h		\
	gimplist.c		\
	gimplist.h		\
	gimplut.c		\
	gimplut.h		\
	gimpmarshal.c		\
	gimpmarshal.h		\
	gimpobject.c		\
	gimpobject.h		\
	gimppalette.c		\
	gimppalette.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimppreviewcache.h	\
	gimppreviewcache.c	\
	gimpprogress.c		\
	gimpprogress.h		\
	gimprc.c		\
	gimprc.h		\
	gimpset.c		\
	gimpset.h		\
	gimpui.c		\
	gimpui.h		\
	gimpunit.c		\
	global_edit.c		\
	global_edit.h		\
	gradient.c		\
	gradientP.h		\
	gradient_header.h	\
	gradient_select.h	\
	gradient_select.c	\
	gradient.h		\
	gtkwrapbox.c		\
	gtkwrapbox.h		\
	gtkhwrapbox.c		\
	gtkhwrapbox.h		\
	gtkvwrapbox.c		\
	gtkvwrapbox.h		\
	gximage.c		\
	gximage.h		\
	histogramwidget.c	\
	histogramwidget.h	\
	image_map.c		\
	image_map.h		\
	image_new.c		\
	image_new.h		\
	image_render.c		\
	image_render.h		\
	indicator_area.c	\
	indicator_area.h	\
	info_dialog.c		\
	info_dialog.h		\
	info_window.c		\
	info_window.h		\
	interface.c		\
	interface.h		\
	invert.c		\
	invert.h		\
	layer.c			\
	layer.h			\
	layer_select.c		\
	layer_select.h		\
	layers_dialog.c		\
	layers_dialog.h		\
	layers_dialogP.h	\
	lc_dialog.c		\
	lc_dialog.h		\
	lc_dialogP.h		\
	lut_funcs.c		\
	lut_funcs.h		\
	main.c			\
	marching_ants.h		\
	menus.c			\
	menus.h			\
	module_db.c		\
	module_db.h		\
	nav_window.c		\
	nav_window.h		\
	ops_buttons.c		\
	ops_buttons.h		\
	palette.c		\
	palette.h		\
	palette_import.c	\
	palette_import.h	\
	palette_select.c	\
	palette_select.h	\
	paletteP.h		\
	paint_funcs.c		\
	paint_funcs.h		\
	parasitelist.c		\
	parasitelist.h		\
	path.c			\
	path.h			\
	pathP.h			\
	path_transform.h	\
	paths_dialog.c		\
	paths_dialog.h		\
	paths_dialogP.h		\
	pattern_header.h	\
	pattern_select.c	\
	pattern_select.h	\
	patterns.c		\
	patterns.h		\
	pixel_processor.c	\
	pixel_processor.h	\
	pixel_region.c		\
	pixel_region.h		\
	pixmaps.h		\
	pixmaps2.h		\
	plug_in.c		\
	plug_in.h		\
	preferences_dialog.c	\
	preferences_dialog.h	\
	qmask.c			\
	qmask.h			\
	resolution_calibrate.c	\
	resolution_calibrate.h	\
	resize.c		\
	resize.h		\
	scale.c			\
	scale.h			\
	scan_convert.c		\
	scan_convert.h		\
	scroll.c		\
	scroll.h		\
	selection.c		\
	selection.h		\
	selection_options.h	\
	session.h		\
	session.c		\
	temp_buf.c		\
	temp_buf.h		\
	tile.c			\
	tile.h			\
	tile_pvt.h		\
	tile_cache.c		\
	tile_cache.h		\
	tile_manager.c		\
	tile_manager.h		\
	tile_manager_pvt.h	\
	tile_swap.c		\
	tile_swap.h		\
	tips_dialog.c		\
	tips_dialog.h		\
	undo.c			\
	undo.h			\
	undo_history.c		\
	undo_types.h		\
	unitrc.h		\
	user_install.c		\
	user_install.h		\
	wilber.h		\
	xcf.c			\
	xcf.h			\
	$(regex_sources)

EXTRA_DIST = \
	makefile.mingw		\
	makefile.mingw.in	\
	makefile.msc		\
	gimp.rc			\
	gimp.sym		\
	wilber.ico


gimp_LDFLAGS = -export-dynamic -export-symbols $(srcdir)/gimp.sym

AM_CPPFLAGS = \
	-DLIBDIR=\""$(gimpplugindir)"\"	\
	-DLOCALEDIR=\""$(localedir)"\"	\
	-DREGEX_MALLOC			\
	-DG_LOG_DOMAIN=\"Gimp\"		\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

gimp_LDADD = \
	pdb/libapppdb.la			\
	tools/libapptools.la			\
	$(top_builddir)/libgimp/libgimpi.a	\
	$(GTK_LIBS)				\
	$(GIMP_THREAD_LIBS)			\
	$(GIMP_MP_LIBS)				\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/app/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=app/$@ CONFIG_HEADERS= $(SHELL) ./config.status
