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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpimage-colorhash.h"
#include "gimpimage-merge.h"
#include "gimpimage-projection.h"
#include "gimpimage-undo.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasite.h"
#include "gimpparasitelist.h"
#include "gimpundostack.h"

#include "floating_sel.h"
#include "path.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


/*  public functions  */

GimpLayer *
gimp_image_merge_visible_layers (GimpImage *gimage, 
				 MergeType  merge_type)
{
  GList     *list;
  GSList    *merge_list       = NULL;
  gboolean   had_floating_sel = FALSE;
  GimpLayer *layer            = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /* if there's a floating selection, anchor it */
  if (gimp_image_floating_sel (gimage))
    {
      floating_sel_anchor (gimage->floating_sel);
      had_floating_sel = TRUE;
    }

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      if (gimp_drawable_get_visible (GIMP_DRAWABLE (layer)))
	merge_list = g_slist_append (merge_list, layer);
    }

  if (merge_list && merge_list->next)
    {
      gimp_set_busy (gimage->gimp);

      layer = gimp_image_merge_layers (gimage, merge_list, merge_type);
      g_slist_free (merge_list);

      gimp_unset_busy (gimage->gimp);

      return layer;
    }
  else
    {
      g_slist_free (merge_list);

      /* If there was a floating selection, we have done something.
	 No need to warn the user. Return the active layer instead */
      if (had_floating_sel)
	return layer;
      else
	g_message (_("Not enough visible layers for a merge.\n"
		     "There must be at least two."));

      return NULL;
    }
}

GimpLayer *
gimp_image_flatten (GimpImage *gimage)
{
  GList     *list;
  GSList    *merge_list = NULL;
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  gimp_set_busy (gimage->gimp);

  /* if there's a floating selection, anchor it */
  if (gimp_image_floating_sel (gimage))
    floating_sel_anchor (gimage->floating_sel);

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      if (gimp_drawable_get_visible (GIMP_DRAWABLE (layer)))
	merge_list = g_slist_append (merge_list, layer);
    }

  layer = gimp_image_merge_layers (gimage, merge_list, FLATTEN_IMAGE);
  g_slist_free (merge_list);

  gimp_image_alpha_changed (gimage);

  gimp_unset_busy (gimage->gimp);

  return layer;
}

GimpLayer *
gimp_image_merge_down (GimpImage *gimage,
		       GimpLayer *current_layer,
		       MergeType  merge_type)
{
  GimpLayer *layer;
  GList     *list;
  GList     *layer_list;
  GSList    *merge_list;
  
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  for (list = GIMP_LIST (gimage->layers)->list, layer_list = NULL;
       list && !layer_list;
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;
      
      if (layer == current_layer)
	break;
    }
    
  for (layer_list = g_list_next (list), merge_list = NULL; 
       layer_list && !merge_list; 
       layer_list = g_list_next (layer_list))
    {
      layer = (GimpLayer *) layer_list->data;
      
      if (gimp_drawable_get_visible (GIMP_DRAWABLE (layer)))
	merge_list = g_slist_append (NULL, layer);
    }

  if (merge_list)
    {
      merge_list = g_slist_prepend (merge_list, current_layer);

      gimp_set_busy (gimage->gimp);

      layer = gimp_image_merge_layers (gimage, merge_list, merge_type);
      g_slist_free (merge_list);

      gimp_unset_busy (gimage->gimp);

      return layer;
    }
  else 
    {
      g_message (_("There are not enough visible layers for a merge down."));
      return NULL;
    }
}

GimpLayer *
gimp_image_merge_layers (GimpImage *gimage, 
			 GSList    *merge_list, 
			 MergeType  merge_type)
{
  GList                *list;
  GSList               *reverse_list = NULL;
  PixelRegion          src1PR, src2PR, maskPR;
  PixelRegion          *mask;
  GimpLayer            *merge_layer;
  GimpLayer            *layer;
  GimpLayer            *bottom_layer;
  GimpLayerModeEffects  bottom_mode;
  guchar                bg[4] = {0, 0, 0, 0};
  GimpImageType         type;
  gint                  count;
  gint                  x1, y1, x2, y2;
  gint                  x3, y3, x4, y4;
  gint                  operation;
  gint                  position;
  gint                  active[MAX_CHANNELS] = {1, 1, 1, 1};
  gint                  off_x, off_y;
  gchar                *name;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  layer        = NULL;
  type         = RGBA_GIMAGE;
  x1 = y1      = 0;
  x2 = y2      = 0;
  bottom_layer = NULL;
  bottom_mode  = GIMP_NORMAL_MODE;

  /*  Get the layer extents  */
  count = 0;
  while (merge_list)
    {
      layer = (GimpLayer *) merge_list->data;
      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      switch (merge_type)
	{
	case EXPAND_AS_NECESSARY:
	case CLIP_TO_IMAGE:
	  if (!count)
	    {
	      x1 = off_x;
	      y1 = off_y;
	      x2 = off_x + gimp_drawable_width (GIMP_DRAWABLE (layer));
	      y2 = off_y + gimp_drawable_height (GIMP_DRAWABLE (layer));
	    }
	  else
	    {
	      if (off_x < x1)
		x1 = off_x;
	      if (off_y < y1)
		y1 = off_y;
	      if ((off_x + gimp_drawable_width (GIMP_DRAWABLE (layer))) > x2)
		x2 = (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer)));
	      if ((off_y + gimp_drawable_height (GIMP_DRAWABLE (layer))) > y2)
		y2 = (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer)));
	    }
	  if (merge_type == CLIP_TO_IMAGE)
	    {
	      x1 = CLAMP (x1, 0, gimage->width);
	      y1 = CLAMP (y1, 0, gimage->height);
	      x2 = CLAMP (x2, 0, gimage->width);
	      y2 = CLAMP (y2, 0, gimage->height);
	    }
	  break;

	case CLIP_TO_BOTTOM_LAYER:
	  if (merge_list->next == NULL)
	    {
	      x1 = off_x;
	      y1 = off_y;
	      x2 = off_x + gimp_drawable_width (GIMP_DRAWABLE (layer));
	      y2 = off_y + gimp_drawable_height (GIMP_DRAWABLE (layer));
	    }
	  break;

	case FLATTEN_IMAGE:
	  if (merge_list->next == NULL)
	    {
	      x1 = 0;
	      y1 = 0;
	      x2 = gimage->width;
	      y2 = gimage->height;
	    }
	  break;
	}

      count ++;
      reverse_list = g_slist_prepend (reverse_list, layer);
      merge_list = g_slist_next (merge_list);
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    return NULL;

  /*  Start a merge undo group  */
  undo_push_group_start (gimage, LAYER_MERGE_UNDO);

  name = g_strdup (gimp_object_get_name (GIMP_OBJECT (layer)));

  if (merge_type == FLATTEN_IMAGE ||
      gimp_drawable_type (GIMP_DRAWABLE (layer)) == INDEXED_GIMAGE)
    {
      switch (gimp_image_base_type (gimage))
	{
	case GIMP_RGB: type = RGB_GIMAGE; break;
	case GIMP_GRAY: type = GRAY_GIMAGE; break;
	case GIMP_INDEXED: type = INDEXED_GIMAGE; break;
	}

      merge_layer = gimp_layer_new (gimage, (x2 - x1), (y2 - y1),
				    type,
				    gimp_object_get_name (GIMP_OBJECT (layer)),
				    OPAQUE_OPACITY, GIMP_NORMAL_MODE);
      if (!merge_layer)
	{
	  g_warning ("%s: could not allocate merge layer.",
		     G_GNUC_PRETTY_FUNCTION);
	  return NULL;
	}

      GIMP_DRAWABLE (merge_layer)->offset_x = x1;
      GIMP_DRAWABLE (merge_layer)->offset_y = y1;

      /*  get the background for compositing  */
      gimp_image_get_background (gimage, GIMP_DRAWABLE (merge_layer), bg);

      /*  init the pixel region  */
      pixel_region_init (&src1PR, 
			 gimp_drawable_data (GIMP_DRAWABLE (merge_layer)), 
			 0, 0, 
			 gimage->width, gimage->height, 
			 TRUE);

      /*  set the region to the background color  */
      color_region (&src1PR, bg);

      position = 0;
    }
  else
    {
      /*  The final merged layer inherits the name of the bottom most layer
       *  and the resulting layer has an alpha channel
       *  whether or not the original did
       *  Opacity is set to 100% and the MODE is set to normal
       */

      merge_layer =
	gimp_layer_new (gimage, (x2 - x1), (y2 - y1),
			gimp_drawable_type_with_alpha (GIMP_DRAWABLE (layer)),
			"merged layer",
			OPAQUE_OPACITY, GIMP_NORMAL_MODE);

      if (!merge_layer)
	{
	  g_warning ("%s: could not allocate merge layer",
		     G_GNUC_PRETTY_FUNCTION);
	  return NULL;
	}

      GIMP_DRAWABLE (merge_layer)->offset_x = x1;
      GIMP_DRAWABLE (merge_layer)->offset_y = y1;

      /*  Set the layer to transparent  */
      pixel_region_init (&src1PR, 
			 gimp_drawable_data (GIMP_DRAWABLE (merge_layer)), 
			 0, 0, 
			 (x2 - x1), (y2 - y1), 
			 TRUE);

      /*  set the region to 0's  */
      color_region (&src1PR, bg);

      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      layer = (GimpLayer *) reverse_list->data;
      position = 
	gimp_container_num_children (gimage->layers) - 
	gimp_container_get_child_index (gimage->layers, GIMP_OBJECT (layer));
      
      /* set the mode of the bottom layer to normal so that the contents
       *  aren't lost when merging with the all-alpha merge_layer
       *  Keep a pointer to it so that we can set the mode right after it's
       *  been merged so that undo works correctly.
       */
      bottom_layer = layer;
      bottom_mode  = bottom_layer->mode;

      /* DISSOLVE_MODE is special since it is the only mode that does not
       *  work on the projection with the lower layer, but only locally on
       *  the layers alpha channel. 
       */
      if (bottom_layer->mode != GIMP_DISSOLVE_MODE)
	gimp_layer_set_mode (bottom_layer, GIMP_NORMAL_MODE);
    }

  /* Copy the tattoo and parasites of the bottom layer to the new layer */
  gimp_drawable_set_tattoo (GIMP_DRAWABLE (merge_layer),
			    gimp_drawable_get_tattoo (GIMP_DRAWABLE (layer)));
  GIMP_DRAWABLE (merge_layer)->parasites =
    gimp_parasite_list_copy (GIMP_DRAWABLE (layer)->parasites);

  while (reverse_list)
    {
      layer = (GimpLayer *) reverse_list->data;

      /*  determine what sort of operation is being attempted and
       *  if it's actually legal...
       */
      operation = gimp_image_get_combination_mode (gimp_drawable_type (GIMP_DRAWABLE (merge_layer)),
                                                   gimp_drawable_bytes (GIMP_DRAWABLE (layer)));

      if (operation == -1)
	{
	  g_warning ("%s: attempting to merge incompatible layers.",
		     G_GNUC_PRETTY_FUNCTION);
	  return NULL;
	}

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
      x3 = CLAMP (off_x, x1, x2);
      y3 = CLAMP (off_y, y1, y2);
      x4 = CLAMP (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer)), x1, x2);
      y4 = CLAMP (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer)), y1, y2);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR, 
			 gimp_drawable_data (GIMP_DRAWABLE (merge_layer)), 
			 (x3 - x1), (y3 - y1), (x4 - x3), (y4 - y3), 
			 TRUE);
      pixel_region_init (&src2PR, 
			 gimp_drawable_data (GIMP_DRAWABLE (layer)), 
			 (x3 - off_x), (y3 - off_y),
			 (x4 - x3), (y4 - y3), 
			 FALSE);

      if (layer->mask && layer->mask->apply_mask)
	{
	  pixel_region_init (&maskPR, 
			     gimp_drawable_data (GIMP_DRAWABLE (layer->mask)), 
			     (x3 - off_x), (y3 - off_y),
			     (x4 - x3), (y4 - y3), 
			     FALSE);
	  mask = &maskPR;
	}
      else
	{
	  mask = NULL;
	}

      combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL,
		       layer->opacity, layer->mode, active, operation);

      gimp_image_remove_layer (gimage, layer);
      reverse_list = g_slist_next (reverse_list);
    }

  /* Save old mode in undo */
  if (bottom_layer)
    gimp_layer_set_mode (bottom_layer, bottom_mode);

  g_slist_free (reverse_list);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == FLATTEN_IMAGE)
    {
      list = GIMP_LIST (gimage->layers)->list;
      while (list)
	{
	  layer = (GimpLayer *) list->data;

	  list = g_list_next (list);
	  gimp_image_remove_layer (gimage, layer);
	}

      gimp_image_add_layer (gimage, merge_layer, position);
    }
  else
    {
      /*  Add the layer to the gimage  */
      gimp_image_add_layer (gimage, merge_layer,
	 gimp_container_num_children (gimage->layers) - position + 1);
    }

  /* set the name after the original layers have been removed so we
   * don't end up with #2 appended to the name
   */
  gimp_object_set_name (GIMP_OBJECT (merge_layer), name);
  g_free (name);

  /*  End the merge undo group  */
  undo_push_group_end (gimage);

  gimp_drawable_set_visible (GIMP_DRAWABLE (merge_layer), TRUE);

  gimp_drawable_update (GIMP_DRAWABLE (merge_layer), 
			0, 0, 
			gimp_drawable_width (GIMP_DRAWABLE (merge_layer)), 
			gimp_drawable_height (GIMP_DRAWABLE (merge_layer)));

  return merge_layer;
}
