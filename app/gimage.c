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

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "channel.h"
#include "dialog_handler.h"
#include "drawable.h"
#include "gimage.h"
#include "gimpimage.h"
#include "gimpset.h"
#include "layer.h"
#include "lc_dialog.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paletteP.h"
#include "procedural_db.h"
#include "tools.h"
#include "undo.h"


/* gimage.c: Junk (ugly dependencies) from gimpimage.c on its way
 * to proper places. That is, the handlers should be moved to
 * layers_dialog, gdisplay, tools, etc..
 */

static void gimage_dirty_handler        (GimpImage* gimage);
static void gimage_destroy_handler      (GimpImage* gimage);
static void gimage_cmap_change_handler  (GimpImage* gimage,
					 gint       ncol,
					 gpointer   user_data);
static void gimage_rename_handler       (GimpImage* gimage);
static void gimage_resize_handler       (GimpImage* gimage);
static void gimage_restructure_handler  (GimpImage* gimage);
static void gimage_repaint_handler      (GimpImage* gimage,
					 gint, gint, gint, gint);


GImage *
gimage_new (gint              width, 
	    gint              height, 
	    GimpImageBaseType base_type)
{
  GimpImage *gimage = gimp_image_new (width, height, base_type);

  gtk_signal_connect (GTK_OBJECT (gimage), "dirty",
		      GTK_SIGNAL_FUNC (gimage_dirty_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "destroy",
		      GTK_SIGNAL_FUNC (gimage_destroy_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "rename",
		      GTK_SIGNAL_FUNC (gimage_rename_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "resize",
		      GTK_SIGNAL_FUNC (gimage_resize_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "restructure",
		      GTK_SIGNAL_FUNC (gimage_restructure_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "repaint",
		      GTK_SIGNAL_FUNC (gimage_repaint_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "colormap_changed",
		      GTK_SIGNAL_FUNC (gimage_cmap_change_handler),
		      NULL);

  gimp_set_add (image_context, gimage);

  return gimage;
}


/* Ack, GImages have their own ref counts! This is going to cause
   trouble.. It should be pretty easy to convert to proper GtkObject
   ref counting, though. */

/* This caused trouble indeed. The ref_count was only used by the
   displays showing the image, so I renamed it to disp_count to 
   make clear that it should only be used for display references.
                                               (Sven, 23.01.2000) */ 

void
gimage_delete (GImage *gimage)
{
  if (gimage->disp_count <= 0)
    gtk_object_unref (GTK_OBJECT (gimage));
}

static void
invalidate_cb (gpointer image, 
	       gpointer user_data)
{
  gimp_image_invalidate_preview (GIMP_IMAGE (image));
}

void
gimage_invalidate_previews (void)
{
  gimp_set_foreach (image_context, invalidate_cb, NULL);
}

static void
gimage_dirty_handler (GimpImage *gimage)
{
  if (active_tool && !active_tool->preserve)
    {
      GDisplay* gdisp = active_tool->gdisp_ptr;

      if (gdisp)
	{
	  if (gdisp->gimage == gimage)
	    tools_initialize (active_tool->type, gdisp);
	  else
	    tools_initialize (active_tool->type, NULL);
	}
    }
}

static void
gimlist_cb (gpointer image, 
	    gpointer data)
{
  GSList **list = (GSList **) data;

  *list = g_slist_prepend (*list, image);
}

gint
gimage_image_count (void)
{
  GSList *list=NULL;
  gint num_images = 0;

  gimage_foreach (gimlist_cb, &list);
  num_images = g_slist_length (list);

  g_slist_free (list);

  return num_images;
}

static void
gimage_destroy_handler (GimpImage *gimage)
{
  GList *list;
  
  /*  free the undo list  */
  undo_free (gimage);

  /*  free all guides  */
  for (list = gimage->guides; list; list = g_list_next (list))
    {
      g_free ((Guide*) list->data);
    }

  g_list_free (gimage->guides);

  if (gimage_image_count () == 1)  /*  This is the last image  */
    {
      dialog_show_toolbox ();
    }
}

static void 
gimage_cmap_change_handler (GimpImage *gimage, 
			    gint       ncol,
			    gpointer   user_data)
{
  gdisplays_update_full (gimage);

  if (gimp_image_base_type (gimage) == INDEXED)
    paint_funcs_invalidate_color_hash_table (gimage, ncol);
}

static void
gimage_rename_handler (GimpImage *gimage)
{
  gdisplays_update_title (gimage);
  lc_dialog_update_image_list ();

  palette_import_image_renamed (gimage);
}

static void
gimage_resize_handler (GimpImage *gimage)
{
  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */
  channel_invalidate_previews (gimage);
  layer_invalidate_previews (gimage);
  gimp_image_invalidate_preview (gimage);
  gdisplays_resize_cursor_label (gimage);
  gdisplays_update_full (gimage);
  gdisplays_shrink_wrap (gimage);
}

static void
gimage_restructure_handler (GimpImage *gimage)
{
  gdisplays_update_title (gimage);
}

static void
gimage_repaint_handler (GimpImage *gimage, 
			gint       x, 
			gint       y, 
			gint       w, 
			gint       h)
{
  gdisplays_update_area (gimage, x, y, w, h);
}

  
/* These really belong in the layer class */

void
gimage_set_layer_mask_apply (GImage    *gimage, 
			     GimpLayer *layer)
{
  int off_x, off_y;

  g_return_if_fail (gimage);
  g_return_if_fail (layer);
  
  if (! layer->mask)
    return;

  layer->apply_mask = ! layer->apply_mask;
  drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
  gdisplays_update_area (gimage, off_x, off_y,
			 drawable_width (GIMP_DRAWABLE (layer)), 
			 drawable_height (GIMP_DRAWABLE (layer)));
}

void
gimage_set_layer_mask_edit (GImage   *gimage, 
			    Layer    *layer, 
			    gboolean  edit)
{
  /*  find the layer  */
  if (!layer)
    return;

  if (layer->mask)
    layer->edit_mask = edit;
}

void
gimage_set_layer_mask_show (GImage    *gimage, 
			    GimpLayer *layer)
{
  gint off_x, off_y;

  g_return_if_fail (gimage);
  g_return_if_fail (layer);
  
  if (! layer->mask)
    return;

  layer->show_mask = ! layer->show_mask;
  drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
  gdisplays_update_area (gimage, off_x, off_y,
			 drawable_width (GIMP_DRAWABLE (layer)), 
			 drawable_height (GIMP_DRAWABLE (layer)));
}

void
gimage_foreach (GFunc    func, 
		gpointer user_data)
{
  gimp_set_foreach (image_context, func, user_data);
}
