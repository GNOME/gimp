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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core-types.h"

/* FIXME: remove the Path <-> BezierSelect dependency */
#include "tools/tools-types.h"

#include "paint-funcs/paint-funcs.h"

#include "apptypes.h"
#include "cursorutil.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-duplicate.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "parasitelist.h"
#include "path.h"
#include "pixel_region.h"
#include "tile_manager.h"

#include "libgimp/gimpintl.h"


GimpImage *
gimp_image_duplicate (GimpImage *gimage)
{
  PixelRegion   srcPR, destPR;
  GimpImage    *new_gimage;
  GimpLayer    *layer, *new_layer;
  GimpLayer    *floating_layer;
  GimpChannel  *channel, *new_channel;
  GList        *list;
  Guide        *guide                     = NULL;
  GimpLayer    *active_layer              = NULL;
  GimpChannel  *active_channel            = NULL;
  GimpDrawable *new_floating_sel_drawable = NULL;
  GimpDrawable *floating_sel_drawable     = NULL;
  ParasiteList *parasites;
  PathList     *paths;
  gint          count;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  gimp_add_busy_cursors_until_idle ();

  /*  Create a new image  */
  new_gimage = gimage_new (gimage->width, gimage->height, gimage->base_type);
  gimp_image_undo_disable (new_gimage);

  /*  Copy resolution and unit information  */
  new_gimage->xresolution = gimage->xresolution;
  new_gimage->yresolution = gimage->yresolution;
  new_gimage->unit = gimage->unit;

  /*  Copy floating layer  */
  floating_layer = gimp_image_floating_sel (gimage);
  if (floating_layer)
    {
      floating_sel_relax (floating_layer, FALSE);

      floating_sel_drawable = floating_layer->fs.drawable;
      floating_layer = NULL;
    }

  /*  Copy the layers  */
  for (list = GIMP_LIST (gimage->layers)->list, count = 0;
       list;
       list = g_list_next (list), count++)
    {
      layer = (GimpLayer *) list->data;

      new_layer = gimp_layer_copy (layer, FALSE);

      gimp_drawable_set_gimage (GIMP_DRAWABLE (new_layer), new_gimage);

      /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_layer),
			    gimp_object_get_name (GIMP_OBJECT (layer)));

      /*  Make sure if the layer has a layer mask, it's name isn't screwed up  */
      if (new_layer->mask)
	{
	  gimp_object_set_name (GIMP_OBJECT (new_layer->mask),
				gimp_object_get_name (GIMP_OBJECT (layer->mask)));
	}

      if (gimp_image_get_active_layer (gimage) == layer)
	active_layer = new_layer;

      if (gimage->floating_sel == layer)
	floating_layer = new_layer;

      if (floating_sel_drawable == GIMP_DRAWABLE (layer))
	new_floating_sel_drawable = GIMP_DRAWABLE (new_layer);

      /*  Add the layer  */
      if (floating_layer != new_layer)
	gimp_image_add_layer (new_gimage, new_layer, count);
    }

  /*  Copy the channels  */
  for (list = GIMP_LIST (gimage->channels)->list, count = 0;
       list;
       list = g_list_next (list), count++)
    {
      channel = (GimpChannel *) list->data;
 
      new_channel = gimp_channel_copy (channel, TRUE);

      gimp_drawable_set_gimage (GIMP_DRAWABLE (new_channel), new_gimage);

      /*  Make sure the copied channel doesn't say: "<old channel> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_channel),
			    gimp_object_get_name (GIMP_OBJECT (channel)));

      if (gimp_image_get_active_channel (gimage) == channel)
	active_channel = (new_channel);

      if (floating_sel_drawable == GIMP_DRAWABLE (channel))
	new_floating_sel_drawable = GIMP_DRAWABLE (new_channel);

      /*  Add the channel  */
      gimp_image_add_channel (new_gimage, new_channel, count);
    }

  /*  Copy the selection mask  */
  pixel_region_init (&srcPR, 
		     gimp_drawable_data (GIMP_DRAWABLE (gimage->selection_mask)), 
		     0, 0, gimage->width, gimage->height, FALSE);
  pixel_region_init (&destPR, 
		     gimp_drawable_data (GIMP_DRAWABLE (new_gimage->selection_mask)),
		     0, 0, gimage->width, gimage->height, TRUE);
  copy_region (&srcPR, &destPR);
  new_gimage->selection_mask->bounds_known = FALSE;
  new_gimage->selection_mask->boundary_known = FALSE;

  /*  Set active layer, active channel  */
  if (active_layer)
    gimp_image_set_active_layer (new_gimage, active_layer);

  if (active_channel)
    gimp_image_set_active_channel (new_gimage, active_channel);

  if (floating_layer)
    floating_sel_attach (floating_layer, new_floating_sel_drawable);

  /*  Copy the colormap if necessary  */
  if (new_gimage->base_type == INDEXED)
    memcpy (new_gimage->cmap, gimage->cmap, gimage->num_cols * 3);
  new_gimage->num_cols = gimage->num_cols;

  /*  copy state of all color channels  */
  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_gimage->visible[count] = gimage->visible[count];
      new_gimage->active[count] = gimage->active[count];
    }

  /*  Copy any Guides  */
  for (list = gimage->guides; list; list = g_list_next (list))
    {
      Guide* new_guide;

      guide = (Guide*) list->data;

      switch (guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  new_guide = gimp_image_add_hguide (new_gimage);
	  new_guide->position = guide->position;
	  break;
	case ORIENTATION_VERTICAL:
	  new_guide = gimp_image_add_vguide (new_gimage);
	  new_guide->position = guide->position;
	  break;
	default:
	  g_error ("Unknown guide orientation.\n");
	}
    }

  /* Copy the qmask info */
  new_gimage->qmask_state = gimage->qmask_state;
  new_gimage->qmask_color = gimage->qmask_color;

  /* Copy parasites */
  parasites = gimage->parasites;
  if (parasites)
    new_gimage->parasites = parasite_list_copy (parasites);

  /* Copy paths */
  paths = gimp_image_get_paths (gimage);
  if (paths)
    {
      GSList   *plist = NULL;
      GSList   *new_plist = NULL;
      Path     *path;
      PathList *new_paths;
      
      for (plist = paths->bz_paths; plist; plist = plist->next)
	{
	  path = plist->data;
	  new_plist = g_slist_append (new_plist, path_copy (new_gimage, path));
	}
      
      new_paths = path_list_new (new_gimage, paths->last_selected_row, new_plist);
      gimp_image_set_paths (new_gimage, new_paths);
    }

  gimp_image_undo_enable (new_gimage);

  return new_gimage;
}
