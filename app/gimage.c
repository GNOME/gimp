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

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpcontainer.h"
#include "core/gimpimage.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "dialog_handler.h"
#include "gdisplay.h"
#include "lc_dialog.h"
#include "gui/palette-import-dialog.h"

#include "context_manager.h"
#include "gimage.h"
#include "undo.h"


/* gimage.c: Junk (ugly dependencies) from gimpimage.c on its way
 * to proper places. That is, the handlers should be moved to
 * layers_dialog, gdisplay, tools, etc..
 */

static void gimage_dirty_handler         (GimpImage *gimage);
static void gimage_destroy_handler       (GimpImage *gimage);
static void gimage_cmap_change_handler   (GimpImage *gimage,
					  gint       ncol,
					  gpointer   user_data);
static void gimage_rename_handler        (GimpImage *gimage);
static void gimage_size_changed_handler  (GimpImage *gimage);
static void gimage_alpha_changed_handler (GimpImage *gimage);
static void gimage_repaint_handler       (GimpImage *gimage,
					  gint       x,
					  gint       y,
					  gint       w,
					  gint       h);


GimpImage *
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
  gtk_signal_connect (GTK_OBJECT (gimage), "name_changed",
		      GTK_SIGNAL_FUNC (gimage_rename_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "size_changed",
		      GTK_SIGNAL_FUNC (gimage_size_changed_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "alpha_changed",
		      GTK_SIGNAL_FUNC (gimage_alpha_changed_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "repaint",
		      GTK_SIGNAL_FUNC (gimage_repaint_handler),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (gimage), "colormap_changed",
		      GTK_SIGNAL_FUNC (gimage_cmap_change_handler),
		      NULL);

  gimp_container_add (image_context, GIMP_OBJECT (gimage));

  return gimage;
}


static void
gimage_dirty_handler (GimpImage *gimage)
{
  if (active_tool && ! active_tool->preserve)
    {
      GDisplay* gdisp = active_tool->gdisp;

      if (gdisp)
	{
	  if (gdisp->gimage == gimage)
	    tool_manager_initialize_tool (active_tool, gdisp);
	  else
	    tool_manager_initialize_tool (active_tool, NULL);

	}
    }
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

  /*  check if this is the last image  */
  if (gimp_container_num_children (image_context) == 1)
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
gimage_size_changed_handler (GimpImage *gimage)
{
  /*  shrink wrap and update all views  */
  gimp_image_invalidate_layer_previews (gimage);
  gimp_image_invalidate_channel_previews (gimage);
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
  gdisplays_resize_cursor_label (gimage);
  gdisplays_update_full (gimage);
  gdisplays_shrink_wrap (gimage);
}

static void
gimage_alpha_changed_handler (GimpImage *gimage)
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
