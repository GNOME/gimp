/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include "appenv.h"
#include "about_dialog.h"
#include "app_procs.h"
#include "brightness_contrast.h"
#include "gimpbrushlist.h"
#include "by_color_select.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "colormap_dialog.i.h"
#include "color_area.h"
#include "color_balance.h"
#include "commands.h"
#include "convert.h"
#include "curves.h"
#include "desaturate.h"
#include "devices.h"
#include "channel_ops.h"
#include "drawable.h"
#include "equalize.h"
#include "errorconsole.h"
#include "fileops.h"
#include "floating_sel.h"
#include "gdisplay_ops.h"
#include "general.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "global_edit.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "image_render.h"
#include "info_window.h"
#include "interface.h"
#include "invert.h"
#include "layers_dialog.h"
#include "layer_select.h"
#include "levels.h"
#include "module_db.h"
#include "palette.h"
#include "patterns.h"
#include "plug_in.h"
#include "posterize.h"
#include "resize.h"
#include "scale.h"
#include "threshold.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

typedef struct
{
  Resize *    resize;
  GimpImage*  gimage;
} ImageResize;

/*  external functions  */
extern void layers_dialog_layer_merge_query (GImage *, int);


/*  local functions  */
static void   image_resize_callback (GtkWidget *, gpointer);
static void   image_scale_callback (GtkWidget *, gpointer);
static void   image_cancel_callback (GtkWidget *, gpointer);
static gint   image_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void   gimage_mask_feather_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_border_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_grow_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_shrink_callback (GtkWidget *, gpointer, gpointer);

/*  local variables  */
static double selection_feather_radius = 5.0;
static int    selection_border_radius  = 5;
static int    selection_grow_pixels    = 1;
static int    selection_shrink_pixels  = 1;


void
file_open_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_open_callback (widget, client_data);
}

void
file_save_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_save_callback (widget, client_data);
}

void
file_save_as_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  file_save_as_callback (widget, client_data);
}

void
file_revert_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  file_revert_callback (widget, client_data);
}




void
file_close_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_close_window (gdisp, FALSE);
}

void
file_quit_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  app_exit (0);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_cut (gdisp);
}

void
edit_copy_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_copy (gdisp);
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_paste (gdisp, 0);
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  global_edit_paste (gdisp, 1);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  edit_clear (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  edit_fill (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_stroke (gdisp->gimage, gimage_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}

void
edit_undo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  undo_pop (gdisp->gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  undo_redo (gdisp->gimage);
}

void
edit_named_cut_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_cut (gdisp);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_copy (gdisp);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  named_edit_paste (gdisp);
}

void
select_toggle_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  selection_hide (gdisp->select, (void *) gdisp);
  gdisplays_flush ();
}

void
select_invert_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_invert (gdisp->gimage);
  gdisplays_flush ();
}

void
select_all_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_all (gdisp->gimage);
  gdisplays_flush ();
}

void
select_none_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_none (gdisp->gimage);
  gdisplays_flush ();
}

void
select_float_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_float (gdisp->gimage, gimage_active_drawable (gdisp->gimage), 0, 0);
  gdisplays_flush ();
}

void
select_sharpen_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_sharpen (gdisp->gimage);
  gdisplays_flush ();
}

void
select_border_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  query_size_box (_("Border Selection"), _("Border selection by:"),
		  selection_border_radius, 1, 32767, 0,
		  gdisp->gimage->unit,
		  MIN (gdisp->gimage->xresolution,
		       gdisp->gimage->yresolution),
		  gdisp->dot_for_dot,
		  GTK_OBJECT (gdisp->gimage), "destroy",
		  gimage_mask_border_callback, gdisp->gimage);
}

void
select_feather_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  query_size_box (_("Feather Selection"), _("Feather selection by:"),
		  selection_feather_radius, 0, 32767, 3,
		  gdisp->gimage->unit,
		  MIN (gdisp->gimage->xresolution,
		       gdisp->gimage->yresolution),
		  gdisp->dot_for_dot,
		  GTK_OBJECT (gdisp->gimage), "destroy",
		  gimage_mask_feather_callback, gdisp->gimage);
}

void
select_grow_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  query_size_box (_("Grow Selection"), _("Grow selection by:"),
		  selection_grow_pixels, 1, 32767, 0,
		  gdisp->gimage->unit,
		  MIN (gdisp->gimage->xresolution,
		       gdisp->gimage->yresolution),
		  gdisp->dot_for_dot,
		  GTK_OBJECT (gdisp->gimage), "destroy",
		  gimage_mask_grow_callback, gdisp->gimage);
}

void
select_shrink_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  query_size_box (_("Shrink Selection"), _("Shrink selection by:"),
		  selection_shrink_pixels, 1, 32767, 0,
		  gdisp->gimage->unit,
		  MIN (gdisp->gimage->xresolution,
		       gdisp->gimage->yresolution),
		  gdisp->dot_for_dot,
		  GTK_OBJECT (gdisp->gimage), "destroy",
		  gimage_mask_shrink_callback, gdisp->gimage);
}

void
select_save_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  gimage_mask_save (gdisp->gimage);
  gdisplays_flush ();
}

void
view_dot_for_dot_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_set_dot_for_dot (gdisp, GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_zoomin_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, ZOOMIN);
}

void
view_zoomout_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, ZOOMOUT);
}

static void
view_zoom_val (GtkWidget *widget,
	       gpointer   client_data,
	       int        val)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  change_scale (gdisp, val);
}

void
view_zoom_16_1_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  view_zoom_val (widget, client_data, 1601);
}

void
view_zoom_8_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 801);
}

void
view_zoom_4_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 401);
}

void
view_zoom_2_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 201);
}

void
view_zoom_1_1_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 101);
}

void
view_zoom_1_2_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 102);
}

void
view_zoom_1_4_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 104);
}

void
view_zoom_1_8_callback (GtkWidget *widget,
			gpointer   client_data)
{
  view_zoom_val (widget, client_data, 108);
}

void
view_zoom_1_16_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  view_zoom_val (widget, client_data, 116);
}

void
view_window_info_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  if (! gdisp->window_info_dialog)
    gdisp->window_info_dialog = info_window_create ((void *) gdisp);

  info_dialog_popup (gdisp->window_info_dialog);
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  /* This routine use promiscuous knowledge of gtk internals
   *  in order to hide and show the rulers "smoothly". This
   *  is kludgy and a hack and may break if gtk is changed
   *  internally.
   */

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_unmap (gdisp->origin);
	  gtk_widget_unmap (gdisp->hrule);
	  gtk_widget_unmap (gdisp->vrule);

	  GTK_WIDGET_UNSET_FLAGS (gdisp->origin, GTK_VISIBLE);
	  GTK_WIDGET_UNSET_FLAGS (gdisp->hrule, GTK_VISIBLE);
	  GTK_WIDGET_UNSET_FLAGS (gdisp->vrule, GTK_VISIBLE);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_map (gdisp->origin);
	  gtk_widget_map (gdisp->hrule);
	  gtk_widget_map (gdisp->vrule);

	  GTK_WIDGET_SET_FLAGS (gdisp->origin, GTK_VISIBLE);
	  GTK_WIDGET_SET_FLAGS (gdisp->hrule, GTK_VISIBLE);
	  GTK_WIDGET_SET_FLAGS (gdisp->vrule, GTK_VISIBLE);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;
  int old_val;

  gdisp = gdisplay_active ();

  old_val = gdisp->draw_guides;
  gdisp->draw_guides = GTK_CHECK_MENU_ITEM (widget)->active;

  if ((old_val != gdisp->draw_guides) && gdisp->gimage->guides)
    {
      gdisplay_expose_full (gdisp);
      gdisplays_flush ();
    }
}

void
view_snap_to_guides_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;
}

void
view_toggle_statusbar_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->statusarea))
	gtk_widget_hide (gdisp->statusarea);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gdisp->statusarea))
	gtk_widget_show (gdisp->statusarea);
    }
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  gdisplay_new_view (gdisp);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  if (gdisp)
    shrink_wrap_display (gdisp);
}

void
image_equalize_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_equalize ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
image_invert_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_invert ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
image_desaturate_cmd_callback (GtkWidget *widget,
			       gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  image_desaturate ((void *) gdisp->gimage);
  gdisplays_flush ();
}

void
channel_ops_duplicate_cmd_callback (GtkWidget *widget,
				    gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  channel_ops_duplicate ((void *) gdisp->gimage);
}

void
channel_ops_offset_cmd_callback (GtkWidget *widget,
				 gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  channel_ops_offset ((void *) gdisp->gimage);
}

void
image_convert_rgb_cmd_callback (GtkWidget *widget,
				gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_rgb ((void *) gdisp->gimage);
}

void
image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_grayscale ((void *) gdisp->gimage);
}

void
image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();
  convert_to_indexed ((void *) gdisp->gimage);
}

void
image_resize_cmd_callback (GtkWidget *widget,
			   gpointer client_data)
{
  GDisplay * gdisp;
  GimpImage * gimage;
  ImageResize *image_resize;

  gdisp = gdisplay_active ();
  g_return_if_fail (gdisp != NULL);

  gimage = gdisp->gimage;

  /*  the ImageResize structure  */
  image_resize = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_resize->gimage = gimage;
  image_resize->resize = resize_widget_new (ResizeWidget,
					    ResizeImage,
					    GTK_OBJECT (gimage),
					    gimage->width,
					    gimage->height,
					    gimage->xresolution,
					    gimage->yresolution,
					    gimage->unit,
					    gdisp->dot_for_dot,
					    image_resize_callback,
					    image_cancel_callback,
					    image_delete_callback,
					    image_resize);

  gtk_widget_show (image_resize->resize->resize_shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer client_data)
{
  GDisplay * gdisp;
  GimpImage * gimage;
  ImageResize *image_scale;

  gdisp = gdisplay_active ();
  g_return_if_fail (gdisp != NULL);

  gimage = gdisp->gimage;

  /*  the ImageResize structure  */
  image_scale = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_scale->gimage = gimage;
  image_scale->resize = resize_widget_new (ScaleWidget,
					   ResizeImage,
					   GTK_OBJECT (gimage),
					   gimage->width,
					   gimage->height,
					   gimage->xresolution,
					   gimage->yresolution,
					   gimage->unit,
					   gdisp->dot_for_dot,
					   image_scale_callback,
					   image_cancel_callback,
					   image_delete_callback,
					   image_scale);

  gtk_widget_show (image_scale->resize->resize_shell);
}

void
layers_raise_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_raise_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_lower_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_lower_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_anchor_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  floating_sel_anchor (gimage_get_active_layer (gdisp->gimage));
  gdisplays_flush ();
}

void
layers_merge_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  layers_dialog_layer_merge_query (gdisp->gimage, TRUE);
}

void
layers_flatten_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_flatten (gdisp->gimage);
  gdisplays_flush ();
}

void
layers_alpha_select_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_layer_alpha (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_mask_select_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  gimage_mask_layer_mask (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_add_alpha_channel_cmd_callback (GtkWidget *widget,
				       gpointer   client_data)
{
  GDisplay * gdisp;

  gdisp = gdisplay_active ();

  layer_add_alpha ( gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
tools_default_colors_cmd_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  palette_set_default_colors ();
}

void
tools_swap_colors_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  palette_swap_colors ();
}

void
tools_select_cmd_callback (GtkWidget           *widget,
			   gpointer             callback_data,
			   guint                callback_action)
{
  GDisplay * gdisp;

  if (!tool_info[callback_action].init_func)
    {
      /*  Activate the approriate widget  */
      gtk_widget_activate (tool_info[callback_action].tool_widget);
    }
  else 
    {
      /* if the tool_info has an init_func */
      gdisp = gdisplay_active ();
     
      gtk_widget_activate (tool_info[callback_action].tool_widget);
      
      (* tool_info[callback_action].init_func) (gdisp);
    }
      
  gdisp = gdisplay_active ();

  active_tool->drawable = gimage_active_drawable (gdisp->gimage);
}

void
filters_repeat_cmd_callback (GtkWidget           *widget,
			     gpointer             callback_data,
			     guint                callback_action)
{
  plug_in_repeat (callback_action);
}

void
dialogs_brushes_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  create_brush_dialog ();
}

void
dialogs_patterns_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  create_pattern_dialog ();
}

void
dialogs_palette_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  palette_create ();
}

void
dialogs_gradient_editor_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  grad_create_gradient_editor ();
} /* dialogs_gradient_editor_cmd_callback */

void
dialogs_lc_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  if (gdisp == NULL) 
    lc_dialog_create (NULL);
  else
    lc_dialog_create (gdisp->gimage);
}

static void
cmap_dlg_sel_cb(ColormapDialog* dlg, gpointer user_data)
{
  guchar* c;

  GimpImage* img = colormap_dialog_image(dlg);
  c = &img->cmap[colormap_dialog_col_index(dlg) * 3];
  if(active_color == FOREGROUND)
    palette_set_foreground (c[0], c[1], c[2]);
  else if(active_color == BACKGROUND)
    palette_set_background (c[0], c[1], c[2]);
}

void
dialogs_indexed_palette_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  static ColormapDialog* cmap_dlg;
  if(!cmap_dlg){
    cmap_dlg = colormap_dialog_create (image_context);
    colormap_dialog_connect_selected(cmap_dlg_sel_cb, NULL,
				     cmap_dlg);
  }
  gtk_widget_show(GTK_WIDGET(cmap_dlg));
}

void
dialogs_tools_options_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  tools_options_dialog_show ();
}

void
dialogs_input_devices_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  create_input_dialog ();
}

void
dialogs_device_status_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  create_device_status ();
}

void
dialogs_error_console_cmd_callback (GtkWidget *widget,
				    gpointer   client_data) 
{
  error_console_add (NULL);
}

void
about_dialog_cmd_callback (GtkWidget *widget,
                           gpointer   client_data)
{
  about_dialog_create (FALSE);
}

void
tips_dialog_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  tips_dialog_create ();
}

void
dialogs_module_browser_cmd_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  GtkWidget *w;

  w = module_db_browser_new ();
  gtk_widget_show (w);
}


/*********************/
/*  Local functions  */
/*********************/

static void
image_resize_callback (GtkWidget *w,
		       gpointer   client_data)
{
  ImageResize *image_resize;
  GImage *gimage;

  image_resize = (ImageResize *) client_data;

  if ((gimage = image_resize->gimage) != NULL)
    {
      if (image_resize->resize->width > 0 &&
	  image_resize->resize->height > 0) 
	{
	  gimage_resize (gimage,
			 image_resize->resize->width,
			 image_resize->resize->height,
			 image_resize->resize->offset_x,
			 image_resize->resize->offset_y);
	  gdisplays_flush ();
	}
      else 
	{
	  g_message (_("Resize Error: Both width and height must be greater than zero."));
	  return;
	}
    }

  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
image_scale_callback (GtkWidget *w,
		      gpointer   client_data)
{
  ImageResize *image_scale;
  GImage *gimage;
  gboolean flush = FALSE;

  image_scale = (ImageResize *) client_data;

  if ((gimage = image_scale->gimage) != NULL)
    {
      if (image_scale->resize->resolution_x != gimage->xresolution ||
	  image_scale->resize->resolution_y != gimage->yresolution)
	{
	  gimage_set_resolution (gimage,
				 image_scale->resize->resolution_x,
				 image_scale->resize->resolution_y);
	  flush = TRUE;
	}

      if (image_scale->resize->unit != gimage->unit)
	{
	  gimage_set_unit (gimage, image_scale->resize->unit);
	  gdisplays_resize_cursor_label (gimage);
	}

      if (image_scale->resize->width != gimage->width ||
	  image_scale->resize->height != gimage->height)
	{
	  if (image_scale->resize->width > 0 &&
	      image_scale->resize->height > 0) 
	    {
	      gimage_scale (gimage,
			    image_scale->resize->width,
			    image_scale->resize->height);
	      flush = TRUE;
	    }
	  else 
	    {
	      g_message (_("Scale Error: Both width and height must be "
			   "greater than zero."));
	      return;
	    }
	}

      if (flush)
	gdisplays_flush ();
    }

  resize_widget_free (image_scale->resize);
  g_free (image_scale);
}

static gint
image_delete_callback (GtkWidget *w,
		       GdkEvent *e,
		       gpointer client_data)
{
  image_cancel_callback (w, client_data);

  return TRUE;
}


static void
image_cancel_callback (GtkWidget *w,
		       gpointer   client_data)
{
  ImageResize *image_resize;
  image_resize = (ImageResize *) client_data;

  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
gimage_mask_feather_callback (GtkWidget *w,
			      gpointer   client_data,
			      gpointer   call_data)
{
  GImage *gimage = GIMP_IMAGE (client_data);
  GUnit unit;
  double radius_x;
  double radius_y;

  selection_feather_radius = *(double*) call_data;
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (w), "size_query_unit");

  radius_x = radius_y = selection_feather_radius;

  if (unit != UNIT_PIXEL)
    {
      double factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimage_mask_feather (gimage, radius_x, radius_y);
  gdisplays_flush ();
}


static void
gimage_mask_border_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage = GIMP_IMAGE (client_data);
  GUnit unit;
  double radius_x;
  double radius_y;

  selection_border_radius = (int) (*(double*) call_data + 0.5);
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (w), "size_query_unit");

  radius_x = radius_y = selection_feather_radius;

  if (unit != UNIT_PIXEL)
    {
      double factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }
  
  gimage_mask_border (gimage, radius_x, radius_y);
  gdisplays_flush ();
}


static void
gimage_mask_grow_callback (GtkWidget *w,
			   gpointer   client_data,
			   gpointer   call_data)
{
  GImage *gimage = GIMP_IMAGE (client_data);
  GUnit unit;
  double radius_x;
  double radius_y;

  selection_grow_pixels = (int) (*(double*) call_data + 0.5);
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (w), "size_query_unit");

  radius_x = radius_y = selection_grow_pixels;
  
  if (unit != UNIT_PIXEL)
    {
      double factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimage_mask_grow (gimage, radius_x, radius_y);
  gdisplays_flush ();
}


static void
gimage_mask_shrink_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage = GIMP_IMAGE (client_data);
  GUnit unit;
  int radius_x;
  int radius_y;

  selection_shrink_pixels = (int) (*(double*) call_data + 0.5);
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (w), "size_query_unit");

  radius_x = radius_y = selection_shrink_pixels;

  if (unit != UNIT_PIXEL)
    {
      double factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimage_mask_shrink (gimage, radius_x, radius_y, FALSE);
  gdisplays_flush ();
}
