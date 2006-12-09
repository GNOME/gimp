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

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-duplicate.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-sample-points.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"
#include "gimpparasitelist.h"

#include "vectors/gimpvectors.h"


GimpImage *
gimp_image_duplicate (GimpImage *image)
{
  GimpImage    *new_image;
  GimpLayer    *floating_layer;
  GList        *list;
  GimpLayer    *active_layer              = NULL;
  GimpChannel  *active_channel            = NULL;
  GimpVectors  *active_vectors            = NULL;
  GimpDrawable *new_floating_sel_drawable = NULL;
  GimpDrawable *floating_sel_drawable     = NULL;
  gchar        *filename;
  gint          count;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  gimp_set_busy_until_idle (image->gimp);

  /*  Create a new image  */
  new_image = gimp_create_image (image->gimp,
                                 image->width, image->height,
                                 image->base_type,
                                 FALSE);
  gimp_image_undo_disable (new_image);

  /*  Store the folder to be used by the save dialog  */
  filename = gimp_image_get_filename (image);
  if (filename)
    {
      g_object_set_data_full (G_OBJECT (new_image), "gimp-image-dirname",
                              g_path_get_dirname (filename),
                              (GDestroyNotify) g_free);
      g_free (filename);
    }

  /*  Copy the colormap if necessary  */
  if (new_image->base_type == GIMP_INDEXED)
    gimp_image_set_colormap (new_image,
                             gimp_image_get_colormap (image),
                             gimp_image_get_colormap_size (image),
                             FALSE);

  /*  Copy resolution information  */
  new_image->xresolution     = image->xresolution;
  new_image->yresolution     = image->yresolution;
  new_image->resolution_unit = image->resolution_unit;

  /*  Copy floating layer  */
  floating_layer = gimp_image_floating_sel (image);
  if (floating_layer)
    {
      floating_sel_relax (floating_layer, FALSE);

      floating_sel_drawable = floating_layer->fs.drawable;
      floating_layer = NULL;
    }

  /*  Copy the layers  */
  for (list = GIMP_LIST (image->layers)->list, count = 0;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      GimpLayer *new_layer;

      new_layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (layer),
                                                 new_image,
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

      if (gimp_image_get_active_layer (image) == layer)
        active_layer = new_layer;

      if (image->floating_sel == layer)
        floating_layer = new_layer;

      if (floating_sel_drawable == GIMP_DRAWABLE (layer))
        new_floating_sel_drawable = GIMP_DRAWABLE (new_layer);

      if (floating_layer != new_layer)
        gimp_image_add_layer (new_image, new_layer, count++);
    }

  /*  Copy the channels  */
  for (list = GIMP_LIST (image->channels)->list, count = 0;
       list;
       list = g_list_next (list))
    {
      GimpChannel *channel = list->data;
      GimpChannel *new_channel;

      new_channel =
        GIMP_CHANNEL (gimp_item_convert (GIMP_ITEM (channel),
                                         new_image,
                                         G_TYPE_FROM_INSTANCE (channel),
                                         FALSE));

      /*  Make sure the copied channel doesn't say: "<old channel> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_channel),
                            gimp_object_get_name (GIMP_OBJECT (channel)));

      if (gimp_image_get_active_channel (image) == channel)
        active_channel = (new_channel);

      if (floating_sel_drawable == GIMP_DRAWABLE (channel))
        new_floating_sel_drawable = GIMP_DRAWABLE (new_channel);

      gimp_image_add_channel (new_image, new_channel, count++);
    }

  /*  Copy any vectors  */
  for (list = GIMP_LIST (image->vectors)->list, count = 0;
       list;
       list = g_list_next (list))
    {
      GimpVectors *vectors = list->data;
      GimpVectors *new_vectors;

      new_vectors =
        GIMP_VECTORS (gimp_item_convert (GIMP_ITEM (vectors),
                                         new_image,
                                         G_TYPE_FROM_INSTANCE (vectors),
                                         FALSE));

      /*  Make sure the copied vectors doesn't say: "<old vectors> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_vectors),
                            gimp_object_get_name (GIMP_OBJECT (vectors)));

      if (gimp_image_get_active_vectors (image) == vectors)
        active_vectors = new_vectors;

      gimp_image_add_vectors (new_image, new_vectors, count++);
    }

  /*  Copy the selection mask  */
  {
    TileManager *src_tiles;
    TileManager *dest_tiles;
    PixelRegion  srcPR, destPR;

    src_tiles  =
      gimp_drawable_get_tiles (GIMP_DRAWABLE (image->selection_mask));
    dest_tiles =
      gimp_drawable_get_tiles (GIMP_DRAWABLE (new_image->selection_mask));

    pixel_region_init (&srcPR, src_tiles,
                       0, 0, image->width, image->height, FALSE);
    pixel_region_init (&destPR, dest_tiles,
                       0, 0, image->width, image->height, TRUE);

    copy_region (&srcPR, &destPR);

    new_image->selection_mask->bounds_known   = FALSE;
    new_image->selection_mask->boundary_known = FALSE;
  }

  if (floating_layer)
    floating_sel_attach (floating_layer, new_floating_sel_drawable);

  /*  Set active layer, active channel, active vectors  */
  if (active_layer)
    gimp_image_set_active_layer (new_image, active_layer);

  if (active_channel)
    gimp_image_set_active_channel (new_image, active_channel);

  if (active_vectors)
    gimp_image_set_active_vectors (new_image, active_vectors);

  /*  Copy state of all color channels  */
  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_image->visible[count] = image->visible[count];
      new_image->active[count]  = image->active[count];
    }

  /*  Copy any guides  */
  for (list = image->guides; list; list = g_list_next (list))
    {
      GimpGuide *guide    = list->data;
      gint       position = gimp_guide_get_position (guide);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          gimp_image_add_hguide (new_image, position, FALSE);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          gimp_image_add_vguide (new_image, position, FALSE);
          break;

        default:
          g_error ("Unknown guide orientation.\n");
        }
    }

  /*  Copy any sample points  */
  for (list = image->sample_points; list; list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;

      gimp_image_add_sample_point_at_pos (new_image,
                                          sample_point->x,
                                          sample_point->y,
                                          FALSE);
    }

  /*  Copy the grid  */
  if (image->grid)
    gimp_image_set_grid (new_image, image->grid, FALSE);

  /*  Copy the quick mask info  */
  new_image->quick_mask_state    = image->quick_mask_state;
  new_image->quick_mask_inverted = image->quick_mask_inverted;
  new_image->quick_mask_color    = image->quick_mask_color;

  /*  Copy parasites  */
  if (image->parasites)
    {
      g_object_unref (new_image->parasites);
      new_image->parasites = gimp_parasite_list_copy (image->parasites);
    }

  gimp_image_undo_enable (new_image);

  return new_image;
}
