/* GIMP - The GNU Image Manipulation Program
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

#include "vectors/gimpvectors.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-colorhash.h"
#include "gimpimage-merge.h"
#include "gimpimage-undo.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimpundostack.h"

#include "gimp-intl.h"

static GimpLayer * gimp_image_merge_layers (GimpImage     *image,
                                            GSList        *merge_list,
                                            GimpContext   *context,
                                            GimpMergeType  merge_type,
                                            const gchar   *undo_desc);


/*  public functions  */

GimpLayer *
gimp_image_merge_visible_layers (GimpImage     *image,
                                 GimpContext   *context,
                                 GimpMergeType  merge_type,
                                 gboolean       discard_invisible)
{
  GList     *list;
  GSList    *merge_list       = NULL;
  GSList    *invisible_list   = NULL;
  gboolean   had_floating_sel = FALSE;
  GimpLayer *layer            = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  /* if there's a floating selection, anchor it */
  if (gimp_image_floating_sel (image))
    {
      floating_sel_anchor (image->floating_sel);
      had_floating_sel = TRUE;
    }

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        {
          merge_list = g_slist_append (merge_list, layer);
        }
      else if (discard_invisible)
        {
          invisible_list = g_slist_append (invisible_list, layer);
        }
    }

  if (merge_list)
    {
      const gchar *undo_desc = _("Merge Visible Layers");

      gimp_set_busy (image->gimp);

      if (invisible_list)
        {
          gimp_image_undo_group_start (image,
                                       GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                       undo_desc);
        }

      layer = gimp_image_merge_layers (image, merge_list, context, merge_type,
                                       _("Merge Visible Layers"));
      g_slist_free (merge_list);

      if (invisible_list)
        {
          GSList *list;

          for (list = invisible_list; list; list = g_slist_next (list))
            gimp_image_remove_layer (image, list->data);

          gimp_image_undo_group_end (image);
          g_slist_free (invisible_list);
        }


      gimp_unset_busy (image->gimp);

      return layer;
    }

  return image->active_layer;
}

GimpLayer *
gimp_image_flatten (GimpImage   *image,
                    GimpContext *context)
{
  GList     *list;
  GSList    *merge_list = NULL;
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  gimp_set_busy (image->gimp);

  /* if there's a floating selection, anchor it */
  if (gimp_image_floating_sel (image))
    floating_sel_anchor (image->floating_sel);

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        merge_list = g_slist_append (merge_list, layer);
    }

  layer = gimp_image_merge_layers (image, merge_list, context,
                                   GIMP_FLATTEN_IMAGE, _("Flatten Image"));
  g_slist_free (merge_list);

  gimp_image_alpha_changed (image);

  gimp_unset_busy (image->gimp);

  return layer;
}

GimpLayer *
gimp_image_merge_down (GimpImage     *image,
                       GimpLayer     *current_layer,
                       GimpContext   *context,
                       GimpMergeType  merge_type)
{
  GimpLayer *layer;
  GList     *list;
  GList     *layer_list;
  GSList    *merge_list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  for (list = GIMP_LIST (image->layers)->list, layer_list = NULL;
       list && !layer_list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (layer == current_layer)
        break;
    }

  for (layer_list = g_list_next (list), merge_list = NULL;
       layer_list && !merge_list;
       layer_list = g_list_next (layer_list))
    {
      layer = layer_list->data;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        merge_list = g_slist_append (NULL, layer);
    }

  merge_list = g_slist_prepend (merge_list, current_layer);

  gimp_set_busy (image->gimp);

  layer = gimp_image_merge_layers (image, merge_list, context, merge_type,
                                   _("Merge Down"));
  g_slist_free (merge_list);

  gimp_unset_busy (image->gimp);

  return layer;
}

/* merging vectors */

GimpVectors *
gimp_image_merge_visible_vectors (GimpImage *image)
{
  GList       *list           = NULL;
  GSList      *merge_list     = NULL;
  GSList      *cur_item       = NULL;
  GimpVectors *vectors        = NULL;
  GimpVectors *target_vectors = NULL;
  gchar       *name           = NULL;
  gint         pos            = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  for (list = GIMP_LIST (image->vectors)->list;
       list;
       list = g_list_next (list))
    {
      vectors = list->data;

      if (gimp_item_get_visible (GIMP_ITEM (vectors)))
        merge_list = g_slist_append (merge_list, vectors);
    }

  if (merge_list && merge_list->next)
    {
      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE,
                                   _("Merge Visible Paths"));

      cur_item = merge_list;
      vectors = GIMP_VECTORS (cur_item->data);

      name = g_strdup (gimp_object_get_name (GIMP_OBJECT (vectors)));
      target_vectors = GIMP_VECTORS (
                            gimp_item_duplicate (GIMP_ITEM (vectors),
                                                 GIMP_TYPE_VECTORS,
                                                 FALSE));
      pos = gimp_image_get_vectors_index (image, vectors);
      gimp_image_remove_vectors (image, vectors);
      cur_item = cur_item->next;

      while (cur_item)
        {
          vectors = GIMP_VECTORS (cur_item->data);
          gimp_vectors_add_strokes (vectors, target_vectors);
          gimp_image_remove_vectors (image, vectors);

          cur_item = g_slist_next (cur_item);
        }

      gimp_object_take_name (GIMP_OBJECT (target_vectors), name);

      g_slist_free (merge_list);

      gimp_image_add_vectors (image, target_vectors, pos);
      gimp_unset_busy (image->gimp);

      gimp_image_undo_group_end (image);

      return target_vectors;
    }
  else
    {
      gimp_message (image->gimp, NULL, GIMP_MESSAGE_WARNING,
                    _("Not enough visible paths for a merge. "
                      "There must be at least two."));
      return NULL;
    }
}


/*  private functions  */

static GimpLayer *
gimp_image_merge_layers (GimpImage     *image,
                         GSList        *merge_list,
                         GimpContext   *context,
                         GimpMergeType  merge_type,
                         const gchar   *undo_desc)
{
  GList           *list;
  GSList          *reverse_list = NULL;
  PixelRegion      src1PR, src2PR, maskPR;
  PixelRegion     *mask;
  GimpLayer       *merge_layer;
  GimpLayer       *layer;
  GimpLayer       *bottom_layer;
  guchar           bg[4] = {0, 0, 0, 0};
  GimpImageType    type;
  gint             count;
  gint             x1, y1, x2, y2;
  gint             x3, y3, x4, y4;
  CombinationMode  operation;
  gint             position;
  gboolean         active[MAX_CHANNELS] = { TRUE, TRUE, TRUE, TRUE };
  gint             off_x, off_y;
  gchar           *name;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  layer        = NULL;
  type         = GIMP_RGBA_IMAGE;
  x1 = y1      = 0;
  x2 = y2      = 0;
  bottom_layer = NULL;

  /*  Get the layer extents  */
  count = 0;
  while (merge_list)
    {
      layer = merge_list->data;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      switch (merge_type)
        {
        case GIMP_EXPAND_AS_NECESSARY:
        case GIMP_CLIP_TO_IMAGE:
          if (! count)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + gimp_item_width  (GIMP_ITEM (layer));
              y2 = off_y + gimp_item_height (GIMP_ITEM (layer));
            }
          else
            {
              if (off_x < x1)
                x1 = off_x;
              if (off_y < y1)
                y1 = off_y;
              if ((off_x + gimp_item_width (GIMP_ITEM (layer))) > x2)
                x2 = (off_x + gimp_item_width (GIMP_ITEM (layer)));
              if ((off_y + gimp_item_height (GIMP_ITEM (layer))) > y2)
                y2 = (off_y + gimp_item_height (GIMP_ITEM (layer)));
            }

          if (merge_type == GIMP_CLIP_TO_IMAGE)
            {
              x1 = CLAMP (x1, 0, image->width);
              y1 = CLAMP (y1, 0, image->height);
              x2 = CLAMP (x2, 0, image->width);
              y2 = CLAMP (y2, 0, image->height);
            }
          break;

        case GIMP_CLIP_TO_BOTTOM_LAYER:
          if (merge_list->next == NULL)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + gimp_item_width (GIMP_ITEM (layer));
              y2 = off_y + gimp_item_height (GIMP_ITEM (layer));
            }
          break;

        case GIMP_FLATTEN_IMAGE:
          if (merge_list->next == NULL)
            {
              x1 = 0;
              y1 = 0;
              x2 = image->width;
              y2 = image->height;
            }
          break;
        }

      count ++;
      reverse_list = g_slist_prepend (reverse_list, layer);
      merge_list = g_slist_next (merge_list);
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    return NULL;

  /*  Start a merge undo group. */

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  name = g_strdup (gimp_object_get_name (GIMP_OBJECT (layer)));

  if (merge_type == GIMP_FLATTEN_IMAGE ||
      gimp_drawable_type (GIMP_DRAWABLE (layer)) == GIMP_INDEXED_IMAGE)
    {
      type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (gimp_image_base_type (image));

      merge_layer = gimp_layer_new (image, (x2 - x1), (y2 - y1),
                                    type,
                                    gimp_object_get_name (GIMP_OBJECT (layer)),
                                    GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer.", G_STRFUNC);
          return NULL;
        }

      GIMP_ITEM (merge_layer)->offset_x = x1;
      GIMP_ITEM (merge_layer)->offset_y = y1;

      /*  get the background for compositing  */
      gimp_image_get_background (image, context,
                                 gimp_drawable_type (GIMP_DRAWABLE (merge_layer)),
                                 bg);

      /*  init the pixel region  */
      pixel_region_init (&src1PR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (merge_layer)),
                         0, 0, (x2 - x1), (y2 - y1),
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
        gimp_layer_new (image, (x2 - x1), (y2 - y1),
                        gimp_drawable_type_with_alpha (GIMP_DRAWABLE (layer)),
                        "merged layer",
                        GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

      if (!merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);
          return NULL;
        }

      GIMP_ITEM (merge_layer)->offset_x = x1;
      GIMP_ITEM (merge_layer)->offset_y = y1;

      /*  Set the layer to transparent  */
      pixel_region_init (&src1PR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (merge_layer)),
                         0, 0,
                         (x2 - x1), (y2 - y1),
                         TRUE);

      /*  set the region to 0's  */
      color_region (&src1PR, bg);

      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      layer = reverse_list->data;
      position =
        gimp_container_num_children (image->layers) -
        gimp_container_get_child_index (image->layers, GIMP_OBJECT (layer));
    }

  bottom_layer = layer;

  /* Copy the tattoo and parasites of the bottom layer to the new layer */
  gimp_item_set_tattoo (GIMP_ITEM (merge_layer),
                        gimp_item_get_tattoo (GIMP_ITEM (bottom_layer)));

  g_object_unref (GIMP_ITEM (merge_layer)->parasites);
  GIMP_ITEM (merge_layer)->parasites =
    gimp_parasite_list_copy (GIMP_ITEM (bottom_layer)->parasites);

  while (reverse_list)
    {
      GimpLayerModeEffects  mode;

      layer = reverse_list->data;

      /*  determine what sort of operation is being attempted and
       *  if it's actually legal...
       */
      operation =
        gimp_image_get_combination_mode (gimp_drawable_type (GIMP_DRAWABLE (merge_layer)),
                                         gimp_drawable_bytes (GIMP_DRAWABLE (layer)));

      if (operation == -1)
        {
          g_warning ("%s: attempting to merge incompatible layers.", G_STRFUNC);
          return NULL;
        }

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      x3 = CLAMP (off_x, x1, x2);
      y3 = CLAMP (off_y, y1, y2);
      x4 = CLAMP (off_x + gimp_item_width  (GIMP_ITEM (layer)), x1, x2);
      y4 = CLAMP (off_y + gimp_item_height (GIMP_ITEM (layer)), y1, y2);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (merge_layer)),
                         (x3 - x1), (y3 - y1), (x4 - x3), (y4 - y3),
                         TRUE);
      pixel_region_init (&src2PR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                         (x3 - off_x), (y3 - off_y),
                         (x4 - x3), (y4 - y3),
                         FALSE);

      if (layer->mask && layer->mask->apply_mask)
        {
          TileManager *tiles;

          tiles = gimp_drawable_get_tiles (GIMP_DRAWABLE (layer->mask));

          pixel_region_init (&maskPR, tiles,
                             (x3 - off_x), (y3 - off_y), (x4 - x3), (y4 - y3),
                             FALSE);
          mask = &maskPR;
        }
      else
        {
          mask = NULL;
        }

      /* DISSOLVE_MODE is special since it is the only mode that does not
       *  work on the projection with the lower layer, but only locally on
       *  the layers alpha channel.
       */
      mode = layer->mode;
      if (layer == bottom_layer && mode != GIMP_DISSOLVE_MODE)
        mode = GIMP_NORMAL_MODE;

      combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL,
                       layer->opacity * 255.999,
                       mode,
                       active,
                       operation);

      gimp_image_remove_layer (image, layer);

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == GIMP_FLATTEN_IMAGE)
    {
      list = GIMP_LIST (image->layers)->list;
      while (list)
        {
          layer = list->data;

          list = g_list_next (list);
          gimp_image_remove_layer (image, layer);
        }

      gimp_image_add_layer (image, merge_layer, position);
    }
  else
    {
      /*  Add the layer to the image  */
      gimp_image_add_layer (image, merge_layer,
         gimp_container_num_children (image->layers) - position + 1);
    }

  /* set the name after the original layers have been removed so we
   * don't end up with #2 appended to the name
   */
  gimp_object_take_name (GIMP_OBJECT (merge_layer), name);

  gimp_item_set_visible (GIMP_ITEM (merge_layer), TRUE, TRUE);

  /*  End the merge undo group  */
  gimp_image_undo_group_end (image);

  gimp_drawable_update (GIMP_DRAWABLE (merge_layer),
                        0, 0,
                        gimp_item_width  (GIMP_ITEM (merge_layer)),
                        gimp_item_height (GIMP_ITEM (merge_layer)));

  return merge_layer;
}
