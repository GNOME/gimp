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
#include "appenv.h"
#include "about_dialog.h"
#include "app_procs.h"
#include "brush_select.h"
#include "colormaps.h"
#include "colormap_dialog.i.h"
#include "color_area.h"
#include "commands.h"
#include "convert.h"
#include "desaturate.h"
#include "devices.h"
#include "channel_ops.h"
#include "drawable.h"
#include "equalize.h"
#include "errorconsole.h"
#include "fileops.h"
#include "floating_sel.h"
#include "gdisplay_ops.h"
#include "gdisplay_color_ui.h"
#include "gimage_mask.h"
#include "gimphelp.h"
#include "gimprc.h"
#include "gimpui.h"
#include "global_edit.h"
#include "gradient_select.h"
#include "image_render.h"
#include "info_window.h"
#include "nav_window.h"
#include "invert.h"
#include "lc_dialog.h"
#include "layer_select.h"
#include "module_db.h"
#include "palette.h"
#include "pattern_select.h"
#include "plug_in.h"
#include "resize.h"
#include "scale.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return

typedef struct
{
  Resize    *resize;
  GimpImage *gimage;
} ImageResize;

/*  external functions  */
extern void   layers_dialog_layer_merge_query (GImage *, gboolean);

/*  local functions  */
static void   image_resize_callback        (GtkWidget *, gpointer);
static void   image_scale_callback         (GtkWidget *, gpointer);
static void   image_cancel_callback        (GtkWidget *, gpointer);
static void   gimage_mask_feather_callback (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_border_callback  (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_grow_callback    (GtkWidget *, gpointer, gpointer);
static void   gimage_mask_shrink_callback  (GtkWidget *, gpointer, gpointer);

/*  local variables  */
static gdouble   selection_feather_radius    = 5.0;
static gint      selection_border_radius     = 5;
static gint      selection_grow_pixels       = 1;
static gint      selection_shrink_pixels     = 1;
static gboolean  selection_shrink_edge_lock  = FALSE;


/*****  File  *****/

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
  return_if_no_display (gdisp);

  gdisplay_close_window (gdisp, FALSE);
}

void
file_quit_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  app_exit (FALSE);
}

/*****  Edit  *****/

void
edit_undo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  undo_pop (gdisp->gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  undo_redo (gdisp->gimage);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  global_edit_cut (gdisp);
}

void
edit_copy_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  global_edit_copy (gdisp);
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  global_edit_paste (gdisp, 0);
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  global_edit_paste (gdisp, 1);
}

void
edit_paste_as_new_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  global_edit_paste_as_new (gdisp);
}

void
edit_named_cut_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  named_edit_cut (gdisp);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  named_edit_copy (gdisp);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  named_edit_paste (gdisp);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  edit_clear (gdisp->gimage, gimage_active_drawable (gdisp->gimage));
  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  edit_fill (gdisp->gimage, gimage_active_drawable (gdisp->gimage));
  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_stroke (gdisp->gimage, gimage_active_drawable (gdisp->gimage));
  gdisplays_flush ();
}

/*****  Select  *****/

void
select_invert_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_invert (gdisp->gimage);
  gdisplays_flush ();
}

void
select_all_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_all (gdisp->gimage);
  gdisplays_flush ();
}

void
select_none_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_none (gdisp->gimage);
  gdisplays_flush ();
}

void
select_float_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_float (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
		     0, 0);
  gdisplays_flush ();
}

void
select_feather_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GtkWidget *qbox;
  GDisplay  *gdisp;

  return_if_no_display (gdisp);

  qbox = gimp_query_size_box (_("Feather Selection"),
			      gimp_standard_help_func,
			      "dialogs/feather_selection.html",
			      _("Feather selection by:"),
			      selection_feather_radius, 0, 32767, 3,
			      gdisp->gimage->unit,
			      MIN (gdisp->gimage->xresolution,
				   gdisp->gimage->yresolution),
			      gdisp->dot_for_dot,
			      GTK_OBJECT (gdisp->gimage), "destroy",
			      gimage_mask_feather_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
select_sharpen_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_sharpen (gdisp->gimage);
  gdisplays_flush ();
}

void
select_shrink_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GtkWidget *edge_lock;
  GtkWidget *shrink_dialog;

  GDisplay * gdisp;
  return_if_no_display (gdisp);

  shrink_dialog =
    gimp_query_size_box (N_("Shrink Selection"),
			 gimp_standard_help_func,
			 "dialogs/shrink_selection.html",
			 N_("Shrink selection by:"),
			 selection_shrink_pixels, 1, 32767, 0,
			 gdisp->gimage->unit,
			 MIN (gdisp->gimage->xresolution,
			      gdisp->gimage->yresolution),
			 gdisp->dot_for_dot,
			 GTK_OBJECT (gdisp->gimage), "destroy",
			 gimage_mask_shrink_callback, gdisp->gimage);

  edge_lock = gtk_check_button_new_with_label (_("Shrink from image border"));
  /* eeek */
  gtk_box_pack_start (GTK_BOX (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_DIALOG (shrink_dialog)->vbox)), 0)), edge_lock,
		      FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (shrink_dialog), "edge_lock_toggle",
		       edge_lock);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (edge_lock),
				! selection_shrink_edge_lock);
  gtk_widget_show (edge_lock);

  gtk_widget_show (shrink_dialog);
}

void
select_grow_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GtkWidget *qbox;
  GDisplay  *gdisp;

  return_if_no_display (gdisp);

  qbox = gimp_query_size_box (_("Grow Selection"),
			      gimp_standard_help_func,
			      "dialogs/grow_selection.html",
			      _("Grow selection by:"),
			      selection_grow_pixels, 1, 32767, 0,
			      gdisp->gimage->unit,
			      MIN (gdisp->gimage->xresolution,
				   gdisp->gimage->yresolution),
			      gdisp->dot_for_dot,
			      GTK_OBJECT (gdisp->gimage), "destroy",
			      gimage_mask_grow_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
select_border_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GtkWidget *qbox;
  GDisplay  *gdisp;

  return_if_no_display (gdisp);

  qbox = gimp_query_size_box (_("Border Selection"),
			      gimp_standard_help_func,
			      "dialogs/border_selection.html",
			      _("Border selection by:"),
			      selection_border_radius, 1, 32767, 0,
			      gdisp->gimage->unit,
			      MIN (gdisp->gimage->xresolution,
				   gdisp->gimage->yresolution),
			      gdisp->dot_for_dot,
			      GTK_OBJECT (gdisp->gimage), "destroy",
			      gimage_mask_border_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
select_save_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_save (gdisp->gimage);
  gdisplays_flush ();
}

/*****  View  *****/

void
view_zoomin_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, ZOOMIN);
}

void
view_zoomout_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, ZOOMOUT);
}

static void
view_zoom_val (GtkWidget *widget,
	       gpointer   client_data,
	       int        val)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, val);
}

void
view_zoom_16_1_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  view_zoom_val (widget, client_data, 1601);
}

void
view_zoom_8_1_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 801);
}

void
view_zoom_4_1_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 401);
}

void
view_zoom_2_1_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 201);
}

void
view_zoom_1_1_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 101);
}

void
view_zoom_1_2_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 102);
}

void
view_zoom_1_4_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 104);
}

void
view_zoom_1_8_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  view_zoom_val (widget, client_data, 108);
}

void
view_zoom_1_16_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  view_zoom_val (widget, client_data, 116);
}

void
view_dot_for_dot_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisplay_set_dot_for_dot (gdisp, GTK_CHECK_MENU_ITEM (widget)->active);
}

void
view_info_window_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  if (!info_window_follows_mouse) 
    {
      if (! gdisp->window_info_dialog)
	gdisp->window_info_dialog = info_window_create ((void *) gdisp);
      info_window_update(gdisp);
      info_dialog_popup (gdisp->window_info_dialog);
    }
  else
    {
      info_window_follow_auto();
    }

}

void
view_nav_window_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  if (nav_window_per_display) 
    {
      if (! gdisp->window_nav_dialog)
	gdisp->window_nav_dialog = nav_window_create ((void *) gdisp);

      nav_dialog_popup (gdisp->window_nav_dialog);
    }
  else
    {
      nav_window_follow_auto ();
    }
}

void
view_undo_history_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  GDisplay * gdisp;
  GImage   * gimage;
  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  if (!gimage->undo_history)
    gimage->undo_history = undo_history_new (gimage);

  if (!GTK_WIDGET_VISIBLE (gimage->undo_history))
    gtk_widget_show (gimage->undo_history);
  else
    gdk_window_raise (gimage->undo_history->window);
}

void
view_toggle_selection_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  GDisplay * gdisp;
  int new_val;

  return_if_no_display (gdisp);

  new_val = GTK_CHECK_MENU_ITEM (widget)->active;

  /*  hidden == TRUE corresponds to the menu toggle being FALSE  */
  if (new_val == gdisp->select->hidden)
    {
      selection_hide (gdisp->select, (void *) gdisp);
      gdisplays_flush ();
    }
}

void
view_toggle_rulers_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_hide (gdisp->origin);
	  gtk_widget_hide (gdisp->hrule);
	  gtk_widget_hide (gdisp->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (gdisp->origin))
	{
	  gtk_widget_show (gdisp->origin);
	  gtk_widget_show (gdisp->hrule);
	  gtk_widget_show (gdisp->vrule);

	  gtk_widget_queue_resize (GTK_WIDGET (gdisp->origin->parent));
	}
    }
}

void
view_toggle_statusbar_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

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
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;
  int old_val;

  return_if_no_display (gdisp);

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
  return_if_no_display (gdisp);

  gdisp->snap_to_guides = GTK_CHECK_MENU_ITEM (widget)->active;
}

void
view_new_view_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisplay_new_view (gdisp);
}

void
view_shrink_wrap_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  shrink_wrap_display (gdisp);
}

/*****  Image  *****/

void
image_convert_rgb_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  convert_to_rgb (gdisp->gimage);
}

void
image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  convert_to_grayscale (gdisp->gimage);
}

void
image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  convert_to_indexed (gdisp->gimage);
}

void
image_desaturate_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  image_desaturate (gdisp->gimage);
  gdisplays_flush ();
}

void
image_invert_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  image_invert (gdisp->gimage);
  gdisplays_flush ();
}

void
image_equalize_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  image_equalize (gdisp->gimage);
  gdisplays_flush ();
}

void
image_offset_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  channel_ops_offset (gdisp->gimage);
}

void
image_resize_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay    * gdisp;
  GimpImage   * gimage;
  ImageResize * image_resize;

  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  /*  the ImageResize structure  */
  image_resize = g_new (ImageResize, 1);
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
					    image_resize);

  gtk_widget_show (image_resize->resize->resize_shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay    * gdisp;
  GimpImage   * gimage;
  ImageResize * image_scale;

  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  /*  the ImageResize structure  */
  image_scale = g_new (ImageResize, 1);
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
					   image_scale);

  gtk_widget_show (image_scale->resize->resize_shell);
}

void
image_duplicate_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  channel_ops_duplicate (gdisp->gimage);
}

/*****  Layers  *****/

void
layers_previous_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay * gdisp;
  Layer    * new_layer;
  gint       current_layer;

  return_if_no_display (gdisp);

  current_layer =
    gimage_get_layer_index (gdisp->gimage, gdisp->gimage->active_layer);

  /*  FIXME: don't use internal knowledge about layer lists
   *  TODO : implement gimage_get_layer_by_index()
   */
  new_layer =
    (Layer *) g_slist_nth_data (gdisp->gimage->layers, current_layer - 1);

  if (new_layer)
    {
      gimage_set_active_layer (gdisp->gimage, new_layer);
      gdisplays_flush ();
    }
}

void
layers_next_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay * gdisp;
  Layer    * new_layer;
  gint       current_layer;

  return_if_no_display (gdisp);

  current_layer =
    gimage_get_layer_index (gdisp->gimage, gdisp->gimage->active_layer);

  /*  FIXME: don't use internal knowledge about layer lists
   *  TODO : implement gimage_get_layer_by_index()
   */
  new_layer =
    (Layer *) g_slist_nth_data (gdisp->gimage->layers, current_layer + 1);

  if (new_layer)
    {
      gimage_set_active_layer (gdisp->gimage, new_layer);
      gdisplays_flush ();
    }
}

void
layers_raise_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_raise_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_lower_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_lower_layer (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_raise_to_top_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_raise_layer_to_top (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_lower_to_bottom_cmd_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_lower_layer_to_bottom (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_anchor_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  floating_sel_anchor (gimage_get_active_layer (gdisp->gimage));
  gdisplays_flush ();
}

void
layers_merge_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  layers_dialog_layer_merge_query (gdisp->gimage, TRUE);
}

void
layers_flatten_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_flatten (gdisp->gimage);
  gdisplays_flush ();
}

void
layers_mask_select_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_layer_mask (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_add_alpha_channel_cmd_callback (GtkWidget *widget,
				       gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  layer_add_alpha ( gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_alpha_select_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  GDisplay * gdisp;
  return_if_no_display (gdisp);

  gimage_mask_layer_alpha (gdisp->gimage, gdisp->gimage->active_layer);
  gdisplays_flush ();
}

void
layers_resize_to_image_cmd_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  layer_resize_to_image (gdisp->gimage->active_layer);
  gdisplays_flush ();
}

/*****  Tools  *****/

void
tools_default_colors_cmd_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  gimp_context_set_default_colors (gimp_context_get_user ());
}

void
tools_swap_colors_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  gimp_context_swap_colors (gimp_context_get_user ());
}

void
tools_select_cmd_callback (GtkWidget *widget,
			   gpointer   callback_data,
			   guint      callback_action)
{
  ToolType tool_type;
  GDisplay * gdisp;
  gdisp = gdisplay_active ();

  tool_type = (ToolType) callback_action;

  gimp_context_set_tool (gimp_context_get_user (), tool_type);

  /*  Paranoia  */
  active_tool->drawable = NULL;

  /*  Complete the initialisation by doing the same stuff
   *  tools_initialize() does after it did what tools_select() does
   */
  if (tool_info[tool_type].init_func)
    {
      (* tool_info[tool_type].init_func) (gdisp);

      active_tool->drawable = gimage_active_drawable (gdisp->gimage);
    }

  /*  setting the gdisp_ptr here is a HACK to allow the tools'
   *  dialog windows being hidden if the tool was selected from
   *  a tear-off-menu and there was no mouse click in the display
   *  before deleting it
   */
  active_tool->gdisp_ptr = gdisp;
}

/*****  Filters  *****/

void
filters_repeat_cmd_callback (GtkWidget *widget,
			     gpointer   callback_data,
			     guint      callback_action)
{
  plug_in_repeat (callback_action);
}

/*****  Dialogs  ******/

void
dialogs_lc_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay * gdisp;
  gdisp = gdisplay_active ();

  lc_dialog_create (gdisp ? gdisp->gimage : NULL);
}

void
dialogs_tool_options_cmd_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  tool_options_dialog_show ();
}

void
dialogs_brushes_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  brush_dialog_create ();
}

void
dialogs_patterns_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  pattern_dialog_create ();
}

void
dialogs_gradients_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  gradient_dialog_create ();
}

void
dialogs_palette_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  palette_dialog_create ();
}

static void
dialogs_indexed_palette_select_callback (ColormapDialog *dlg,
					 gpointer        user_data)
{
  guchar    *c;
  GimpImage *img = colormap_dialog_image (dlg);

  c = &img->cmap[colormap_dialog_col_index (dlg) * 3];

  if (active_color == FOREGROUND)
    gimp_context_set_foreground (gimp_context_get_user (), c[0], c[1], c[2]);
  else if (active_color == BACKGROUND)
    gimp_context_set_background (gimp_context_get_user (), c[0], c[1], c[2]);
}

void
dialogs_indexed_palette_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  static ColormapDialog *cmap_dlg;

  if (!cmap_dlg)
    {
      cmap_dlg = colormap_dialog_create (image_context);
      colormap_dialog_connect_selected (dialogs_indexed_palette_select_callback,
					NULL, cmap_dlg);
    }

  gtk_widget_show (GTK_WIDGET (cmap_dlg));
}

void
dialogs_input_devices_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  input_dialog_create ();
}

void
dialogs_device_status_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  device_status_create ();
}

void
dialogs_error_console_cmd_callback (GtkWidget *widget,
				    gpointer   client_data) 
{
  error_console_add (NULL);
}

void
dialogs_display_filters_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  GDisplay *gdisp;

  gdisp = gdisplay_active ();
  if (!gdisp)
    gdisp = color_area_gdisp;

  if (!gdisp->cd_ui)
    gdisplay_color_ui_new (gdisp);

  if (!GTK_WIDGET_VISIBLE (gdisp->cd_ui))
    gtk_widget_show (gdisp->cd_ui);
  else
    gdk_window_raise (gdisp->cd_ui->window);
}

void
dialogs_module_browser_cmd_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  GtkWidget *w;

  w = module_db_browser_new ();
  gtk_widget_show (w);
}

/*****  Help  *****/

void
help_help_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  gimp_help (NULL);
}

void
help_context_help_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  gimp_context_help ();
}

void
help_tips_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  tips_dialog_create ();
}

void
help_about_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  about_dialog_create (FALSE);
}


/*****************************/
/*****  Local functions  *****/
/*****************************/

static void
image_resize_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  ImageResize *image_resize;
  GImage      *gimage;

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
	  g_message (_("Resize Error: Both width and height must be "
		       "greater than zero."));
	  return;
	}
    }

  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
image_scale_callback (GtkWidget *widget,
		      gpointer   client_data)
{
  ImageResize *image_scale;
  GImage      *gimage;
  gboolean     rulers_flush = FALSE;
  gboolean     display_flush = FALSE;  /* this is a bit ugly: 
				          we hijack the flush variable 
					  to check if an undo_group was 
					  already started */

  image_scale = (ImageResize *) client_data;

  if ((gimage = image_scale->gimage) != NULL)
    {
      if (image_scale->resize->resolution_x != gimage->xresolution ||
	  image_scale->resize->resolution_y != gimage->yresolution)
	{
	  undo_push_group_start (gimage, IMAGE_SCALE_UNDO);
	  
	  gimage_set_resolution (gimage,
				 image_scale->resize->resolution_x,
				 image_scale->resize->resolution_y);

	  rulers_flush = TRUE;
	  display_flush = TRUE;
	}

      if (image_scale->resize->unit != gimage->unit)
	{
	  if (!display_flush)
	    undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

	  gimage_set_unit (gimage, image_scale->resize->unit);
	  gdisplays_setup_scale (gimage);
	  gdisplays_resize_cursor_label (gimage);

	  rulers_flush = TRUE;
	  display_flush = TRUE;
	}

      if (image_scale->resize->width != gimage->width ||
	  image_scale->resize->height != gimage->height)
	{
	  if (image_scale->resize->width > 0 &&
	      image_scale->resize->height > 0) 
	    {
	      if (!display_flush)
		undo_push_group_start (gimage, IMAGE_SCALE_UNDO);

	      gimage_scale (gimage,
			    image_scale->resize->width,
			    image_scale->resize->height);

	      display_flush = TRUE;
	    }
	  else
	    {
	      g_message (_("Scale Error: Both width and height must be "
			   "greater than zero."));
	      return;
	    }
	}

      if (rulers_flush)
	{
	  gdisplays_setup_scale (gimage);
	  gdisplays_resize_cursor_label (gimage);
	}
      
      if (display_flush)
	{
	  undo_push_group_end (gimage);
	  gdisplays_flush ();
	}
    }

  resize_widget_free (image_scale->resize);
  g_free (image_scale);
}

static void
image_cancel_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  ImageResize *image_resize;

  image_resize = (ImageResize *) client_data;

  resize_widget_free (image_resize->resize);
  g_free (image_resize);
}

static void
gimage_mask_feather_callback (GtkWidget *widget,
			      gpointer   client_data,
			      gpointer   call_data)
{
  GImage  *gimage;
  GUnit    unit;
  gdouble  radius_x;
  gdouble  radius_y;

  gimage = GIMP_IMAGE (client_data);

  selection_feather_radius = *(gdouble *) call_data;
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (widget), "size_query_unit");

  radius_x = radius_y = selection_feather_radius;

  if (unit != UNIT_PIXEL)
    {
      gdouble factor;

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
gimage_mask_border_callback (GtkWidget *widget,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage  *gimage;
  GUnit    unit;
  gdouble  radius_x;
  gdouble  radius_y;

  gimage = GIMP_IMAGE (client_data);

  selection_border_radius = (gint) (*(gdouble *) call_data + 0.5);
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (widget), "size_query_unit");

  radius_x = radius_y = selection_border_radius;

  if (unit != UNIT_PIXEL)
    {
      gdouble factor;

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
gimage_mask_grow_callback (GtkWidget *widget,
			   gpointer   client_data,
			   gpointer   call_data)
{
  GImage  *gimage;
  GUnit    unit;
  gdouble  radius_x;
  gdouble  radius_y;

  gimage = GIMP_IMAGE (client_data);

  selection_grow_pixels = (gint) (*(gdouble *) call_data + 0.5);
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (widget), "size_query_unit");

  radius_x = radius_y = selection_grow_pixels;

  if (unit != UNIT_PIXEL)
    {
      gdouble factor;

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
gimage_mask_shrink_callback (GtkWidget *widget,
			     gpointer   client_data,
			     gpointer   call_data)
{
  GImage    *gimage;
  GUnit      unit;
  gint       radius_x;
  gint       radius_y;

  gimage = GIMP_IMAGE (client_data);

  selection_shrink_pixels = (gint) (*(gdouble *) call_data + 0.5);
  g_free (call_data);
  unit = (GUnit) gtk_object_get_data (GTK_OBJECT (widget), "size_query_unit");

  radius_x = radius_y = selection_shrink_pixels;

  selection_shrink_edge_lock =
    ! GTK_TOGGLE_BUTTON (gtk_object_get_data (GTK_OBJECT (widget),
					      "edge_lock_toggle"))->active;

  if (unit != UNIT_PIXEL)
    {
      gdouble factor;

      factor = (MAX (gimage->xresolution, gimage->yresolution) /
		MIN (gimage->xresolution, gimage->yresolution));

      if (gimage->xresolution == MIN (gimage->xresolution, gimage->yresolution))
	radius_y *= factor;
      else
	radius_x *= factor;
    }

  gimage_mask_shrink (gimage, radius_x, radius_y, selection_shrink_edge_lock);
  gdisplays_flush ();
}
