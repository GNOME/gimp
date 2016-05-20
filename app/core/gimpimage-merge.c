/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-babl-compat.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-utils.h"

#include "vectors/gimpvectors.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimperror.h"
#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-merge.h"
#include "gimpimage-undo.h"
#include "gimpitemstack.h"
#include "gimplayer-floating-selection.h"
#include "gimplayer-new.h"
#include "gimplayermask.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimpundostack.h"

#include "gimp-intl.h"


static GimpLayer * gimp_image_merge_layers (GimpImage     *image,
                                            GimpContainer *container,
                                            GSList        *merge_list,
                                            GimpContext   *context,
                                            GimpMergeType  merge_type);


/*  public functions  */

GimpLayer *
gimp_image_merge_visible_layers (GimpImage     *image,
                                 GimpContext   *context,
                                 GimpMergeType  merge_type,
                                 gboolean       merge_active_group,
                                 gboolean       discard_invisible)
{
  GimpContainer *container;
  GList         *list;
  GSList        *merge_list     = NULL;
  GSList        *invisible_list = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (merge_active_group)
    {
      GimpLayer *active_layer = gimp_image_get_active_layer (image);

      /*  if the active layer is the floating selection, get the
       *  underlying drawable, but only if it is a layer
       */
      if (active_layer && gimp_layer_is_floating_sel (active_layer))
        {
          GimpDrawable *fs_drawable;

          fs_drawable = gimp_layer_get_floating_sel_drawable (active_layer);

          if (GIMP_IS_LAYER (fs_drawable))
            active_layer = GIMP_LAYER (fs_drawable);
        }

      if (active_layer)
        container = gimp_item_get_container (GIMP_ITEM (active_layer));
      else
        container = gimp_image_get_layers (image);
    }
  else
    {
      container = gimp_image_get_layers (image);
    }

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (container));
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      if (gimp_layer_is_floating_sel (layer))
        continue;

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
      GimpLayer *layer;

      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   C_("undo-type", "Merge Visible Layers"));

      /* if there's a floating selection, anchor it */
      if (gimp_image_get_floating_selection (image))
        floating_sel_anchor (gimp_image_get_floating_selection (image));

      layer = gimp_image_merge_layers (image,
                                       container,
                                       merge_list, context, merge_type);
      g_slist_free (merge_list);

      if (invisible_list)
        {
          GSList *list;

          for (list = invisible_list; list; list = g_slist_next (list))
            gimp_image_remove_layer (image, list->data, TRUE, NULL);

          g_slist_free (invisible_list);
        }

      gimp_image_undo_group_end (image);

      gimp_unset_busy (image->gimp);

      return layer;
    }

  return gimp_image_get_active_layer (image);
}

GimpLayer *
gimp_image_flatten (GimpImage    *image,
                    GimpContext  *context,
                    GError      **error)
{
  GList     *list;
  GSList    *merge_list = NULL;
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = gimp_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (gimp_layer_is_floating_sel (layer))
        continue;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        merge_list = g_slist_append (merge_list, layer);
    }

  if (merge_list)
    {
      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   C_("undo-type", "Flatten Image"));

      /* if there's a floating selection, anchor it */
      if (gimp_image_get_floating_selection (image))
        floating_sel_anchor (gimp_image_get_floating_selection (image));

      layer = gimp_image_merge_layers (image,
                                       gimp_image_get_layers (image),
                                       merge_list, context,
                                       GIMP_FLATTEN_IMAGE);
      g_slist_free (merge_list);

      gimp_image_alpha_changed (image);

      gimp_image_undo_group_end (image);

      gimp_unset_busy (image->gimp);

      return layer;
    }

  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                       _("Cannot flatten an image without any visible layer."));
  return NULL;
}

GimpLayer *
gimp_image_merge_down (GimpImage      *image,
                       GimpLayer      *current_layer,
                       GimpContext    *context,
                       GimpMergeType   merge_type,
                       GError        **error)
{
  GimpLayer *layer;
  GList     *list;
  GList     *layer_list = NULL;
  GSList    *merge_list = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (current_layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (current_layer)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = gimp_item_get_container_iter (GIMP_ITEM (current_layer));
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (layer == current_layer)
        break;
    }

  for (layer_list = g_list_next (list);
       layer_list;
       layer_list = g_list_next (layer_list))
    {
      layer = layer_list->data;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        {
          if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
            {
              g_set_error_literal (error, 0, 0,
                                   _("Cannot merge down to a layer group."));
              return NULL;
            }

          if (gimp_item_is_content_locked (GIMP_ITEM (layer)))
            {
              g_set_error_literal (error, 0, 0,
                                   _("The layer to merge down to is locked."));
              return NULL;
            }

          merge_list = g_slist_append (NULL, layer);
          break;
        }
    }

  if (! merge_list)
    {
      g_set_error_literal (error, 0, 0,
                           _("There is no visible layer to merge down to."));
      return NULL;
    }

  merge_list = g_slist_prepend (merge_list, current_layer);

  gimp_set_busy (image->gimp);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               C_("undo-type", "Merge Down"));

  layer = gimp_image_merge_layers (image,
                                   gimp_item_get_container (GIMP_ITEM (current_layer)),
                                   merge_list, context, merge_type);
  g_slist_free (merge_list);

  gimp_image_undo_group_end (image);

  gimp_unset_busy (image->gimp);

  return layer;
}

GimpLayer *
gimp_image_merge_group_layer (GimpImage      *image,
                              GimpGroupLayer *group)
{
  GimpLayer *parent;
  GimpLayer *layer;
  gint       index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);
  g_return_val_if_fail (gimp_item_get_image (GIMP_ITEM (group)) == image, NULL);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               C_("undo-type", "Merge Layer Group"));

  parent = gimp_layer_get_parent (GIMP_LAYER (group));
  index  = gimp_item_get_index (GIMP_ITEM (group));

  layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (group),
                                           GIMP_TYPE_LAYER));

  gimp_object_set_name (GIMP_OBJECT (layer), gimp_object_get_name (group));

  gimp_image_remove_layer (image, GIMP_LAYER (group), TRUE, NULL);
  gimp_image_add_layer (image, layer, parent, index, TRUE);

  gimp_image_undo_group_end (image);

  return layer;
}


/* merging vectors */

GimpVectors *
gimp_image_merge_visible_vectors (GimpImage  *image,
                                  GError    **error)
{
  GList       *list;
  GList       *merge_list = NULL;
  GimpVectors *vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = gimp_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      vectors = list->data;

      if (gimp_item_get_visible (GIMP_ITEM (vectors)))
        merge_list = g_list_prepend (merge_list, vectors);
    }

  merge_list = g_list_reverse (merge_list);

  if (merge_list && merge_list->next)
    {
      GimpVectors *target_vectors;
      gchar       *name;
      gint         pos;

      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE,
                                   C_("undo-type", "Merge Visible Paths"));

      vectors = GIMP_VECTORS (merge_list->data);

      name = g_strdup (gimp_object_get_name (vectors));
      pos = gimp_item_get_index (GIMP_ITEM (vectors));

      target_vectors = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                                          GIMP_TYPE_VECTORS));
      gimp_image_remove_vectors (image, vectors, TRUE, NULL);

      for (list = g_list_next (merge_list);
           list;
           list = g_list_next (list))
        {
          vectors = list->data;

          gimp_vectors_add_strokes (vectors, target_vectors);
          gimp_image_remove_vectors (image, vectors, TRUE, NULL);
        }

      gimp_object_take_name (GIMP_OBJECT (target_vectors), name);

      g_list_free (merge_list);

      /* FIXME tree */
      gimp_image_add_vectors (image, target_vectors, NULL, pos, TRUE);
      gimp_unset_busy (image->gimp);

      gimp_image_undo_group_end (image);

      return target_vectors;
    }
  else
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Not enough visible paths for a merge. "
			     "There must be at least two."));
      return NULL;
    }
}


/*  private functions  */

static GimpLayer *
gimp_image_merge_layers (GimpImage     *image,
                         GimpContainer *container,
                         GSList        *merge_list,
                         GimpContext   *context,
                         GimpMergeType  merge_type)
{
  GList            *list;
  GSList           *reverse_list = NULL;
  GSList           *layers;
  GimpLayer        *merge_layer;
  GimpLayer        *layer;
  GimpLayer        *bottom_layer;
  GimpParasiteList *parasites;
  gint              count;
  gint              x1, y1, x2, y2;
  gint              off_x, off_y;
  gint              position;
  gchar            *name;
  GimpLayer        *parent;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  layer        = NULL;
  x1 = y1      = 0;
  x2 = y2      = 0;
  bottom_layer = NULL;

  parent = gimp_layer_get_parent (merge_list->data);

  /*  Get the layer extents  */
  count = 0;
  while (merge_list)
    {
      layer = merge_list->data;

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      switch (merge_type)
        {
        case GIMP_EXPAND_AS_NECESSARY:
        case GIMP_CLIP_TO_IMAGE:
          if (! count)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + gimp_item_get_width  (GIMP_ITEM (layer));
              y2 = off_y + gimp_item_get_height (GIMP_ITEM (layer));
            }
          else
            {
              if (off_x < x1)
                x1 = off_x;
              if (off_y < y1)
                y1 = off_y;
              if ((off_x + gimp_item_get_width (GIMP_ITEM (layer))) > x2)
                x2 = (off_x + gimp_item_get_width (GIMP_ITEM (layer)));
              if ((off_y + gimp_item_get_height (GIMP_ITEM (layer))) > y2)
                y2 = (off_y + gimp_item_get_height (GIMP_ITEM (layer)));
            }

          if (merge_type == GIMP_CLIP_TO_IMAGE)
            {
              x1 = CLAMP (x1, 0, gimp_image_get_width  (image));
              y1 = CLAMP (y1, 0, gimp_image_get_height (image));
              x2 = CLAMP (x2, 0, gimp_image_get_width  (image));
              y2 = CLAMP (y2, 0, gimp_image_get_height (image));
            }
          break;

        case GIMP_CLIP_TO_BOTTOM_LAYER:
          if (merge_list->next == NULL)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + gimp_item_get_width  (GIMP_ITEM (layer));
              y2 = off_y + gimp_item_get_height (GIMP_ITEM (layer));
            }
          break;

        case GIMP_FLATTEN_IMAGE:
          if (merge_list->next == NULL)
            {
              x1 = 0;
              y1 = 0;
              x2 = gimp_image_get_width  (image);
              y2 = gimp_image_get_height (image);
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

  name = g_strdup (gimp_object_get_name (layer));

  if (merge_type == GIMP_FLATTEN_IMAGE ||
      (gimp_drawable_is_indexed (GIMP_DRAWABLE (layer)) &&
       ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer))))
    {
      GeglColor *color;
      GimpRGB    bg;

      merge_layer = gimp_layer_new (image, (x2 - x1), (y2 - y1),
                                    gimp_image_get_layer_format (image, FALSE),
                                    gimp_object_get_name (layer),
                                    GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer.", G_STRFUNC);
          return NULL;
        }

      gimp_item_set_offset (GIMP_ITEM (merge_layer), x1, y1);

      /*  get the background for compositing  */
      gimp_context_get_background (context, &bg);

      color = gimp_gegl_color_new (&bg);
      gegl_buffer_set_color (gimp_drawable_get_buffer (GIMP_DRAWABLE (merge_layer)),
                             GEGL_RECTANGLE(0,0,x2-x1,y2-y1), color);
      g_object_unref (color);

      position = 0;
    }
  else
    {
      /*  The final merged layer inherits the name of the bottom most layer
       *  and the resulting layer has an alpha channel whether or not the
       *  original did. Opacity is set to 100% and the MODE is set to normal.
       */

      merge_layer =
        gimp_layer_new (image, (x2 - x1), (y2 - y1),
                        gimp_drawable_get_format_with_alpha (GIMP_DRAWABLE (layer)),
                        "merged layer",
                        GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

      if (!merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);
          return NULL;
        }

      gimp_item_set_offset (GIMP_ITEM (merge_layer), x1, y1);

      /*  clear the layer  */
      gegl_buffer_clear (gimp_drawable_get_buffer (GIMP_DRAWABLE (merge_layer)),
                         NULL);

      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      layer = reverse_list->data;
      position =
        gimp_container_get_n_children (container) -
        gimp_container_get_child_index (container, GIMP_OBJECT (layer));
    }

  bottom_layer = layer;

  /* Copy the tattoo and parasites of the bottom layer to the new layer */
  gimp_item_set_tattoo (GIMP_ITEM (merge_layer),
                        gimp_item_get_tattoo (GIMP_ITEM (bottom_layer)));

  parasites = gimp_item_get_parasites (GIMP_ITEM (bottom_layer));
  parasites = gimp_parasite_list_copy (parasites);
  gimp_item_set_parasites (GIMP_ITEM (merge_layer), parasites);
  g_object_unref (parasites);

  for (layers = reverse_list; layers; layers = g_slist_next (layers))
    {
      GeglBuffer           *merge_buffer;
      GeglBuffer           *layer_buffer;
      GimpApplicator       *applicator;
      GimpLayerModeEffects  mode;

      layer = layers->data;

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      /* DISSOLVE_MODE is special since it is the only mode that does not
       *  work on the projection with the lower layer, but only locally on
       *  the layers alpha channel.
       */
      mode = gimp_layer_get_mode (layer);
      if (layer == bottom_layer && mode != GIMP_DISSOLVE_MODE)
        mode = GIMP_NORMAL_MODE;

      merge_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (merge_layer));
      layer_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      applicator =
        gimp_applicator_new (NULL,
                             gimp_drawable_get_linear (GIMP_DRAWABLE (layer)),
                             FALSE, FALSE);

      if (gimp_layer_get_mask (layer) &&
          gimp_layer_get_apply_mask (layer))
        {
          GeglBuffer *mask_buffer;

          mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer->mask));

          gimp_applicator_set_mask_buffer (applicator, mask_buffer);
          gimp_applicator_set_mask_offset (applicator,
                                           - (x1 - off_x),
                                           - (y1 - off_y));
        }

      gimp_applicator_set_src_buffer (applicator, merge_buffer);
      gimp_applicator_set_dest_buffer (applicator, merge_buffer);

      gimp_applicator_set_apply_buffer (applicator, layer_buffer);
      gimp_applicator_set_apply_offset (applicator,
                                        - (x1 - off_x),
                                        - (y1 - off_y));

      gimp_applicator_set_opacity (applicator, gimp_layer_get_opacity (layer));
      gimp_applicator_set_mode (applicator, mode);

      gimp_applicator_blit (applicator,
                            GEGL_RECTANGLE (0, 0,
                                            gegl_buffer_get_width  (merge_buffer),
                                            gegl_buffer_get_height (merge_buffer)));

      g_object_unref (applicator);

      gimp_image_remove_layer (image, layer, TRUE, NULL);
    }

  g_slist_free (reverse_list);

  gimp_object_take_name (GIMP_OBJECT (merge_layer), name);
  gimp_item_set_visible (GIMP_ITEM (merge_layer), TRUE, FALSE);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == GIMP_FLATTEN_IMAGE)
    {
      list = gimp_image_get_layer_iter (image);
      while (list)
        {
          layer = list->data;

          list = g_list_next (list);
          gimp_image_remove_layer (image, layer, TRUE, NULL);
        }

      gimp_image_add_layer (image, merge_layer, parent,
                            position, TRUE);
    }
  else
    {
      /*  Add the layer to the image  */

      gimp_image_add_layer (image, merge_layer, parent,
                            gimp_container_get_n_children (container) -
                            position + 1,
                            TRUE);
    }

  gimp_drawable_update (GIMP_DRAWABLE (merge_layer),
                        0, 0,
                        gimp_item_get_width  (GIMP_ITEM (merge_layer)),
                        gimp_item_get_height (GIMP_ITEM (merge_layer)));

  return merge_layer;
}
