## Process this file with automake to produce Makefile.in

scriptdata =
INTLLIBS=-lintl
bin_PROGRAMS = gimppath gimpparse plugdir
#bin_PROGRAMS = testplugin gimppath gimpparse plugshow plugdir

OFILES=../about_dialog.o          ../disp_callbacks.o        ../gimpunit.o \
             ../pattern_select.o \
../actionarea.o            ../docindex.o              ../global_edit.o \
          ../pattern_select_cmds.o \
../airbrush.o              ../docindexif.o            ../gradient.o              ../patterns.o\
../draw_core.o             ../gradient_cmds.o         ../patterns_cmds.o \
../asupsample.o            ../drawable.o              ../gradient_select.o       ../pencil.o\
../batch.o                 ../drawable_cmds.o         ../gradient_select_cmds.o  ../perspective_tool.o\
../bezier_select.o         ../edit_cmds.o             ../guides_cmds.o           ../pixel_processor.o\
../blend.o                 ../edit_selection.o        ../gximage.o               ../pixel_region.o\
../blob.o                  ../ellipse_select.o        ../histogram_tool.o        \
../boundary.o              ../equalize.o              ../histogramwidget.o       ../plug_in_cmds.o\
../brightness_contrast.o   ../eraser.o                ../hue_saturation.o        ../posterize.o\
../brush_edit.o            ../errorconsole.o          ../image_map.o             ../preferences_dialog.o\
../brush_select.o          ../errors.o                ../image_render.o          ../procedural_db.o\
../brush_select_cmds.o     ../file_new_dialog.o       ../info_dialog.o           ../procedural_db_cmds.o\
../brushes_cmds.o          ../fileops.o               ../info_window.o           ../rect_select.o\
../bucket_fill.o           ../fileops_cmds.o          ../ink.o                   ../regex.o\
../buildmenu.o             ../flip_tool.o             ../install.o               ../resize.o\
../by_color_select.o       ../floating_sel.o          ../interface.o             ../rotate_tool.o\
../channel.o               ../floating_sel_cmds.o     ../interface_cmds.o        ../scale.o\
../channel_cmds.o          ../undo_history.o                  ../internal_procs.o        ../scale_tool.o\
../channel_ops.o           ../free_select.o           ../invert.o                ../scroll.o\
../channel_ops_cmds.o      ../fuzzy_select.o          ../iscissors.o             ../selection.o\
../channels_dialog.o       ../gdisplay.o              ../layer.o                 ../session.o\
../clone.o                 ../gdisplay_cmds.o         ../layer_cmds.o            ../shear_tool.o\
../color_area.o            ../gdisplay_ops.o          ../layer_select.o          ../temp_buf.o\
../color_balance.o         ../general.o               ../layers_dialog.o         ../text_tool.o\
../color_cmds.o            ../gimage.o                ../levels.o                ../text_tool_cmds.o\
../color_notebook.o        ../gimage_cmds.o           ../lut_funcs.o             ../threshold.o\
../color_panel.o           ../gimage_mask.o           ../magnify.o               ../tile.o\
../color_picker.o          ../gimage_mask_cmds.o      ../app_procs.o                  ../tile_cache.o\
../color_select.o          ../gimpbrush.o             ../menus.o                 ../tile_manager.o\
../color_transfer.o        ../gimpbrushgenerated.o    ../misc_cmds.o             ../tile_swap.o\
../colormap_dialog.o       ../gimpbrushlist.o         ../module_db.o             ../tips_dialog.o\
../colormaps.o             ../gimpdrawable.o          ../move.o                  ../tool_options.o\
../commands.o              ../gimphistogram.o         ../ops_buttons.o           ../tools.o\
../convert.o               ../gimpimage.o             ../paint_core.o            ../tools_cmds.o\
../convert_cmds.o          ../gimplist.o              ../paint_funcs.o           ../transform_core.o\
../convolve.o              ../gimplut.o               ../paintbrush.o            ../transform_tool.o\
../crop.o                  ../gimpobject.o            ../palette.o               ../undo.o\
../cursorutil.o            ../gimpparasite.o          ../palette_cmds.o          ../undo_cmds.o\
../curves.o                ../gimpprogress.o          ../palette_select.o        ../unit_cmds.o\
../datafiles.o                             ../parasite_cmds.o         ../vector2d.o\
../desaturate.o            ../gimprc_cmds.o           ../parasitelist.o          ../xcf.o\
../devices.o               ../gimpset.o               ../paths_cmds.o\
../dialog_handler.o        ../gimpsignal.o            ../paths_dialog.o \
../path_tool.o ../gimpbrushpipe.o ../gtkwrapbox.o ../gtkhwrapbox.o ../gtkvwrapbox.o \
../gimpdnd.o ../brush_scale.o ../measure.o ../xinput_airbrush.o \
../airbrush_blob.o ../gdisplay_color_ui.o \
../image_new.o ../nav_window.o ../smudge.o ../dodgeburn.o \
../indicator_area.o ../gdisplay_color.o ../gimpcontextpreview.o \
../qmask.o ../context_manager.o ../lc_dialog.o ../gimpcontext.o

testplugin_SOURCES = \
	testplugin.c \
	init.c		
plugshow_SOURCES = \
	plugshow.c
plugdir_SOURCES = \
	plugdir.c
gimppath_SOURCES = \
	gimppath.c	

AM_CPPFLAGS = \
	-DLIBDIR=\""$(gimpplugindir)"\"	\
	-DLOCALEDIR=\""$(localedir)"\"	\
	-DREGEX_MALLOC			\
	@GIMP_THREAD_FLAGS@

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/intl		\
	$(GTK_CFLAGS)			\
	-I$(includedir)\
	-I..

LDADD = $(OFILES) \
	../libgimpim.a \
	$(top_builddir)/libgimp/libgimpi.a	\
	$(GTK_LIBS)				\
	$(GIMP_THREAD_LIBS)			\
	$(INTLLIBS)

plugshow_LDADD = $(OFILES) \
	../libgimpim.a \
	$(top_builddir)/libgimp/libgimpi.a	\
	$(GTK_LIBS)				\
	$(GIMP_THREAD_LIBS)			\
	$(INTLLIBS)
plugdir_LDADD = $(OFILES) \
	../libgimpim.a \
	$(top_builddir)/libgimp/libgimpi.a	\
	$(GTK_LIBS)				\
	$(GIMP_THREAD_LIBS)			\
	$(INTLLIBS)
gimppath_LDADD = $(OFILES) ../plug_in.o \
	../libgimpim.a \
	$(top_builddir)/libgimp/libgimpi.a	\
	$(GTK_LIBS)				\
	$(GIMP_THREAD_LIBS)			\
	$(INTLLIBS)

gimpparse_LDADD = $(OFILES) ../plug_in.o\
	../libgimpim.a \
	$(top_builddir)/libgimp/libgimpi.a	\
	$(GTK_LIBS)				\
	$(GIMP_THREAD_LIBS)			\
	$(INTLLIBS)

DEPS = \
	../libgimpim.a \
	$(top_builddir)/libgimp/libgimpi.a


.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done


