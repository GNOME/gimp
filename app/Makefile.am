## Process this file with automake to produce Makefile.in

SUBDIRS = paint-funcs pdb tools widgets gui

scriptdata =

if HAVE_GLIBC_REGEX
regex_sources =
else
regex_sources = \
	regexrepl.c 		\
	regexrepl.h
endif

if HAVE_ASM_MMX
mmx_sources = \
	detect_mmx.S
else
mmx_sources =
endif

bin_PROGRAMS = gimp

gimp_SOURCES = \
##
## gui stuff that needs to go to libgimp/, gui/ or /dev/null
##
	colormaps.c		\
	colormaps.h		\
	cursorutil.c		\
	cursorutil.h		\
	devices.c		\
	devices.h		\
	dialog_handler.c	\
	dialog_handler.h	\
	docindex.c		\
	docindex.h		\
	errorconsole.c		\
	errorconsole.h		\
	gimpdnd.c		\
	gimpdnd.h		\
	gimpprogress.c		\
	gimpprogress.h		\
	gimpui.c		\
	gimpui.h		\
	lc_dialog.c		\
	lc_dialog.h		\
	nav_window.c		\
	nav_window.h		\
	ops_buttons.c		\
	ops_buttons.h		\
	paths_dialogP.h		\
	qmask.c			\
	qmask.h			\
	resize.c		\
	resize.h		\
	resolution_calibrate.c	\
	resolution_calibrate.h	\
	undo_history.c		\
	undo_history.h		\
##
## display stuff
##
	disp_callbacks.c	\
	disp_callbacks.h	\
	gdisplay.c		\
	gdisplay.h		\
	gdisplay_ops.c		\
	gdisplay_ops.h		\
	gximage.c		\
	gximage.h		\
	image_render.c		\
	image_render.h		\
	interface.c		\
	interface.h		\
	scale.c			\
	scale.h			\
	scroll.c		\
	scroll.h		\
	selection.c		\
	selection.h		\
	marching_ants.h		\
##
## stuff that needs to be split up in core / ui files
##
	global_edit.c		\
	global_edit.h		\
	image_map.c		\
	image_map.h		\
	module_db.c		\
	module_db.h		\
	user_install.c		\
	user_install.h		\
##
## base stuff that does not depend on anything else in app/
##
	gimpcontainer.c		\
	gimpcontainer.h		\
	gimpdata.c		\
	gimpdata.h		\
	gimpdatafactory.c	\
	gimpdatafactory.h	\
	gimpdatalist.c		\
	gimpdatalist.h		\
	gimplist.c		\
	gimplist.h		\
	gimpobject.c		\
	gimpobject.h		\
##
## the core object system
##
	gimpbrush.c			\
	gimpbrush.h			\
	gimpbrush-header.h		\
	gimpbrushgenerated.c		\
	gimpbrushgenerated.h		\
	gimpbrushpipe.c			\
	gimpbrushpipe.h			\
	gimpchannel.c			\
	gimpchannel.h			\
	gimpcontext.c			\
	gimpcontext.h			\
	gimpdrawable.c			\
	gimpdrawable.h			\
	gimpdrawable-desaturate.c	\
	gimpdrawable-desaturate.h	\
	gimpdrawable-equalize.c		\
	gimpdrawable-equalize.h		\
	gimpdrawable-invert.c		\
	gimpdrawable-invert.h		\
	gimpdrawable-offset.c		\
	gimpdrawable-offset.h		\
	gimpdrawable-preview.c		\
	gimpdrawable-preview.h		\
	gimpgradient.c			\
	gimpgradient.h			\
	gimpimage.c			\
	gimpimage.h			\
	gimpimage-convert.c		\
	gimpimage-convert.h		\
	gimpimage-duplicate.c		\
	gimpimage-duplicate.h		\
	gimpimage-undo.c		\
	gimpimage-undo.h		\
	gimplayer.c			\
	gimplayer.h			\
	gimplayermask.c			\
	gimplayermask.h			\
	gimpmarshal.c			\
	gimpmarshal.h			\
	gimppattern.c			\
	gimppattern.h			\
	gimppattern-header.h		\
	gimppalette.c			\
	gimppalette.h			\
	gimppalette-import.c		\
	gimppalette-import.h		\
	gimpundo.c			\
	gimpundo.h			\
	gimpundostack.c			\
	gimpundostack.h			\
	gimpviewable.c			\
	gimpviewable.h			\
##
## stuff that needs to be sorted into yet undecided subsystems, split up,
## removed or otherwise tortured
##
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
	brush_scale.c		\
	brush_scale.h		\
	color_transfer.c	\
	color_transfer.h	\
	context_manager.c	\
	context_manager.h	\
	datafiles.c		\
	datafiles.h		\
	drawable.c		\
	drawable.h		\
	errors.c		\
	errors.h		\
	file-open.c		\
	file-open.h		\
	file-save.c		\
	file-save.h		\
	file-utils.h		\
	file-utils.c		\
	floating_sel.c		\
	floating_sel.h		\
	fsdither.h		\
	general.c		\
	general.h		\
	gimage.c		\
	gimage.h		\
	gimage_mask.c		\
	gimage_mask.h		\
	gimphelp.c		\
	gimphelp.h		\
	gimphistogram.c		\
	gimphistogram.h		\
	gimplut.c		\
	gimplut.h		\
	gimpparasite.c		\
	gimpparasite.h		\
	gimppreviewcache.h	\
	gimppreviewcache.c	\
	gimprc.c		\
	gimprc.h		\
	gimpunit.c		\
	image_new.c		\
	image_new.h		\
	libgimp_glue.c		\
	libgimp_glue.h		\
	lut_funcs.c		\
	lut_funcs.h		\
	main.c			\
	parasitelist.c		\
	parasitelist.h		\
	path.c			\
	path.h			\
	pathP.h			\
	path_transform.h	\
	path_curves.h		\
	path_curves.c		\
	path_bezier.h		\
	path_bezier.c		\
	pixel_processor.c	\
	pixel_processor.h	\
	pixel_region.c		\
	pixel_region.h		\
	pixel_surround.c	\
	pixel_surround.h	\
	plug_in.c		\
	plug_in.h		\
	scan_convert.c		\
	scan_convert.h		\
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
	undo.c			\
	undo.h			\
	undo_types.h		\
	unitrc.h		\
	xcf.c			\
	xcf.h			\
	$(regex_sources)	\
	$(mmx_sources)		\
	\
	pixmaps2.h

EXTRA_DIST = \
	makefile.mingw			\
	makefile.mingw.in		\
	makefile.msc			\
	gimp.rc				\
	gimp.sym			\
	wilber.ico			\
	arch/i386/mmx/detect_mmx.S	\
	arch/i386/mmx/paint_funcs_mmx.S


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
	gui/libappgui.la						\
	paint-funcs/libapppaint-funcs.la				\
	pdb/libapppdb.la						\
	tools/libapptools.la						\
	widgets/libappwidgets.la					\
	$(top_builddir)/libgimpcolor/libgimpcolor-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpmath/libgimpmath-$(LT_RELEASE).la	\
	$(top_builddir)/libgimpwidgets/libgimpwidgets-$(LT_RELEASE).la	\
	$(top_builddir)/libgimp/libgimpi.a				\
	$(GTK_LIBS)							\
	$(GIMP_THREAD_LIBS)						\
	$(GIMP_MP_LIBS)							\
	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/app/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=app/$@ CONFIG_HEADERS= $(SHELL) ./config.status
