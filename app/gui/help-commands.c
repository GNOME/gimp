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
#include "actionarea.h"
#include "app_procs.h"
#include "brightness_contrast.h"
#include "gimpbrushlist.h"
#include "by_color_select.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "color_balance.h"
#include "commands.h"
#include "convert.h"
#include "curves.h"
#include "desaturate.h"
#include "devices.h"
#include "channel_ops.h"
#include "drawable.h"
#include "equalize.h"
#include "fileops.h"
#include "floating_sel.h"
#include "gdisplay_ops.h"
#include "general.h"
#include "gimage_cmds.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "global_edit.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "image_render.h"
#include "indexed_palette.h"
#include "info_window.h"
#include "interface.h"
#include "invert.h"
#include "layers_dialog.h"
#include "layer_select.h"
#include "levels.h"
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

typedef struct
{
  GtkWidget * shell;
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

/*  variables declared in gimage_mask.c--we need them to set up
 *  initial values for the various dialog boxes which query for values
 */
extern double gimage_mask_feather_radius;
extern int    gimage_mask_border_radius;
extern int    gimage_mask_grow_pixels;
extern int    gimage_mask_shrink_pixels;


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
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_border_radius);
  query_string_box ("Border Selection", "Border selection by:", initial,
		    gimage_mask_border_callback, gdisp->gimage);
}

void
select_feather_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%f", gimage_mask_feather_radius);
  query_string_box ("Feather Selection", "Feather selection by:", initial,
		    gimage_mask_feather_callback, gdisp->gimage);
}

void
select_grow_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_grow_pixels);
  query_string_box ("Grow Selection", "Grow selection by:", initial,
		    gimage_mask_grow_callback, gdisp->gimage);
}

void
select_shrink_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  char initial[16];

  gdisp = gdisplay_active ();

  sprintf (initial, "%d", gimage_mask_shrink_pixels);
  query_string_box ("Shrink Selection", "Shrink selection by:", initial,
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
  static ActionAreaItem action_items[2] =
  {
    { "OK", image_resize_callback, NULL, NULL },
    { "Cancel", image_cancel_callback, NULL, NULL }
  };
  GDisplay * gdisp;
  GtkWidget *vbox;
  ImageResize *image_resize;

  gdisp = gdisplay_active ();

  /*  the ImageResize structure  */
  image_resize = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_resize->gimage = gdisp->gimage;
  image_resize->resize = resize_widget_new (ResizeWidget, gdisp->gimage->width, gdisp->gimage->height);

  /*  the dialog  */
  image_resize->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (image_resize->shell), "image_resize", "Gimp");
  gtk_window_set_title (GTK_WINDOW (image_resize->shell), "Image Resize");
  gtk_window_set_policy (GTK_WINDOW (image_resize->shell), FALSE, FALSE, TRUE);
  gtk_window_position (GTK_WINDOW (image_resize->shell), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (image_resize->shell), "delete_event",
		      GTK_SIGNAL_FUNC (image_delete_callback),
		      image_resize);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_resize->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_resize->resize->resize_widget, FALSE, FALSE, 0);

  action_items[0].user_data = image_resize;
  action_items[1].user_data = image_resize;
  build_action_area (GTK_DIALOG (image_resize->shell), action_items, 2, 0);

  gtk_widget_show (image_resize->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_resize->shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer client_data)
{
  static ActionAreaItem action_items[2] =
  {
    { "OK", image_scale_callback, NULL, NULL },
    { "Cancel", image_cancel_callback, NULL, NULL }
  };
  GDisplay * gdisp;
  GtkWidget *vbox;
  ImageResize *image_scale;

  gdisp = gdisplay_active ();

  /*  the ImageResize structure  */
  image_scale = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_scale->gimage = gdisp->gimage;
  image_scale->resize = resize_widget_new (ScaleWidget, gdisp->gimage->width, gdisp->gimage->height);

  /*  the dialog  */
  image_scale->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (image_scale->shell), "image_scale", "Gimp");
  gtk_window_set_title (GTK_WINDOW (image_scale->shell), "Image Scale");
  gtk_window_set_policy (GTK_WINDOW (image_scale->shell), FALSE, FALSE, TRUE);
  gtk_window_position (GTK_WINDOW (image_scale->shell), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (image_scale->shell), "delete_event",
		      GTK_SIGNAL_FUNC (image_delete_callback),
		      image_scale);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_scale->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_scale->resize->resize_widget, FALSE, FALSE, 0);

  action_items[0].user_data = image_scale;
  action_items[1].user_data = image_scale;
  build_action_area (GTK_DIALOG (image_scale->shell), action_items, 2, 0);

  gtk_widget_show (image_scale->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_scale->shell);
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
  int value;

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

void
dialogs_indexed_palette_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();

  indexed_palette_create (gdisp->gimage);
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


/****************************************************/
/**           LOCAL FUNCTIONS                      **/
/****************************************************/


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
			 image_resize->resize->off_x,
			 image_resize->resize->off_y);
	  gdisplays_flush ();
	}
      else 
	{
	  g_message ("Resize Error: Both width and height must be greater than zero.");
	  return;
	}
    }

  gtk_widget_destroy (image_resize->shell);
  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
image_scale_callback (GtkWidget *w,
		      gpointer   client_data)
{
  ImageResize *image_scale;
  GImage *gimage;

  image_scale = (ImageResize *) client_data;
  if ((gimage = image_scale->gimage) != NULL)
    {
      if (image_scale->resize->width > 0 &&
	  image_scale->resize->height > 0) 
	{
	  gimage_scale (gimage,
			image_scale->resize->width,
			image_scale->resize->height);
	  gdisplays_flush ();
	}
      else 
	{
	  g_message ("Scale Error: Both width and height must be greater than zero.");
	  return;
	}
    }

  gtk_widget_destroy (image_scale->shell);
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

  gtk_widget_destroy (image_resize->shell);
  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
gimage_mask_feather_callback (GtkWidget *w,
			      gpointer   client_data,
			      gpointer   call_data)
{
  GImage *gimage=GIMP_IMAGE(client_data);
  double feather_radius;

  feather_radius = atof (call_data);

  gimage_mask_feather (gimage, feather_radius);
  gdisplays_flush ();
}


static void
gimage_mask_border_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage;
  int border_radius;

  gimage=GIMP_IMAGE(client_data);
  
  border_radius = atoi (call_data);

  gimage_mask_border (gimage, border_radius);
  gdisplays_flush ();
}


static void
gimage_mask_grow_callback (GtkWidget *w,
			   gpointer   client_data,
			   gpointer   call_data)
{
  GImage *gimage=GIMP_IMAGE(client_data);
  int grow_pixels;

  grow_pixels = atoi (call_data);

  gimage_mask_grow (gimage, grow_pixels);
  gdisplays_flush ();
}


static void
gimage_mask_shrink_callback (GtkWidget *w,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage *gimage=GIMP_IMAGE(client_data);
  int shrink_pixels;

  shrink_pixels = atoi (call_data);

  gimage_mask_shrink (gimage, shrink_pixels);
  gdisplays_flush ();
}






