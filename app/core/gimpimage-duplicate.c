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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/pixel-region.h"

#include "config/gimpconfig.h"

#include "paint-funcs/paint-funcs.h"

#include "vectors/gimpvectors.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpgrid.h"
#include "gimpimage.h"
#include "gimpimage-duplicate.h"
#include "gimpimage-guides.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplist.h"
#include "gimpparasitelist.h"

#include "gimp-intl.h"


GimpImage *
gimp_image_duplicate (GimpImage *gimage)
{
  PixelRegion       srcPR, destPR;
  GimpImage        *new_gimage;
  GimpLayer        *new_layer;
  GimpLayer        *floating_layer;
  GimpChannel      *new_channel;
  GimpVectors      *new_vectors;
  GList            *list;
  GimpLayer        *active_layer              = NULL;
  GimpChannel      *active_channel            = NULL;
  GimpVectors      *active_vectors            = NULL;
  GimpDrawable     *new_floating_sel_drawable = NULL;
  GimpDrawable     *floating_sel_drawable     = NULL;
  GimpParasiteList *parasites;
  gint              count;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  gimp_set_busy_until_idle (gimage->gimp);

  /*  Create a new image  */
  new_gimage = gimp_create_image (gimage->gimp,
				  gimage->width, gimage->height,
				  gimage->base_type,
				  FALSE);
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
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      new_layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (layer),
                                                   G_TYPE_FROM_INSTANCE (layer),
                                                   FALSE));

      gimp_item_set_image (GIMP_ITEM (new_layer), new_gimage);

      /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_layer),
			    gimp_object_get_name (GIMP_OBJECT (layer)));

      /*  Make sure that if the layer has a layer mask,
          it's name isn't screwed up  */
      if (new_layer->mask)
        gimp_object_set_name (GIMP_OBJECT (new_layer->mask),
                              gimp_object_get_name (GIMP_OBJECT(layer->mask)));

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

      new_channel =
        GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (channel),
                                           G_TYPE_FROM_INSTANCE (channel),
                                           FALSE));

      gimp_item_set_image (GIMP_ITEM (new_channel), new_gimage);

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

      new_vectors =
        GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                           G_TYPE_FROM_INSTANCE (vectors),
                                           FALSE));

      gimp_item_set_image (GIMP_ITEM (new_vectors), new_gimage);

      /*  Make sure the copied vectors doesn't say: "<old vectors> copy"  */
      gimp_object_set_name (GIMP_OBJECT (new_vectors),
			    gimp_object_get_name (GIMP_OBJECT (vectors)));

      if (gimp_image_get_active_vectors (gimage) == vectors)
	active_vectors = new_vectors;

      gimp_image_add_vectors (new_gimage, new_vectors, count++);
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

  /*  Set active layer, active channel, active vectors  */
  if (active_layer)
    gimp_image_set_active_layer (new_gimage, active_layer);

  if (active_channel)
    gimp_image_set_active_channel (new_gimage, active_channel);

  if (active_vectors)
    gimp_image_set_active_vectors (new_gimage, active_vectors);

  if (floating_layer)
    floating_sel_attach (floating_layer, new_floating_sel_drawable);

  /*  Copy the colormap if necessary  */
  if (new_gimage->base_type == GIMP_INDEXED)
    memcpy (new_gimage->cmap, gimage->cmap, gimage->num_cols * 3);

  new_gimage->num_cols = gimage->num_cols;

  /*  Copy state of all color channels  */
  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_gimage->visible[count] = gimage->visible[count];
      new_gimage->active[count] = gimage->active[count];
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

  /* Copy the grid */
  if (gimage->grid)
    new_gimage->grid = gimp_config_duplicate (GIMP_CONFIG (gimage->grid));

  /* Copy the qmask info */
  new_gimage->qmask_state = gimage->qmask_state;
  new_gimage->qmask_color = gimage->qmask_color;

  /* Copy parasites */
  parasites = gimage->parasites;
  if (parasites)
    {
      g_object_unref (new_gimage->parasites);
      new_gimage->parasites = gimp_parasite_list_copy (parasites);
    }

  gimp_image_undo_enable (new_gimage);

  return new_gimage;
}
