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

#include "config.h"

#include <stdio.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-desaturate.h"
#include "core/gimpdrawable-equalize.h"
#include "core/gimpimage.h"
#include "core/gimpimage-duplicate.h"
#include "core/gimpimage-mask.h"
#include "core/gimptoolinfo.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "commands.h"
#include "convert-dialog.h"
#include "errorconsole.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"
#include "info-dialog.h"
#include "info-window.h"
#include "layer-select.h"
#include "offset-dialog.h"

#include "app_procs.h"
#include "context_manager.h"
#include "floating_sel.h"
#include "gdisplay_ops.h"
#include "gimphelp.h"
#include "gimprc.h"
#include "gimpui.h"
#include "global_edit.h"
#include "image_render.h"
#include "nav_window.h"
#include "plug_in.h"
#include "resize.h"
#include "scale.h"
#include "selection.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


/*  local functions  */
static void     image_resize_callback        (GtkWidget *widget,
					      gpointer   data);
static void     image_scale_callback         (GtkWidget *widget,
					      gpointer   data);
static void     gimage_mask_feather_callback (GtkWidget *widget,
					      gdouble    size,
					      GimpUnit   unit,
					      gpointer   data);
static void     gimage_mask_border_callback  (GtkWidget *widget,
					      gdouble    size,
					      GimpUnit   unit,
		  			      gpointer   data);
static void     gimage_mask_grow_callback    (GtkWidget *widget,
					      gdouble    size,
					      GimpUnit   unit,
					      gpointer   data);
static void     gimage_mask_shrink_callback  (GtkWidget *widget,
					      gdouble    size,
					      GimpUnit   unit,
					      gpointer   data);


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
file_save_a_copy_as_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  file_save_a_copy_as_callback (widget, client_data);
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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  undo_pop (gdisp->gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  undo_redo (gdisp->gimage);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  global_edit_cut (gdisp);
}

void
edit_copy_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  global_edit_copy (gdisp);
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  global_edit_paste (gdisp, FALSE);
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  global_edit_paste (gdisp, TRUE);
}

void
edit_paste_as_new_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  global_edit_paste_as_new (gdisp);
}

void
edit_named_cut_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  named_edit_cut (gdisp);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  named_edit_copy (gdisp);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  named_edit_paste (gdisp);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  edit_clear (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage));
  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   callback_data,
			guint      callback_action)
{
  GimpFillType fill_type;
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  fill_type = (GimpFillType) callback_action;
  edit_fill (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage),
	     fill_type);
  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimage_mask_stroke (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage));
  gdisplays_flush ();
}

/*****  Select  *****/

void
select_invert_cmd_callback (GtkWidget *widget,
			    gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimage_mask_invert (gdisp->gimage);
  gdisplays_flush ();
}

void
select_all_cmd_callback (GtkWidget *widget,
			 gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimage_mask_all (gdisp->gimage);
  gdisplays_flush ();
}

void
select_none_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimage_mask_none (gdisp->gimage);
  gdisplays_flush ();
}

void
select_float_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimage_mask_float (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage),
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
			      _("Feather Selection by:"),
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
  GDisplay *gdisp;
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

  GDisplay *gdisp;
  return_if_no_display (gdisp);

  shrink_dialog =
    gimp_query_size_box (N_("Shrink Selection"),
			 gimp_standard_help_func,
			 "dialogs/shrink_selection.html",
			 _("Shrink Selection by:"),
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
			      _("Grow Selection by:"),
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
			      _("Border Selection by:"),
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
  GDisplay *gdisp;
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

  change_scale (gdisp, GIMP_ZOOM_IN);
}

void
view_zoomout_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  change_scale (gdisp, GIMP_ZOOM_OUT);
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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! info_window_follows_mouse) 
    {
      if (! gdisp->window_info_dialog)
	gdisp->window_info_dialog = info_window_create ((void *) gdisp);

      info_window_update (gdisp);
      info_dialog_popup (gdisp->window_info_dialog);
    }
  else
    {
      info_window_follow_auto ();
    }
}

void
view_nav_window_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisp;
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
view_toggle_selection_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  GDisplay *gdisp;
  gint      new_val;

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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
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
      if (! GTK_WIDGET_VISIBLE (gdisp->origin))
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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    {
      if (GTK_WIDGET_VISIBLE (gdisp->statusarea))
	gtk_widget_hide (gdisp->statusarea);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (gdisp->statusarea))
	gtk_widget_show (gdisp->statusarea);
    }
}

void
view_toggle_guides_cmd_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  GDisplay *gdisp;
  gint      old_val;

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
  GDisplay *gdisp;
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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  convert_to_rgb (gdisp->gimage);
}

void
image_convert_grayscale_cmd_callback (GtkWidget *widget,
				      gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  convert_to_grayscale (gdisp->gimage);
}

void
image_convert_indexed_cmd_callback (GtkWidget *widget,
				    gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  convert_to_indexed (gdisp->gimage);
}

void
image_desaturate_cmd_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;
  return_if_no_display (gdisp);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! gimp_drawable_is_rgb (drawable))
    {
      g_message (_("Desaturate operates only on RGB color drawables."));
      return;
    }

  gimp_drawable_desaturate (drawable);

  gdisplays_flush ();
}

void
image_invert_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;
  Argument     *return_vals;
  gint          nreturn_vals;
  return_if_no_display (gdisp);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Invert does not operate on indexed drawables."));
      return;
    }

  return_vals =
    procedural_db_run_proc ("gimp_invert",
			    &nreturn_vals,
			    PDB_DRAWABLE, gimp_drawable_get_ID (drawable),
			    PDB_END);

  if (!return_vals || return_vals[0].value.pdb_int != PDB_SUCCESS)
    g_message (_("Invert operation failed."));

  procedural_db_destroy_args (return_vals, nreturn_vals);

  gdisplays_flush ();
}

void
image_equalize_cmd_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;
  return_if_no_display (gdisp);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Equalize does not operate on indexed drawables."));
      return;
    }

  gimp_drawable_equalize (drawable, TRUE);

  gdisplays_flush ();
}

void
image_offset_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  offset_dialog_create (gdisp->gimage);
}

void
image_resize_cmd_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  GDisplay    *gdisp;
  GimpImage   *gimage;
  ImageResize *image_resize;

  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  /*  the ImageResize structure  */
  image_resize = g_new (ImageResize, 1);
  image_resize->gimage = gimage;
  image_resize->resize = resize_widget_new (ResizeWidget,
					    ResizeImage,
					    GTK_OBJECT (gimage),
					    "destroy",
					    gimage->width,
					    gimage->height,
					    gimage->xresolution,
					    gimage->yresolution,
					    gimage->unit,
					    gdisp->dot_for_dot,
					    image_resize_callback,
					    NULL,
					    image_resize);

  gtk_signal_connect_object (GTK_OBJECT (image_resize->resize->resize_shell),
			     "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) image_resize);

  gtk_widget_show (image_resize->resize->resize_shell);
}

void
image_scale_cmd_callback (GtkWidget *widget,
			  gpointer   client_data)
{
  GDisplay    *gdisp;
  GimpImage   *gimage;
  ImageResize *image_scale;

  return_if_no_display (gdisp);

  gimage = gdisp->gimage;

  /*  the ImageResize structure  */
  image_scale = g_new (ImageResize, 1);
  image_scale->gimage = gimage;
  image_scale->resize = resize_widget_new (ScaleWidget,
					   ResizeImage,
					   GTK_OBJECT (gimage),
					   "destroy",
					   gimage->width,
					   gimage->height,
					   gimage->xresolution,
					   gimage->yresolution,
					   gimage->unit,
					   gdisp->dot_for_dot,
					   image_scale_callback,
					   NULL,
					   image_scale);

  gtk_signal_connect_object (GTK_OBJECT (image_scale->resize->resize_shell),
			     "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) image_scale);

  gtk_widget_show (image_scale->resize->resize_shell);
}

void
image_duplicate_cmd_callback (GtkWidget *widget,
			      gpointer   client_data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisplay_new (gimp_image_duplicate (gdisp->gimage), 0x0101);
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
tools_swap_contexts_cmd_callback (GtkWidget *widget,
				  gpointer   client_data)
{
  static GimpContext *swap_context = NULL;
  static GimpContext *temp_context = NULL;

  if (! swap_context)
    {
      swap_context = gimp_context_new ("Swap Context",
				       gimp_context_get_user ());
      temp_context = gimp_context_new ("Temp Context", NULL);
    }

  gimp_context_copy_args (gimp_context_get_user (),
			  temp_context,
			  GIMP_CONTEXT_ALL_ARGS_MASK);
  gimp_context_copy_args (swap_context,
			  gimp_context_get_user (),
			  GIMP_CONTEXT_ALL_ARGS_MASK);
  gimp_context_copy_args (temp_context,
			  swap_context,
			  GIMP_CONTEXT_ALL_ARGS_MASK);
}

void
tools_select_cmd_callback (GtkWidget *widget,
			   gpointer   callback_data,
			   guint      callback_action)
{
  GtkType       tool_type;
  GimpToolInfo *tool_info;
  GDisplay     *gdisp;

  tool_type = callback_action;

  tool_info = tool_manager_get_info_by_type (tool_type);
  gdisp     = gdisplay_active ();

  gimp_context_set_tool (gimp_context_get_user (), tool_info);

#ifdef __GNUC__
#warning FIXME (let the tool manager to this stuff)
#endif

  /*  Paranoia  */
  active_tool->drawable = NULL;

  /*  Complete the initialisation by doing the same stuff
   *  tools_initialize() does after it did what tools_select() does
   */
  if (GIMP_TOOL_CLASS (GTK_OBJECT (active_tool)->klass)->initialize)
    {
      gimp_tool_initialize (active_tool, gdisp);

      active_tool->drawable = gimp_image_active_drawable (gdisp->gimage);
    }

  /*  setting the tool->gdisp here is a HACK to allow the tools'
   *  dialog windows being hidden if the tool was selected from
   *  a tear-off-menu and there was no mouse click in the display
   *  before deleting it
   */
  active_tool->gdisp = gdisp;
}

/*****  Filters  *****/

void
filters_repeat_cmd_callback (GtkWidget *widget,
			     gpointer   callback_data,
			     guint      callback_action)
{
  plug_in_repeat (callback_action);
}

/*****  Help  *****/

void
help_help_cmd_callback (GtkWidget *widget,
			gpointer   client_data)
{
  gimp_standard_help_func (NULL);
}

void
help_context_help_cmd_callback (GtkWidget *widget,
				gpointer   client_data)
{
  gimp_context_help ();
}


/*****************************/
/*****  Local functions  *****/
/*****************************/

static void
image_resize_callback (GtkWidget *widget,
		       gpointer   client_data)
{
  ImageResize *image_resize;
  GimpImage   *gimage;

  image_resize = (ImageResize *) client_data;

  gtk_widget_set_sensitive (image_resize->resize->resize_shell, FALSE);

  if ((gimage = image_resize->gimage) != NULL)
    {
      if (image_resize->resize->width > 0 &&
	  image_resize->resize->height > 0) 
	{
	  gimp_image_resize (gimage,
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

  gtk_widget_destroy (image_resize->resize->resize_shell);
}

static void
image_scale_callback (GtkWidget *widget,
		      gpointer   client_data)
{
  ImageResize *image_scale = (ImageResize *) client_data;

  g_assert (image_scale != NULL);
  g_assert (image_scale->gimage != NULL);

  gtk_widget_set_sensitive (image_scale->resize->resize_shell, FALSE);

  if (resize_check_layer_scaling (image_scale))
    {
      resize_scale_implement (image_scale);
      gtk_widget_destroy (image_scale->resize->resize_shell);
    }
}

static void
gimage_mask_feather_callback (GtkWidget *widget,
			      gdouble    size,
			      GimpUnit   unit,
			      gpointer   data)
{
  GimpImage *gimage;
  gdouble    radius_x;
  gdouble    radius_y;

  gimage = GIMP_IMAGE (data);

  selection_feather_radius = size;

  radius_x = radius_y = selection_feather_radius;

  if (unit != GIMP_UNIT_PIXEL)
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
			     gdouble    size,
			     GimpUnit   unit,
			     gpointer   data)
{
  GimpImage *gimage;
  gdouble    radius_x;
  gdouble    radius_y;

  gimage = GIMP_IMAGE (data);

  selection_border_radius = ROUND (size);

  radius_x = radius_y = selection_border_radius;

  if (unit != GIMP_UNIT_PIXEL)
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
			   gdouble    size,
			   GimpUnit   unit,
			   gpointer   data)
{
  GimpImage *gimage;
  gdouble    radius_x;
  gdouble    radius_y;

  gimage = GIMP_IMAGE (data);

  selection_grow_pixels = ROUND (size);

  radius_x = radius_y = selection_grow_pixels;

  if (unit != GIMP_UNIT_PIXEL)
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
			     gdouble    size,
			     GimpUnit   unit,
			     gpointer   data)
{
  GimpImage *gimage;
  gint       radius_x;
  gint       radius_y;

  gimage = GIMP_IMAGE (data);

  selection_shrink_pixels = ROUND (size);

  radius_x = radius_y = selection_shrink_pixels;

  selection_shrink_edge_lock =
    ! GTK_TOGGLE_BUTTON (gtk_object_get_data (GTK_OBJECT (widget),
					      "edge_lock_toggle"))->active;

  if (unit != GIMP_UNIT_PIXEL)
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
