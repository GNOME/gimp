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

#include <glib-object.h>

#include "core-types.h"

#include "base/pixel-region.h"

#include "config/gimpconfig.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-duplicate.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"
#include "gimpparasitelist.h"

#include "vectors/gimpvectors.h"


GimpImage *
gimp_image_duplicate (GimpImage *gimage)
{
  GimpImage    *new_gimage;
  GimpLayer    *floating_layer;
  GList        *list;
  GimpLayer    *active_layer              = NULL;
  GimpChannel  *active_channel            = NULL;
  GimpVectors  *active_vectors            = NULL;
  GimpDrawable *new_floating_sel_drawable = NULL;
  GimpDrawable *floating_sel_drawable     = NULL;
  gint          count;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  gimp_set_busy_until_idle (gimage->gimp);

  /*  Create a new image  */
  new_gimage = gimp_create_image (gimage->gimp,
				  gimage->width, gimage->height,
				  gimage->base_type,
				  FALSE);
  gimp_image_undo_disable (new_gimage);

  /*  Copy the colormap if necessary  */
  if (new_gimage->base_type == GIMP_INDEXED)
    gimp_image_set_colormap (new_gimage,
                             gimp_image_get_colormap (gimage),
                             gimp_image_get_colormap_size (gimage),
                             FALSE);

  /*  Copy resolution information  */
  new_gimage->xresolution     = gimage->xresolution;
  new_gimage->yresolution     = gimage->yresolution;
  new_gimage->resolution_unit = gimage->resolution_unit;

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
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      GimpLayer *new_layer;

      new_layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (layer),
                                                 new_gimage,
                                                 G_TYPE_FROM_INSTANCE (layer),
                                                 FALSE));

      /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_layer),
			    gimp_object_get_name (GIMP_OBJECT (layer)));

      /*  Make sure that if the layer has a layer mask,
       *  its name isn't screwed up
       */
      if (new_layer->mask)
        gimp_object_set_name (GIMP_OBJECT (new_layer->mask),
                              gimp_object_get_name (GIMP_OBJECT (layer->mask)));

      if (gimp_image_get_active_layer (gimage) == layer)
	active_layer = new_layer;

      if (gimage->floating_sel == layer)
	floating_layer = new_layer;

      if (floating_sel_drawable == GIMP_DRAWABLE (layer))
	new_floating_sel_drawable = GIMP_DRAWABLE (new_layer);

      if (floating_layer != new_layer)
	gimp_image_add_layer (new_gimage, new_layer, count++);
    }

  /*  Copy the channels  */
  for (list = GIMP_LIST (gimage->channels)->list, count = 0;
       list;
       list = g_list_next (list))
    {
      GimpChannel *channel = list->data;
      GimpChannel *new_channel;

      new_channel =
        GIMP_CHANNEL (gimp_item_convert (GIMP_ITEM (channel),
                                         new_gimage,
                                         G_TYPE_FROM_INSTANCE (channel),
                                         FALSE));

      /*  Make sure the copied channel doesn't say: "<old channel> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_channel),
			    gimp_object_get_name (GIMP_OBJECT (channel)));

      if (gimp_image_get_active_channel (gimage) == channel)
	active_channel = (new_channel);

      if (floating_sel_drawable == GIMP_DRAWABLE (channel))
	new_floating_sel_drawable = GIMP_DRAWABLE (new_channel);

      gimp_image_add_channel (new_gimage, new_channel, count++);
    }

  /*  Copy any vectors  */
  for (list = GIMP_LIST (gimage->vectors)->list, count = 0;
       list;
       list = g_list_next (list))
    {
      GimpVectors *vectors = list->data;
      GimpVectors *new_vectors;

      new_vectors =
        GIMP_VECTORS (gimp_item_convert (GIMP_ITEM (vectors),
                                         new_gimage,
                                         G_TYPE_FROM_INSTANCE (vectors),
                                         FALSE));

      /*  Make sure the copied vectors doesn't say: "<old vectors> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_vectors),
			    gimp_object_get_name (GIMP_OBJECT (vectors)));

      if (gimp_image_get_active_vectors (gimage) == vectors)
	active_vectors = new_vectors;

      gimp_image_add_vectors (new_gimage, new_vectors, count++);
    }

  /*  Copy the selection mask  */
  {
    TileManager *src_tiles;
    TileManager *dest_tiles;
    PixelRegion  srcPR, destPR;

    src_tiles  = gimp_drawable_data (GIMP_DRAWABLE (gimage->selection_mask));
    dest_tiles = gimp_drawable_data (GIMP_DRAWABLE (new_gimage->selection_mask));

    pixel_region_init (&srcPR, src_tiles,
                       0, 0, gimage->width, gimage->height, FALSE);
    pixel_region_init (&destPR, dest_tiles,
                       0, 0, gimage->width, gimage->height, TRUE);

    copy_region (&srcPR, &destPR);

    new_gimage->selection_mask->bounds_known   = FALSE;
    new_gimage->selection_mask->boundary_known = FALSE;
  }

  if (floating_layer)
    floating_sel_attach (floating_layer, new_floating_sel_drawable);

  /*  Set active layer, active channel, active vectors  */
  if (active_layer)
    gimp_image_set_active_layer (new_gimage, active_layer);

  if (active_channel)
    gimp_image_set_active_channel (new_gimage, active_channel);

  if (active_vectors)
    gimp_image_set_active_vectors (new_gimage, active_vectors);

  /*  Copy state of all color channels  */
  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_gimage->visible[count] = gimage->visible[count];
      new_gimage->active[count]  = gimage->active[count];
    }

  /*  Copy any guides  */
  for (list = gimage->guides; list; list = g_list_next (list))
    {
      GimpGuide *guide = list->data;

      switch (guide->orientation)
	{
	case GIMP_ORIENTATION_HORIZONTAL:
	  gimp_image_add_hguide (new_gimage, guide->position, FALSE);
	  break;

	case GIMP_ORIENTATION_VERTICAL:
	  gimp_image_add_vguide (new_gimage, guide->position, FALSE);
	  break;

	default:
	  g_error ("Unknown guide orientation.\n");
	}
    }

  /*  Copy the grid  */
  if (gimage->grid)
    gimp_image_set_grid (new_gimage, gimage->grid, FALSE);

  /*  Copy the qmask info  */
  new_gimage->qmask_state = gimage->qmask_state;
  new_gimage->qmask_color = gimage->qmask_color;

  /*  Copy parasites  */
  if (gimage->parasites)
    {
      g_object_unref (new_gimage->parasites);
      new_gimage->parasites = gimp_parasite_list_copy (gimage->parasites);
    }

  gimp_image_undo_enable (new_gimage);

  return new_gimage;
}
